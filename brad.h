#ifndef __BRAD_H__
#define __BRAD_H__

/** Binary scaling library
 *
 * There are B8 binary scaled ints.  That means the 8 lowest bits are
 * the fractional part.  To convert to a float you'd just divide by 256.
 *
 * The trig functions use brads (Binary Radians).  There are 128 brads
 * in 2pi radians.  One brad is about 1.4 degrees.  Using brads makes
 * trig really fast on a binary computer, and reallier faster on a
 * binary computer without an FPU.
 *
 * You must be careful not to overflow your bs_t.  For instance, I wrote
 * this library for a game with a 2^9-pixel-wide playfield.  That's
 * 2^17 B8, but I also needed to do distance calculations, which
 * requires squaring things.  2^34 is obviously too big to represent in
 * 32 bits.
 *
 * C's type system leaves a lot to be desired when it comes to making
 * sure you're not mixing these things up with normal ints.  Be careful!
 */

/*  Just to make it clear what scale these functions are dealing with */
typedef struct {
  int v;
} bs_t;
typedef int brad_t;

/* If you change this, you must also change the lookup tables in
 * brad.c.  Don't change this. */
#define BINARY_SCALE 8
#define BS_DENOMINATOR 1 << BINARY_SCALE

#define bs_to_int(n) ((n) >> BINARY_SCALE)
#define bs_of_int(i) ((i) << BINARY_SCALE)

bs_t bs_cos(brad_t angle);
bs_t bs_sin(brad_t angle);
bs_t bs_tan(brad_t angle);

brad_t bs_acos(bs_t angle);
brad_t bs_asin(bs_t angle);

bs_t bs_mul(bs_t a, bs_t b);
bs_t bs_div(bs_t a, bs_t b);

bs_t bs_approx_dist(bs_t dx, bs_t dy);

#endif /* __BRAD_H__ */
