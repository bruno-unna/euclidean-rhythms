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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2_util.h>

#include "euclidean.h"
#include "lv2_uris.h"
#include "plugin.h"

typedef struct {
    LV2_URID_Map *map;     // URID map feature
    LV2_Log_Logger logger; // Logger API
    Euclidean_URIs uris;    // Cache of mapped URIDs

    struct {
        const LV2_Atom_Sequence *control;
        const float *knobs[N_GENERATORS][N_KNOBS];
        LV2_Atom_Sequence *midi_out;
    } ports;

    // this state is common to all generators
    struct {
        float speed;
        float beats_per_minute;
        float beats_per_bar;
        long current_bar;
        float frames_per_second;
    } common_state;

    // this state is particular to each generator
    struct {
        unsigned short beats;
        unsigned short onsets;
        short rotation;
        unsigned short size_in_bars;

        unsigned long euclidean;

        long reference_frame;
        unsigned short note_on_index;
        unsigned short note_off_index;
        long *note_on_vector;
        long *note_off_vector;

        unsigned short playing;
    } state[N_GENERATORS];
} Euclidean;

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
    Euclidean *self = (Euclidean *) instance;

    if (port == CONTROL_PORT) {
        self->ports.control = (const LV2_Atom_Sequence *) data;
    } else if (port == MIDI_OUT_PORT) {
        self->ports.midi_out = (LV2_Atom_Sequence *) data;
    } else {
        unsigned short generator = (port - 1) / N_KNOBS;
        unsigned short knob = (port - 1) % N_KNOBS;
        self->ports.knobs[generator][knob] = (float *) data;
    }
}

