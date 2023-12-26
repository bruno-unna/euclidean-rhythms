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

#define EUCLIDEAN_URI "https://github.com/bruno-unna/euclidean-rhythms"
#define EUCLIDEAN_UI_URI "https://github.com/bruno-unna/euclidean-rhythms#ui"

#define N_GENERATORS 1
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

#endif //EUCLIDEAN_PLUGIN_H
