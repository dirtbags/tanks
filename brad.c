#include <stdlib.h>
#include "brad.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* Approximate vector length
 *
 * http://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
 *
 * (1007/1024) * 256 == 251.75
 *  (441/1024) * 256 == 110.25
 */
bs_t
bs_approx_dist(bs_t dx, bs_t dy)
{
  bs_t x = abs(dx);
  bs_t y = abs(dy);

  return (252 * max(x, y)) + (110 * min(x, y));
}


static bs_t bs_cos_table[] = {
  256, 256, 256, 255, 255, 254, 253, 252,
  251, 250, 248, 247, 245, 243, 241, 239,
  237, 234, 231, 229, 226, 223, 220, 216,
  213, 209, 206, 202, 198, 194, 190, 185,
  181, 177, 172, 167, 162, 157, 152, 147,
  142, 137, 132, 126, 121, 115, 109, 104,
   98,  92,  86,  80,  74,  68,  62,  56,
   50,  44,  38,  31,  25,  19,  13,   6,
    0
};

bs_t
bs_cos(brad_t angle)
{
  brad_t cos;
  bs_t   a;

  a = abs(angle) % 128;
  if (a > 64) {
    a = 128 - a;
  }
  cos = bs_cos_table[a];
  a = abs(angle) % 256;
  if ((a > 64) && (a < 192)) {
    cos = -cos;
  }
  return cos;
}

bs_t
bs_sin(brad_t angle)
{
  return bs_cos(64 - angle);
}

bs_t
bs_tan(brad_t angle)
{
  return bs_sin(angle) / bs_cos(angle);
}


static brad_t bs_asin_table[] = {
   0,   0,   1,   1,   1,   2,   2,   2,
   3,   3,   3,   4,   4,   4,   4,   5,
   5,   5,   6,   6,   6,   7,   7,   7,
   8,   8,   8,   9,   9,   9,  10,  10,
  10,  11,  11,  11,  11,  12,  12,  12,
  13,  13,  13,  14,  14,  14,  15,  15,
  15,  16,  16,  16,  17,  17,  17,  18,
  18,  18,  19,  19,  19,  20,  20,  20,
  21,  21,  21,  22,  22,  22,  23,  23,
  23,  24,  24,  24,  25,  25,  25,  26,
  26,  26,  27,  27,  27,  28,  28,  28,
  29,  29,  29,  30,  30,  30,  31,  31,
  31,  32,  32,  32,  33,  33,  33,  34,
  34,  34,  35,  35,  35,  36,  36,  37,
  37,  37,  38,  38,  38,  39,  39,  39,
  40,  40,  40,  41,  41,  42,  42,  42,
  43,  43,  43,  44,  44,  45,  45,  45,
  46,  46,  46,  47,  47,  48,  48,  48,
  49,  49,  49,  50,  50,  51,  51,  51,
  52,  52,  53,  53,  53,  54,  54,  55,
  55,  55,  56,  56,  57,  57,  57,  58,
  58,  59,  59,  60,  60,  60,  61,  61,
  62,  62,  63,  63,  64,  64,  64,  65,
  65,  66,  66,  67,  67,  68,  68,  69,
  69,  70,  70,  71,  71,  72,  72,  73,
  73,  74,  74,  75,  75,  76,  76,  77,
  77,  78,  78,  79,  80,  80,  81,  81,
  82,  82,  83,  84,  84,  85,  86,  86,
  87,  87,  88,  89,  90,  90,  91,  92,
  92,  93,  94,  95,  96,  96,  97,  98,
  99, 100, 101, 102, 103, 104, 105, 106,
 108, 109, 110, 112, 114, 116, 118, 121,
 128
};

brad_t
bs_asin(bs_t n)
{
  bs_t a = abs(n);
  brad_t ret;

  if (a > 256) {
    return 0;
  }
  ret = bs_asin_table[a];
  if (n < 0) {
    ret = -ret;
  }
  return ret;
}

brad_t
bs_acos(bs_t n)
{
  return 128 - bs_asin(n);
}

brad_t
bs_atan2(bs_t y, bs_t x)
{
  bs_t   r = bs_approx_dist(x, y);
  brad_t t;

  t = bs_acos(x / r);
  if (y < 0) {
    t = 256 - t;
  }
  return t;
}


#include <math.h>
#include <stdio.h>
#define PI 3.14159265358979323846
int
main()
{
  int   i, j;
  float f;
  int   t;

  for (i = 0; i < 257; i += 1) {
    for (j = 0; j < 257; j += 1) {
      f = sqrt(i*i + j*j);
      t = bs_approx_dist(i, j);
      printf("%d  %d  %d\n", i, (int)round(f*256)/256, t/256);
    }
  }
  return 0;
}
