#include <stdio.h>

int main() {
   int r = e();
   if (r == 4) {
      printf("Received the expected result (4)");
   } else {
      printf("Received a wrong result (%d)", r);
      return 1;
   }
   return 0;
}