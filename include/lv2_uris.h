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

#ifndef LV2_URIS_H
#define LV2_URIS_H

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include "lv2/patch/patch.h"
#include "lv2/urid/urid.h"

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

#endif //LV2_URIS_H
