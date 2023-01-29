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

#include <stdio.h>
#include "../include/euclidean.h"

int main() {
    unsigned long r;

    printf("Testing e(onsets: 0, beats: 4, rotation: 0)\n");
    r = e(0, 4, 0);
    if (r == 0b0000) {
        printf("Received the expected result (....)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b0000L);
        return 1;
    }

    printf("Testing e(onsets: 3, beats: 8, rotation: 0)\n");
    r = e(3, 8, 0);
    if (r == 0b10010010) {
        printf("Received the expected result (x..x..x.)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b10010010L);
        return 1;
    }

    printf("Testing e(onsets: 3, beats: 8, rotation: 1)\n");
    r = e(3, 8, 1);
    if (r == 0b00100101) {
        printf("Received the expected result (..x..x.x)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b00100101L);
        return 1;
    }

    printf("Testing e(onsets: 3, beats: 8, rotation: -2)\n");
    r = e(3, 8, -2);
    if (r == 0b10100100) {
        printf("Received the expected result (x.x..x..)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b10100100L);
        return 1;
    }

    printf("Testing e(onsets: 4, beats: 6, rotation: 0)\n");
    r = e(4, 6, 0);
    if (r == 0b101101) {
        printf("Received the expected result (x.xx.x)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b101101L);
        return 1;
    }

    printf("Testing e(onsets: 7, beats: 8, rotation: 0)\n");
    r = e(7, 8, 0);
    if (r == 0b11111110) {
        printf("Received the expected result (xxxxxxx.)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b11111110L);
        return 1;
    }

    printf("Testing e(onsets: 4, beats: 16, rotation: 0)\n");
    r = e(4, 16, 0);
    if (r == 0b1000100010001000) {
        printf("Received the expected result (x...x...x...x...)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b1000100010001000L);
        return 1;
    }

    printf("Testing e(onsets: 1, beats: 4, rotation: 0)\n");
    r = e(1, 4, 0);
    if (r == 0b1000) {
        printf("Received the expected result (x...)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b1000L);
        return 1;
    }

    printf("Testing e(onsets: 10, beats: 8, rotation: 0)\n");
    r = e(10, 8, 0);
    if (r == 0b11111111) {
        printf("Received the expected result (xxxxxxxx)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b11111111L);
        return 1;
    }

    printf("Testing e(onsets: 4, beats: 7, rotation: 0)\n");
    r = e(4, 7, 0);
    if (r == 0b1010101) {
        printf("Received the expected result (x.x.x.x)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b1010101L);
        return 1;
    }

    return 0;
}