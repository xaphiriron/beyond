/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "fcmp.h"

bool fcmp (float a, float b) {
  return fcmp_t (a, b, 0.0001);
}

bool fcmp_t (float a, float b, float tolerance) {
  float
    diff = fabs (a - b),
    e = 0,
    fa = 0,
    fb = 0;
/* THIS CODE IS REDACTED; SEE NOTE BELOW
  if (sizeof (int) == sizeof (float)) {
    return (fdiff (a, b) <= tolerance)
      ? true
      : false;
  }
*/
  if (a == b || diff < tolerance) {
    return true;
  }
  fa = fabs (a);
  fb = fabs (b);
  if (fa < fb) {
    e = diff / fb;
  } else {
    e = diff / fa;
  }
  return (e <= tolerance)
    ? true
    : false;
}

/* this code uses the Dark Arts of type-punning and relies on specific results
 * occurring from an explicitly undefined operation. One might assume that the
 * *(int*)&float cast allows you to read float's bits as an integer value; but
 * that is not the case always, and as I have found out, it is not a technique
 * you want to rely on unless you enjoy the results changing depending on your
 * compiler edition, your CPU, the number of printf debugging statements used,
 * the system time, the local weather and humidity, and the phase of the moon.
 * READ THIS AND BEWARE, FOR IF THESE COMMENTS ARE EVER UNDONE THE GREAT BEAST
 * SHALL AWAKEN AND LOPE THROUGH THE CODE LEAVING NON-DETERMINISM IN ITS WAKE!
 */
/*
int fdiff (float a, float b) {
  int ai, bi;
  if (sizeof (int) != sizeof (float)) {
    return INT_MAX;
  }
  ai = *(int *)&a;
  bi = *(int *)&b;
  // this constant assumes both int and float are 32 bit and using two's-compliment
  if (ai < 0)
    ai = 0x80000000 - ai;
  if (bi < 0)
    bi = 0x80000000 - bi;
  return abs (ai - bi);
}
*/