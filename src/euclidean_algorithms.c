#include <stdio.h>

unsigned long e(unsigned short onsets, unsigned short beats) {
  if (onsets > beats) {
    fprintf(stderr,
            "number of onsets (%d) can't be larger than number of beats (%d)",
            onsets, beats);
            return -1;
  }
  // er(g, r)
  return 0b10010110;
}

/*
e(7,8)
g=(7,1) r=(1,0)
g=(1,10) r=(6,1)
g=(1,101) r=(5,1)
g=(1,1011) r=(4,1)
g=(1,10111) r=(3,1)
g=(1,101111) r=(2,1)
g=(1,1011111) r=(1,1)
e=   10111111

e(4,6)
g=(4,1) r=(2,0)
g=(2,10) r=(2,1)
g=(2,101) r=(0,_)
e=   101101

e(3,8)
g=(3,1) r=(5,0)
g=(3,10) r=(2,0)
g=(2,100) r=(1,10)
e=   10010010
*/