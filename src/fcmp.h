/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef FCMP_H
#define FCMP_H

#include <limits.h>
#include <math.h>
#include <stdbool.h>

bool fcmp (float a, float b);
bool fcmp_t (float a, float b, float tolerance);
/*
int fdiff (float a, float b);
*/

#endif /* FCMP_H*/