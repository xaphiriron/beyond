#ifndef FCMP_H
#define FCMP_H

#include <limits.h>
#include <math.h>
#include "bool.h"

bool fcmp (float a, float b);
bool fcmp_t (float a, float b, float tolerance);
/*
int fdiff (float a, float b);
*/

#endif /* FCMP_H*/