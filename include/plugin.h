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

#ifndef EUCLIDEAN_PLUGIN_H
#define EUCLIDEAN_PLUGIN_H

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2_util.h>

#include "lv2_uris.h"

#define EUCLIDEAN_URI "https://github.com/bruno-unna/euclidean-rhythms"
#define EUCLIDEAN_UI_URI "https://github.com/bruno-unna/euclidean-rhythms#ui"

#define N_GENERATORS 8
#define N_KNOBS 7

#define CONTROL_PORT 0
#define MIDI_OUT_PORT (1 + N_GENERATORS * N_KNOBS)

enum {
    BEATS_IDX = 0,
    ONSETS_IDX = 1,
    ROTATION_IDX = 2,
    BARS_IDX = 3,
    CHANNEL_IDX = 4,
    NOTE_IDX = 5,
    VELOCITY_IDX = 6
};

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

#endif //EUCLIDEAN_PLUGIN_H
