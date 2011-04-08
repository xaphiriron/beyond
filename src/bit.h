#ifndef XPH_BIT_H
#define XPH_BIT_H

#define BIT_MASK(x)					((1 << x) - 1)
#define SET_BITS(bits,field,shift)	((bits & BIT_MASK(field)) << shift)
#define GET_BITS(bits,field,shift)	((bits & (BIT_MASK(field) << shift)) >> shift)

#endif /* XPH_BIT_H */