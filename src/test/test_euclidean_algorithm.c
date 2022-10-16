#include <stdio.h>
#include "../euclidean_algorithms.h"

int main() {
   unsigned long r = e(3, 8);
   if (r == 0b10010010) {
      printf("Received the expected result (x..x..x.)");
   } else {
      printf("Received a wrong result (%8lx)", r);
      return 1;
   }
   return 0;
}