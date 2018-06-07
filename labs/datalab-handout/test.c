#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

int main() {
#if 0
    int x = -1;
    int n = 1;
    int mask = ~(0x1 << 31 >> (32 + ~n + 1) << 1);
    int a = x & mask; //n的部分
    int b = x & ~mask & ~(1 << 31);
    int c = x & ~mask & (1 << 31);
    int d = x & (1 << n >> 1); //n部分的符号
    printf("%x\n", mask);
    printf("%x\n", a);
    printf("%x\n", b);
    printf("%x\n", c);
    printf("%x\n", d);
    printf("%x", !b && !(d ^ !!c));
#endif
# if 0
    int x = -2147483648, y = 0x7fffffff;
    int sub1 = x + (~y+1);
    int sub2 = y + (~x+1);
    printf("%x\n", sub1);
    printf("%x\n", sub2);
    printf("%x\n", !!((1 << 31) & sub1)| !((1 << 31) & sub2) );
# endif
    int x = 1 << 31;
    printf("%x, %x",x, x-1);

}