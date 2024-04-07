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

#include <stdio.h>

typedef struct {
    unsigned short n; // how many repetitions?
    unsigned long v;  // of what sequence?
    unsigned short s; // how long is the sequence?
} repeats;

void er(repeats *g, repeats *r) {
    while (r->n > 1) {
        if (g->n <= r->n) {
            // distribute some elements of `r` amongst the elements of `g`
            r->n -= g->n;
            g->s += r->s;
            g->v <<= r->s;
            g->v |= r->v;
        } else {
            // forget `r` and split old `g` unto new `g` and new `r`
            repeats rt = *r;    // this is black magic: I don't need to memcpy the struct!
            r->v = g->v;
            r->n = g->n - r->n;
            r->s = g->s;
            g->v <<= rt.s;
            g->v |= rt.v;
            g->s += rt.s;
            g->n = rt.n;
        }
    }
}

unsigned long e(unsigned short onsets, unsigned short beats, short rotation) {
    unsigned long result = 0L;

    if (beats == 0) {
        fprintf(stderr, "number of beats can't be zero\n");
        result = 0;
    } else if (onsets == 0) {
        result = 0;
    } else if (onsets > beats) {
        fprintf(stderr,
                "number of onsets (%d) can't be larger than number of beats (%d)\n",
                onsets, beats);
        for (unsigned short i = 0; i < beats; ++i) {
            result <<= 1;
            result |= 1;
        }
    } else {
        repeats g = {onsets, 0b1, 1};
        repeats r = {beats - onsets, 0b0, 1};
        er(&g, &r);

        result = g.v;
        for (unsigned short i = 1; i < g.n; ++i) {
            result <<= g.s;
            result |= g.v;
        }
        if (r.n > 0) {
            result <<= r.s;
            result |= r.v;
        }
    }
    for (int i = 0; i < rotation; ++i) {
        unsigned short lowBit = (result & 1 << (beats - 1)) >> (beats - 1);
        result &= (long) -1 ^ 1 << (beats - 1);
        result <<= 1;
        result |= lowBit;
    }
    for (int i = 0; i > rotation; --i) {
        unsigned long highBit = (result & 1) << (beats - 1);
        result >>= 1;
        result |= highBit;
    }
    return result;
}