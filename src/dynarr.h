#ifndef XPH_DYNARR_H
#define XPH_DYNARR_H

#include <assert.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "bool.h"
#include "xph_log.h"

typedef struct dynarr * Dynarr;
typedef struct dyn_iterator * DynIterator;

Dynarr dynarr_create (int indices, size_t size);
void dynarr_destroy (Dynarr da);

/*
typedef union
{
	unsigned char
		c[32];
	float
		f;
	double
		d;
} DYNARG;
*/

/* NOTE: this uses some kind of type punning to store arbitrary types. This
 * type punning fails in certain cases. Specifically, when the value being
 * stored is larger than the size of a pointer, or if the value being stored
 * is a float. (If doubles happen to be <= pointers in some hypothetical
 * architecture, then they will work here.) This is annoying but not a
 * critical flaw, since 99% of the time these arrays are used for chars, ints,
 * or pointers.
 * (i don't know if there's a better way to do things; a void * argument is
 * less hair-raisingly undefined, but it's important that it's possible to
 * call the function as dynarr_assign (foo, 0, 92) or whatever, instead of
 * having to store a value and then cast it, and even then that raises
 * compiler errors from impossible casting.)
 * (an option i'm considering is using a union as the function argument, but i
 * don't know if that would work or is even allowed; i suspect it would
 * require casting in either case)
 */
int dynarr_assign (Dynarr da, int index, ...);
int dynarrInsertInPlace (Dynarr da, int index, ...);
int dynarr_push (Dynarr da, ...);
char * dynarr_at (const Dynarr da, int index);
char * dynarr_pop (Dynarr da);
char * dynarr_front (Dynarr da);
char * dynarr_back (Dynarr da);

void dynarr_unset (Dynarr da, int index);
void dynarr_clear (Dynarr da);
void dynarr_wipe (Dynarr da, void free_func (void *));

void dynarr_condense (Dynarr da);
void dynarr_sort (Dynarr da, int (*sort) (const void *, const void *));
void dynarrSortFinal (Dynarr da, int (*sort) (const void *, const void *), int amount);
char * dynarr_search (const Dynarr da, int (*search) (const void *, const void *), ...);

void dynarr_map (Dynarr da, void map_func (void *));

void dynarr_remove_condense (Dynarr da, ...);

int in_dynarr (const Dynarr da, ...);
int dynarr_size (const Dynarr da);
int dynarr_capacity (const Dynarr da);
bool dynarr_isEmpty (const Dynarr da);

DynIterator dynIterator_create (const Dynarr da);
void dynIterator_destroy (DynIterator it);

char * dynIterator_next (DynIterator it);
int dynIterator_nextIndex (DynIterator it);
int dynIterator_lastIndex (const DynIterator it);
void dynIterator_reset (DynIterator it);
bool dynIterator_done (const DynIterator it);

#endif /* XPH_DYNARR_H */