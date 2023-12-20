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

#endif //EUCLIDEAN_PLUGIN_H
