/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_BIT_H
#define XPH_BIT_H

#define BIT_MASK(x)					((1 << x) - 1)
#define SET_BITS(bits,field,shift)	((bits & BIT_MASK(field)) << shift)
#define GET_BITS(bits,field,shift)	((bits & (BIT_MASK(field) << shift)) >> shift)

#endif /* XPH_BIT_H */