#include <stdio.h>
#include <stdlib.h>

int main() {
    for (int i = 0; i < 1024; ++i) {
        //unsigned int y = 0x30a080 + 4 * i;
        unsigned int y = 0x34a080 + 4 * i;
        if ((y >> 5U) % 32U == 4) {
            printf("row = %d, column = %d\n", i/32, i%32);
        }
    }

    return 0;
}