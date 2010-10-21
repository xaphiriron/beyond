#ifndef XPH_MEMORY_H
#define XPH_MEMORY_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define new(TYPE, NAME)		TYPE * NAME = xph_alloc (sizeof (TYPE), #TYPE)

#ifdef MEM_DEBUG

/*
extern void free (void *);
extern void * realloc (void *, size_t);
extern void qsort (void *, size_t, size_t,  int (*)(const void *, const void *));
extern void * bsearch (const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
extern void exit (int);
*/

struct mdata {
  void * address;
  size_t size;
  char * exp;
};

struct mlist {
  struct mdata ** a;
  int o, l;
};

#define xph_alloc(a, b)		_xph_alloc(a, b)
void * _xph_alloc (size_t size, char * exp);
void * xph_realloc (void * d, size_t size);
void xph_free (void * d);
void xph_audit ();
struct mdata * xph_isallocated (void * d);

int memory_sort (const void * a, const void * b);
int memory_search (const void * key, const void * datum);
#else /* MEM_DEBUG */
#define xph_alloc(a, b)		malloc(a)
#define xph_realloc(a, b)	realloc(a, b)
#define xph_free(a)		free(a)
#endif /* MEM_DEBUG */

#endif /* XPH_MEMORY_H */