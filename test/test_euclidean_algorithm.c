#include <stdio.h>
#include "../include/euclidean.h"

int main() {
    unsigned long r;

    r = e(0, 4, 0);
    if (r == 0b0000) {
        printf("Received the expected result (....)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0x00L);
        return 1;
    }

    r = e(3, 8, 0);
    if (r == 0b10010010) {
        printf("Received the expected result (x..x..x.)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0x52L);
        return 1;
    }

    r = e(3, 8, 1);
    if (r == 0b00100101) {
        printf("Received the expected result (..x..x.x)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b00100101L);
        return 1;
    }

    r = e(3, 8, -2);
    if (r == 0b10100100) {
        printf("Received the expected result (x.x..x..)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0b10100100L);
        return 1;
    }

    r = e(4, 6, 0);
    if (r == 0b101101) {
        printf("Received the expected result (x.xx.x)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0x2dL);
        return 1;
    }

    r = e(7, 8, 0);
    if (r == 0b11111110) {
        printf("Received the expected result (xxxxxxx.)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0xfeL);
        return 1;
    }

    r = e(4, 16, 0);
    if (r == 0b1000100010001000) {
        printf("Received the expected result (x...x...x...x...)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0x8888L);
        return 1;
    }

    r = e(1, 4, 0);
    if (r == 0b1000) {
        printf("Received the expected result (x...)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0x8L);
        return 1;
    }

    r = e(10, 8, 0);
    if (r == 0b11111111) {
        printf("Received the expected result (xxxxxxxx)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0xffL);
        return 1;
    }

    r = e(4, 7, 0);
    if (r == 0b1010101) {
        printf("Received the expected result (x.x.x.x)\n");
    } else {
        printf("Received a wrong result (0x%lx), expecting 0x%lx\n", r, 0x33L);
        return 1;
    }

    return 0;
}