static void recalculate_onsets(Euclidean *self) {
    const float fps = self->common_state.frames_per_second;
    const float bpm = self->common_state.beats_per_minute;
    const float beats_per_bar = self->common_state.beats_per_bar;

    // How many frames per bar?
    const long frames_per_bar = (long) (60 * fps / bpm * beats_per_bar);

    // How many frames per MIDI tick (minimum sensible length of a note)?
    const long frames_per_tick = (long) ((60 * fps) / (bpm * 24));

    for (unsigned short gen = 0; gen < N_GENERATORS; ++gen) {
        const unsigned short size_in_bars = self->state[gen].size_in_bars;
        const unsigned short beats = self->state[gen].beats;
        const long reference_frame = self->state[gen].reference_frame;
        unsigned long et = self->state[gen].euclidean;
        long *note_on = self->state[gen].note_on_vector;
        long *note_off = self->state[gen].note_off_vector;

        // How many frames per pattern?
        const long frames_per_pattern = frames_per_bar * size_in_bars;

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
    self->common_state.current_bar = -1;
    self->common_state.frames_per_second = (float) rate;
    for (unsigned short gen = 0; gen < N_GENERATORS; ++gen) {
        self->state[gen].note_on_vector = NULL;
        self->state[gen].note_off_vector = NULL;
        self->state[gen].beats = 8;
        self->state[gen].onsets = 0;
        self->state[gen].rotation = 0;
        self->state[gen].size_in_bars = 1;
        self->state[gen].reference_frame = 0;
        self->state[gen].euclidean = 0;
        self->state[gen].playing = 0;
    }
    return (LV2_Handle) self;
}

static void cleanup(LV2_Handle instance) {
    Euclidean *self = (Euclidean *) instance;
    for (unsigned short gen = 0; gen < N_GENERATORS; ++gen) {
        if (self->state[gen].note_on_vector != NULL) free(self->state[gen].note_on_vector);
        if (self->state[gen].note_off_vector != NULL) free(self->state[gen].note_off_vector);
    }
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

    for (unsigned short gen = 0; gen < N_GENERATORS; ++gen) {
        // Analyse the parameters
        bool calculate_euclidean = false;

        unsigned short port_beats = (unsigned short) *self->ports.knobs[gen][BEATS_IDX];
        if (port_beats != self->state[gen].beats) {
            lv2_log_trace(&self->logger, "[gen %d] plugin beats per bar set to %d\n", gen, port_beats);
            self->state[gen].beats = port_beats;
            calculate_euclidean = true;
        }

        unsigned short port_onsets = (unsigned short) *self->ports.knobs[gen][ONSETS_IDX];
        if (port_onsets != self->state[gen].onsets) {
            lv2_log_trace(&self->logger, "[gen %d] plugin onsets set to %d\n", gen, port_onsets);
            self->state[gen].onsets = port_onsets;
            calculate_euclidean = true;
        }

        short port_rotation = (short) *self->ports.knobs[gen][ROTATION_IDX];
        if (port_rotation != self->state[gen].rotation) {
            lv2_log_trace(&self->logger, "[gen %d] plugin rotation set to %d\n", gen, port_rotation);
            self->state[gen].rotation = port_rotation;
            calculate_euclidean = true;
        }

        unsigned short size_in_bars = (unsigned short) *self->ports.knobs[gen][BARS_IDX];
        if (size_in_bars != self->state[gen].size_in_bars) {
            lv2_log_trace(&self->logger, "[gen %d] size of the pattern (in bars) set to %d\n", gen, size_in_bars);
            self->state[gen].size_in_bars = size_in_bars;
            calculate_euclidean = true;
        }

        if (calculate_euclidean) {
            if (self->state[gen].note_on_vector != NULL) free(self->state[gen].note_on_vector);
            if (self->state[gen].note_off_vector != NULL) free(self->state[gen].note_off_vector);
            self->state[gen].note_on_vector = calloc(self->state[gen].onsets + 1, sizeof(long));
            self->state[gen].note_off_vector = calloc(self->state[gen].onsets + 1, sizeof(long));
            self->state[gen].note_on_vector[self->state[gen].onsets] = INT64_MAX;
            self->state[gen].note_off_vector[self->state[gen].onsets] = INT64_MAX;

            lv2_log_trace(&self->logger, "[gen %d] recalculating euclidean\n", gen);

            self->state[gen].euclidean = e((unsigned short) *self->ports.knobs[gen][ONSETS_IDX],
                                           (unsigned short) *self->ports.knobs[gen][BEATS_IDX],
                                           (short) *self->ports.knobs[gen][ROTATION_IDX]);

            recalculate_onsets(self);
        }
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
                    self->common_state.speed = speed;
                }

                bool dirty_vector = false;

                if (host_beats_per_minute_atom != 0) {
                    const float beats_per_minute = (float) ((LV2_Atom_Float *) host_beats_per_minute_atom)->body;
                    if (self->common_state.beats_per_minute != beats_per_minute) {
                        self->common_state.beats_per_minute = beats_per_minute;

                        lv2_log_trace(&self->logger, "dirtying the onsets vector because bpm changed to %f\n",
                                     beats_per_minute);
                        dirty_vector = true;
                    }
                }

                if (host_beats_per_bar_atom != 0) {
                    const float beats_per_bar = (float) ((LV2_Atom_Float *) host_beats_per_bar_atom)->body;
                    if (self->common_state.beats_per_bar != beats_per_bar) {
                        self->common_state.beats_per_bar = beats_per_bar;

                        lv2_log_trace(&self->logger, "dirtying the onsets vector because beats per bar changed to %f\n",
                                     beats_per_bar);
                        dirty_vector = true;
                    }
                }

                if (host_bar_atom != 0) {
                    const long current_bar = (long) ((LV2_Atom_Long *) host_bar_atom)->body;
                    if (current_bar != self->common_state.current_bar) {
                        // The bar has changed for a new pattern to begin
                        self->common_state.current_bar = current_bar;
                        for (unsigned short gen = 0; gen < N_GENERATORS; ++gen) {
                            if (current_bar % self->state[gen].size_in_bars == 0) {
                                self->state[gen].reference_frame = frame;

                                self->state[gen].note_on_index = 0;
                                self->state[gen].note_off_index = 0;
                            }
                        }
                        lv2_log_trace(&self->logger,
                                     "dirtying the onsets vector because the bar has changed to %ld\n",
                                     current_bar);
                        dirty_vector = true;
                    }
                }

                if (dirty_vector == true)
                    recalculate_onsets(self);

                for (unsigned short gen = 0; gen < N_GENERATORS; ++gen) {
                    // Perhaps produce a MIDI event?
                    if (self->common_state.speed > 0 &&
                        frame >= self->state[gen].note_on_vector[self->state[gen].note_on_index]) {

                        if (self->state[gen].playing == 0) {
                            MIDI_note_event note;
                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_ON + (int) *self->ports.knobs[gen][CHANNEL_IDX] - 1;
                            note.msg[1] = (int) *self->ports.knobs[gen][NOTE_IDX];
                            note.msg[2] = (int) *self->ports.knobs[gen][VELOCITY_IDX];
                            self->state[gen].playing = (int) *self->ports.knobs[gen][NOTE_IDX];
                            lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);
                        }
                        self->state[gen].note_on_index++;
                    }
                    if (self->common_state.speed > 0 &&
                        frame >= self->state[gen].note_off_vector[self->state[gen].note_off_index]) {
                        if (self->state[gen].playing > 0) {
                            MIDI_note_event note;
                            note.event.time.frames = ev->time.frames;
                            note.event.body.type = uris->midi_Event;
                            note.event.body.size = 3;
                            note.msg[0] = LV2_MIDI_MSG_NOTE_OFF + (int) *self->ports.knobs[gen][CHANNEL_IDX] - 1;
                            note.msg[1] = self->state[gen].playing;
                            note.msg[2] = 0x00;
                            self->state[gen].playing = 0;
                            lv2_atom_sequence_append_event(self->ports.midi_out, out_capacity, &note.event);
                        }
                        self->state[gen].note_off_index++;
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
