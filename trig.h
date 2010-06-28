#ifndef __TRIG_H__
#define __TRIG_H__

/** Trigonometry in degrees
 *
 * It's just a library to do discrete(-ish) trig, using degrees.  It
 * uses a lookup table to make sin, cos, and tan really fast.
 * Everything is as slow as normal :)
 */

/*  Just to make it clear what scale these functions are dealing with */
#define PI 3.14159265358979323846

#define rad2deg(rad) ((int)(rad * 180 / PI))
#define deg2rad(deg) ((float)(deg * PI / 180))

float trig_cos(int angle);
float trig_sin(int angle);
float trig_atan(int angle);

int trig_acos(float angle);
int trig_asin(float angle);
int trig_atan2(float y, float x);

#endif /* __TRIG_H__ */
