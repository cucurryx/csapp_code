#line 141 "./bits.c"
int bitAnd(int x, int y) {
  return ~(~x | ~y);
#line 31 "<command-line>"
#include "/usr/include/stdc-predef.h"
#line 143 "./bits.c"
}
#line 153
int getByte(int x, int n) {
  return (x >>( n << 3) & 0xff);
}
#line 164
int logicalShift(int x, int n) {

  int mask=  1 << 31;
  return x >> n & ~((mask >> n) << 1);
#line 171
}
#line 179
int bitCount(int x) {
  return 0;
}
#line 189
int bang(int x) {


  int a=  x |( x + ~(1 << 31));
  return ~a >> 31 & 0x1;
}
#line 201
int tmin(void) {
  return (1 << 31);
}
#line 213
int fitsBits(int x, int n) {
  int t=  1 <<( n + ~1 + 1);return 4L;

}
#line 225
int divpwr2(int x, int n) {
#line 229
  int mask=((  1 << 31) & x) >> 31;
  x =((( 1 << n) +( ~1) + 1) & mask) + x;
  return x >> n;
}
#line 240
int negate(int x) {
  return ~x + 1;
}
#line 250
int isPositive(int x) {
  return !(x&(1 << 31)) & !!x;
}
#line 260
int isLessOrEqual(int x, int y) {
  return 0;
}
#line 270
int ilog2(int x) {
  return 1;
}
#line 284
unsigned float_neg(unsigned uf) {
  int mask=(  1 << 31);
  int t=(  uf | mask);
  t = t >> 23;
  if (uf << 9 && !~t) {
    return uf;
  }
  return uf ^ mask;
}
#line 302
unsigned float_i2f(int x) {
  return 2;
}
#line 316
unsigned float_twice(unsigned uf) {
  return 2;
}
