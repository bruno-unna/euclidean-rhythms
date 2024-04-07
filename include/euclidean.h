/*
 * Copyright 2023, 2024 by Bruno Unna.
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

#ifndef EUCLIDEAN_H
#define EUCLIDEAN_H

#define EUCLIDEAN_URI "https://github.com/bruno-unna/euclidean-rhythms"
#define EUCLIDEAN_UI_URI "https://github.com/bruno-unna/euclidean-rhythms#ui"

#define N_GENERATORS 8
#define N_PARAMETERS 8

#define CONTROL_PORT 0
#define MIDI_OUT_PORT 1

enum {
    ENABLED_IDX = 0,
    BEATS_IDX = 1,
    ONSETS_IDX = 2,
    ROTATION_IDX = 3,
    BARS_IDX = 4,
    CHANNEL_IDX = 5,
    NOTE_IDX = 6,
    VELOCITY_IDX = 7,
};

unsigned long e(unsigned short onsets, unsigned short beats, short rotation);

#endif //EUCLIDEAN_H
