#include <stdio.h>
#include <string.h>

unsigned test() {
    unsigned x = 0x59b997fa;
    return x;
}
int main() {
    unsigned a = 0xc7c78948;
    unsigned b = 0x59b997fa;
    printf("%u, %u", a-0x3c3876b8, b);
    return 0;
}