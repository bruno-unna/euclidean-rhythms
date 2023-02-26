/*
 * Copyright 2023 by Bruno Unna.
 *
 * This file is part of Euclidean Rhythms.
 *
 * Euclidean Rhythms is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Euclidean Rhythms is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Euclidean Rhythms.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#define EUCLIDEAN_URI "https://github.com/bruno-unna/euclidean-rhythms"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2_util.h>
#include "lv2/patch/patch.h"
#include "lv2/urid/urid.h"
#include "euclidean.h"

typedef struct {
    LV2_URID atom_Float;
    LV2_URID atom_Long;
    LV2_URID atom_Object;
    LV2_URID atom_Path;
    LV2_URID atom_Sequence;
    LV2_URID atom_URID;
    LV2_URID midi_Event;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
    LV2_URID time_Position;
    LV2_URID time_Rate;
    LV2_URID time_frames_per_second;
    LV2_URID time_beats_per_minute;
    LV2_URID time_beats_per_bar;
    LV2_URID time_bar;
    LV2_URID time_frame;
    LV2_URID time_speed;
} Euclidean_URIs;

typedef enum {
    EUCLIDEAN_CONTROL = 0,
    EUCLIDEAN_BEATS = 1,
    EUCLIDEAN_ONSETS = 2,
    EUCLIDEAN_ROTATION = 3,
    EUCLIDEAN_BARS = 4,
    EUCLIDEAN_CHANNEL = 5,
    EUCLIDEAN_NOTE = 6,
    EUCLIDEAN_VELOCITY = 7,
    EUCLIDEAN_MIDI_OUT = 8
} Port_index;

typedef struct {
    LV2_URID_Map *map;     // URID map feature
    LV2_Log_Logger logger; // Logger API
    Euclidean_URIs uris;    // Cache of mapped URIDs

    struct {
        const LV2_Atom_Sequence *control;
        const float *beats;
        const float *onsets;
        const float *rotation;
        const float *size_in_bars;
        const float *channel;
        const float *note;
        const float *velocity;
        LV2_Atom_Sequence *midi_out;
    } ports;

    struct {
        unsigned short beats;
        unsigned short onsets;
        short rotation;
        unsigned short size_in_bars;

        unsigned long euclidean;

        float speed;
        float beats_per_minute;
        float beats_per_bar;
        float frames_per_second;

        long current_bar;
        long reference_frame;
        unsigned short note_on_index;
        unsigned short note_off_index;
        long *note_on_vector;
        long *note_off_vector;
    } state;
} Euclidean;

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    Euclidean *self = (Euclidean *) instance;

    switch (port) {
        case EUCLIDEAN_CONTROL:
            self->ports.control = (const LV2_Atom_Sequence *) data;
            break;
        case EUCLIDEAN_BEATS:
            self->ports.beats = (float *) data;
            break;
        case EUCLIDEAN_ONSETS:
            self->ports.onsets = (float *) data;
            break;
        case EUCLIDEAN_ROTATION:
            self->ports.rotation = (float *) data;
            break;
        case EUCLIDEAN_BARS:
            self->ports.size_in_bars = (float *) data;
            break;
        case EUCLIDEAN_CHANNEL:
            self->ports.channel = (float *) data;
            break;
        case EUCLIDEAN_NOTE:
            self->ports.note = (float *) data;
            break;
        case EUCLIDEAN_VELOCITY:
            self->ports.velocity = (float *) data;
            break;
        case EUCLIDEAN_MIDI_OUT:
            self->ports.midi_out = (LV2_Atom_Sequence *) data;
            break;
        default:
            break;
    }
}

static inline void map_uris(LV2_URID_Map *map, Euclidean_URIs *uris) {
    uris->atom_Float = map->map(map->handle, LV2_ATOM__Float);
    uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_Path = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_Sequence = map->map(map->handle, LV2_ATOM__Sequence);
    uris->atom_URID = map->map(map->handle, LV2_ATOM__URID);
    uris->midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);
    uris->patch_Set = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_property = map->map(map->handle, LV2_PATCH__property);
    uris->patch_value = map->map(map->handle, LV2_PATCH__value);
    uris->time_Position = map->map(map->handle, LV2_TIME__Position);
    uris->time_beats_per_minute = map->map(map->handle, LV2_TIME__beatsPerMinute);
    uris->time_beats_per_bar = map->map(map->handle, LV2_TIME__beatsPerBar);
    uris->time_bar = map->map(map->handle, LV2_TIME__bar);
    uris->time_frame = map->map(map->handle, LV2_TIME__frame);
    uris->time_speed = map->map(map->handle, LV2_TIME__speed);
}

static void recalculate_onsets(Euclidean *self) {
    const float fps = self->state.frames_per_second;
    const float bpm = self->state.beats_per_minute;
    const float beats_per_bar = self->state.beats_per_bar;
    const unsigned short size_in_bars = self->state.size_in_bars;
    const unsigned short beats = self->state.beats;
    const long reference_frame = self->state.reference_frame;
    unsigned long et = self->state.euclidean;
    long *note_on = self->state.note_on_vector;
    long *note_off = self->state.note_off_vector;

    // How many frames per bar?
    const long frames_per_bar = (long) (60 * fps / bpm * beats_per_bar);

    // How many frames per pattern?
    const long frames_per_pattern = frames_per_bar * size_in_bars;

    // How many frames per MIDI tick?
    const long frames_per_tick = (long) ((60 * fps) / (bpm * 24));

    const long delta = frames_per_pattern / beats;

    long f = reference_frame;
    const unsigned long mask = 1L << (beats - 1);
    for (int i = 0, j = 0; i < beats; ++i) {
        if ((et & mask) != 0L) {
            note_on[j] = f;
            note_off[j] = f + frames_per_tick;
            ++j;
        }
        et <<= 1;
        f += delta;
    }
}

static LV2_Handle instantiate(const LV2_Descriptor *descriptor,
                              double rate,
                              const char *path,
                              const LV2_Feature *const *features) {
    Euclidean *self = (Euclidean *) calloc(1, sizeof(Euclidean));
    if (!self) {
        return NULL;
    }

    self->logger.log = NULL;
    self->map = NULL;
    // clang-format off
    const char *missing = lv2_features_query(features,
                                             LV2_LOG__log, &self->logger.log, false,
                                             LV2_URID__map, &self->map, true,
                                             NULL);
    // clang-format on

    lv2_log_logger_set_map(&self->logger, self->map);

    if (missing) {
        lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
        free(self);
        return NULL;
    }

    map_uris(self->map, &self->uris);

    // Initialise instance fields
    self->state.note_on_vector = NULL;
    self->state.note_off_vector = NULL;
    self->state.onsets = 0;
    self->state.rotation = 0;
    self->state.size_in_bars = 1;
    self->state.current_bar = -1;
    self->state.reference_frame = 0;
    self->state.euclidean = 0;
    self->state.frames_per_second = (float) rate;

    return (LV2_Handle) self;
}

static void cleanup(LV2_Handle instance) {
    Euclidean *self = (Euclidean *) instance;
    if (self->state.note_on_vector != NULL) free(self->state.note_on_vector);
    if (self->state.note_off_vector != NULL) free(self->state.note_off_vector);
    free(instance);
}

static void run(LV2_Handle instance, uint32_t sample_count) {
    Euclidean *self = (Euclidean *) instance;
    Euclidean_URIs *uris = &self->uris;

    typedef struct {
        LV2_Atom_Event event;
        uint8_t msg[3];
    } MIDI_note_event;

    const uint32_t out_capacity = self->ports.midi_out->atom.size;

    // Write an empty Sequence header to the output
    lv2_atom_sequence_clear(self->ports.midi_out);
    self->ports.midi_out->atom.type = uris->atom_Sequence;

    // Analyse the parameters
    bool calculate_euclidean = false;

    unsigned short port_beats = (unsigned short) *self->ports.beats;
    if (port_beats != self->state.beats) {
        lv2_log_trace(&self->logger, "plugin beats per bar set to %d\n", port_beats);
        self->state.beats = port_beats;
        calculate_euclidean = true;
    }

    unsigned short port_onsets = (unsigned short) *self->ports.onsets;
    if (port_onsets != self->state.onsets) {
        lv2_log_trace(&self->logger, "plugin onsets set to %d\n", port_onsets);
        self->state.onsets = port_onsets;
        calculate_euclidean = true;
    }

    short port_rotation = (short) *self->ports.rotation;
    if (port_rotation != self->state.rotation) {
        lv2_log_trace(&self->logger, "plugin rotation set to %d\n", port_rotation);
        self->state.rotation = port_rotation;
        calculate_euclidean = true;
    }

    unsigned short size_in_bars = (unsigned short) *self->ports.size_in_bars;
    if (size_in_bars != self->state.size_in_bars) {
        lv2_log_trace(&self->logger, "size of the pattern (in bars) set to %d\n", size_in_bars);
        self->state.size_in_bars = size_in_bars;
        calculate_euclidean = true;
    }

    if (calculate_euclidean) {
        if (self->state.note_on_vector != NULL) free(self->state.note_on_vector);
        if (self->state.note_off_vector != NULL) free(self->state.note_off_vector);
        self->state.note_on_vector = calloc(self->state.onsets + 1, sizeof(long));
        self->state.note_off_vector = calloc(self->state.onsets + 1, sizeof(long));
        self->state.note_on_vector[self->state.onsets] = INT64_MAX;
        self->state.note_off_vector[self->state.onsets] = INT64_MAX;

        self->state.euclidean = e((unsigned short) *self->ports.onsets,
                                  (unsigned short) *self->ports.beats,
                                  (short) *self->ports.rotation);
    }

    LV2_ATOM_SEQUENCE_FOREACH(self->ports.control, ev) {
        if (ev->body.type == uris->atom_Object) {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *) &ev->body;

            if (obj->body.otype == uris->time_Position) {
                // Position information from the host will be stored here
                LV2_Atom const *host_beats_per_minute_atom = NULL;
                LV2_Atom const *host_beats_per_bar_atom = NULL;
                LV2_Atom const *host_bar_atom = NULL;
                LV2_Atom const *host_frame_atom = NULL;
                LV2_Atom const *host_speed_atom = NULL;
                // clang-format off
                lv2_atom_object_get(obj,
                                    uris->time_beats_per_minute, &host_beats_per_minute_atom,
                                    uris->time_beats_per_bar, &host_beats_per_bar_atom,
                                    uris->time_bar, &host_bar_atom,
                                    uris->time_frame, &host_frame_atom,
                                    uris->time_speed, &host_speed_atom,
                                    NULL);
                // clang-format on

                long frame = -1;
                if (host_frame_atom != 0) {
                    frame = (long) ((LV2_Atom_Long *) host_frame_atom)->body;
                }

                if (host_speed_atom != 0) {
                    const float speed = (float) ((LV2_Atom_Float *) host_speed_atom)->body;
                    self->state.speed = speed;
                }

                bool dirty_vector = false;

                if (host_beats_per_minute_atom != 0) {
                    const float beats_per_minute = (float) ((LV2_Atom_Float *) host_beats_per_minute_atom)->body;
                    if (self->state.beats_per_minute != beats_per_minute) {
                        self->state.beats_per_minute = beats_per_minute;

                        lv2_log_trace(&self->logger, "dirtying the onsets vector because bpm changed to %f\n",
                                      beats_per_minute);
                        dirty_vector = true;
                    }
                }

                if (host_beats_per_bar_atom != 0) {
                    const float beats_per_bar = (float) ((LV2_Atom_Float *) host_beats_per_bar_atom)->body;
                    if (self->state.beats_per_bar != beats_per_bar) {
                        self->state.beats_per_bar = beats_per_bar;

                        lv2_log_trace(&self->logger, "dirtying the onsets vector because beats per bar changed to %f\n",
                                      beats_per_bar);
                        dirty_vector = true;
                    }
                }

                if (host_bar_atom != 0) {
                    const long current_bar = (long) ((LV2_Atom_Long *) host_bar_atom)->body;
                    if (current_bar != self->state.current_bar && current_bar % self->state.size_in_bars == 0) {
                        // The bar has changed for a new pattern to begin
                        self->state.current_bar = current_bar;
                        self->state.reference_frame = frame;

                        self->state.note_on_index = 0;
                        self->state.note_off_index = 0;

                        lv2_log_trace(&self->logger, "dirtying the onsets vector because the bar has changed to %ld\n",
                                      current_bar);
                        dirty_vector = true;
                    }
                }

                if (dirty_vector == true)
                    recalculate_onsets(self);

                // Perhaps produce a MIDI event?
                if (self->state.speed > 0 && frame >= self->state.note_on_vector[self->state.note_on_index]) {
                    MIDI_note_event note;
                    note.event.time.frames = frame;
                    note.event.body.type = uris->midi_Event;
                    note.event.body.size = 3;
                    note.msg[0] = LV2_MIDI_MSG_NOTE_ON + (int) *self->ports.channel - 1;
                    note.msg[1] = (int) *self->ports.note;
                    note.msg[2] = (int) *self->ports.velocity;
                    lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);
                    self->state.note_on_index++;
                }
                if (self->state.speed > 0 && frame >= self->state.note_off_vector[self->state.note_off_index]) {
                    MIDI_note_event note;
                    note.event.time.frames = frame;
                    note.event.body.type = uris->midi_Event;
                    note.event.body.size = 3;
                    note.msg[0] = LV2_MIDI_MSG_NOTE_OFF + (int) *self->ports.channel - 1;
                    note.msg[1] = (int) *self->ports.note;
                    note.msg[2] = 0x00;
                    lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);
                    self->state.note_off_index++;
                }
            }
        }
    }
}

// clang-format off
static const LV2_Descriptor descriptor = {
        EUCLIDEAN_URI,
        instantiate,
        connect_port,
        NULL,   // activate
        run,
        NULL, // deactivate,
        cleanup,
        NULL, // extension_data
};
// clang-format on

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : NULL;
}
