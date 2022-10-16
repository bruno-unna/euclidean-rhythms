#include <stdio.h>

typedef struct {
  unsigned short n; // how many repetitions?
  unsigned long v;  // of what sequence?
  unsigned short s; // how long is the sequence?
} repeats;

void er(repeats *g, repeats *r) {
  if (r->n <= 1)
    return;
  if (g->n <= r->n) {
    // distribute some elements of `r` amongst the elements of `g`
    r->n -= g->n;
    g->s += r->s;
    g->v <<= r->s;
    g->v |= r->v;
  } else {
    // forget `r` and split old `g` unto new `g` and new `r`
    repeats rt = *r;
    r->v = g->v;
    r->n = g->n - r->n;
    r->s = g->s;
    g->v <<= rt.s;
    g->v |= rt.v;
    g->s += rt.s;
    g->n = rt.n;
  }
  er(g, r);
}

unsigned long e(unsigned short onsets, unsigned short beats) {
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
    for (short i = 0; i < beats; ++i) {
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
  return result;
}