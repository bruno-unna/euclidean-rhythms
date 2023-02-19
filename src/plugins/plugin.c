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
        unsigned short current_beat;
        unsigned short onsets;
        short rotation;
        unsigned short size_in_bars;

        float frames_per_second;
        long current_bar;
        long reference_frame;
        long *positions_vector;
        unsigned long euclidean;
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
    self->state.positions_vector = NULL;
    self->state.onsets = 0;
    self->state.rotation = 0;
    self->state.size_in_bars = 1;
    self->state.current_bar = 0;
    self->state.reference_frame = 0;
    self->state.euclidean = 0;
    self->state.frames_per_second = (float) rate;

    return (LV2_Handle) self;
}

static void cleanup(LV2_Handle instance) {
    Euclidean *self = (Euclidean *) instance;
    if (self->state.positions_vector != NULL) free(self->state.positions_vector);
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
    bool calculateEuclidean = false;

    unsigned short port_beats = (unsigned short) *self->ports.beats;
    if (port_beats != self->state.beats) {
        lv2_log_note(&self->logger, "plugin beats per bar set to %d\n", port_beats);
        self->state.beats = port_beats;
        if (self->state.positions_vector != NULL) free(self->state.positions_vector);
        self->state.positions_vector = calloc(port_beats, sizeof(long));
        calculateEuclidean = true;
    }

    unsigned short port_onsets = (unsigned short) *self->ports.onsets;
    if (port_onsets != self->state.onsets) {
        lv2_log_note(&self->logger, "plugin onsets set to %d\n", port_onsets);
        self->state.onsets = port_onsets;
        calculateEuclidean = true;
    }

    short port_rotation = (short) *self->ports.rotation;
    if (port_rotation != self->state.rotation) {
        lv2_log_note(&self->logger, "plugin rotation set to %d\n", port_rotation);
        self->state.rotation = port_rotation;
        calculateEuclidean = true;
    }

    unsigned short size_in_bars = (unsigned short) *self->ports.size_in_bars;
    if (size_in_bars != self->state.size_in_bars) {
        lv2_log_note(&self->logger, "size of the pattern (in bars) set to %d\n", size_in_bars);
        self->state.size_in_bars = size_in_bars;
    }

    if (calculateEuclidean)
        self->state.euclidean = e((unsigned short) *self->ports.onsets,
                                  (unsigned short) *self->ports.beats,
                                  (short) *self->ports.rotation);

    LV2_ATOM_SEQUENCE_FOREACH(self->ports.control, ev) {
        if (ev->body.type == uris->atom_Object) {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *) &ev->body;

            if (obj->body.otype == uris->time_Position) {
                // Received new transport position/host_speed_atom
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

                float beats_per_minute;
                if (host_beats_per_minute_atom != 0) {
                    beats_per_minute = (float) ((LV2_Atom_Float *) host_beats_per_minute_atom)->body;
                    lv2_log_note(&self->logger, "beats per minute set to %f\n", beats_per_minute);
                }

                float beats_per_bar;
                if (host_beats_per_bar_atom != 0) {
                    beats_per_bar = (float) ((LV2_Atom_Float *) host_beats_per_bar_atom)->body;
                    lv2_log_note(&self->logger, "beats per bar set to %f\n", beats_per_bar);
                }

                long frame = 0;
                if (host_frame_atom != 0) {
                    frame = (long) ((LV2_Atom_Long *) host_frame_atom)->body;
                }

                float speed;
                if (host_speed_atom != 0) {
                    speed = (float) ((LV2_Atom_Float *) host_speed_atom)->body;
                }

                if (host_bar_atom != 0) {
                    const long current_bar = (long) ((LV2_Atom_Long *) host_bar_atom)->body;
                    if (current_bar != self->state.current_bar && current_bar % self->state.size_in_bars == 0) {
                        // The bar has changed for a new pattern to begin
                        self->state.current_bar = current_bar;
                        self->state.reference_frame = frame;
                        lv2_log_note(&self->logger, "the bar is now %ld\n", self->state.current_bar);

                        // How many frames per bar?
                        const long frames_per_bar = (long) (60 * self->state.frames_per_second / beats_per_minute *
                                                            beats_per_bar);
                        lv2_log_note(&self->logger, "frames per bar: %ld\n", frames_per_bar);

                        // How many frames per pattern?
                        const long frames_per_pattern = frames_per_bar * size_in_bars;
                        lv2_log_note(&self->logger, "frames per pattern: %ld\n", frames_per_pattern);

                        const long delta = frames_per_pattern / port_beats;
                        lv2_log_note(&self->logger, "delta: %ld\n", delta);
                        self->state.positions_vector[0] = frame;
                        for (int i = 1; i < port_beats; ++i) {
                            self->state.positions_vector[i] = self->state.positions_vector[i - 1] + delta;
                        }

                        lv2_log_note(&self->logger, "positions vector (of size %d):\n", port_beats);
                        for (int i = 0; i < port_beats; ++i) {
                            lv2_log_note(&self->logger, "\t%ld, ", self->state.positions_vector[i]);
                        }
                        lv2_log_note(&self->logger, "\n");
                    }
                }

                // Perhaps produce a MIDI event?
                if (speed > 0) {
                    // Find the beat!
                    // This, in conjunction with the calculated e, will be used to determine
                    // whether to produce an event or not
                    unsigned short beat;
                    lv2_log_note(&self->logger, "frame -  %ld\n", frame);
                    for (beat = 1; beat < port_beats; ++beat) {
                        lv2_log_note(&self->logger, "comparing %ld with %ld...\n", frame,
                                     self->state.positions_vector[beat]);
                        if (frame < self->state.positions_vector[beat]) {
                            lv2_log_note(&self->logger, "found the beat as %ud\n", beat);
                            break;
                        }
                    }
                    beat--;

                    if (beat != self->state.current_beat) {
                        self->state.current_beat = beat;
                        lv2_log_trace(&self->logger, "the beat is now %d\n", beat);

                        if (self->state.euclidean & 1 << (self->state.beats - beat - 1)) {
                            MIDI_note_event note;
                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_ON + (int) *self->ports.channel - 1;
                            note.msg[1] = (int) *self->ports.note;
                            note.msg[2] = (int) *self->ports.velocity;
                            lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);

                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_OFF + (int) *self->ports.channel - 1;
                            note.msg[1] = (int) *self->ports.note;
                            note.msg[2] = 0x00;
                            lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);
                        }
                    }
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
