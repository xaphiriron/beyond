#ifndef CPV_H
#define CPV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bool.h"

typedef struct cp_vector {
  int a, o;
  size_t s;
  void * l;
} Vector;


Vector * vector_create (int c, size_t size);
void vector_destroy (Vector * v);

void vector_sort (Vector * v, int (*f)(const void *, const void *));
void * vector_search (Vector * v, const void * k, int (*f)(const void *, const void *));

/* let's just assume there's no good way to expand this into the kind of thing you can use in a loop and be done with it. :/
#define		in_vector(v, i)		\
		({typeof (i) _i = i;	\
		_in_vector (v, &_i, sizeof (i));})
*/
int in_vector (const Vector * v, const void * k);

// remove value "val" from vector "v" if it exists
#define		vector_remove(v, i)	\
		{typeof (i) _i = i;	\
		_vector_remove (v, &_i, sizeof(i));}
void _vector_remove (Vector * v, void * val, size_t s);



#define		vector_assign(v, i, c)	\
		({typeof (c) _c = c; \
		_vector_assign (v, i, &_c, sizeof (typeof (_c))); \
		})
void _vector_assign (Vector * v, int i, void * d, size_t s);


#define		vector_at(f, v, i)	\
		(f = *(typeof (f) *)_vector_at (v, i, sizeof(typeof(f))))
void * _vector_at (const Vector * v, int i, size_t s);

/*

#define		vector_at(f, v, i)	\
		{void * _r = _vector_at (v, i, sizeof (f)); \
		if (_r == NULL) { \
		  memset (&f, '\0', sizeof (f)); \
		} else { \
		  f = *(typeof (f) *)_r; \
		}}
*/

#define		vector_back(f, v)	\
		(f = *(typeof (f) *)_vector_at (v, v->o - 1, sizeof (f)))


int vector_capacity (const Vector * v);
void vector_clear (Vector * v);
bool vector_empty (const Vector * v);
// remove the value at index i from vector v, retaining the order of the vector's elements
void vector_erase (Vector * v, int i);


#define		vector_front(f, v)	\
		(f = *(typeof (f) *)_vector_at (v, 0, sizeof (f)))

// This macro has problems with doubly-evaluating f because of the final entry (which is needed to make vector_pop_back "return" f, for use in loops)
#define		vector_pop_back(f, v)	\
		(f = *(typeof (f) *)_vector_at ((v), (v)->o - 1, sizeof (f)), \
		vector_erase (v, (v)->o - 1), \
		f)

#define		vector_push_back(v, c)	\
		{typeof (c) _c = (c);	\
		_vector_assign (v, (v)->o, &_c, sizeof (typeof (_c)));}

#define vector_resize(v, n, val)	\
	{typeof (val) _val = (val);	\
	 _vector_resize (v, n, &_val)}
void _vector_resize (Vector * v, int n, void * val);


int vector_size (const Vector * v);



#endif /* CPV_H */