#include "cpv.h"

/*
static void vector_debug_print_final (const Vector * v, const char * f);
*/

Vector * vector_create (int c, size_t size) {
  Vector * v = malloc (sizeof (Vector));
  v->s = size;
  v->o = 0;
  v->a = c;
  v->l = calloc (c + 1, size);
  return v;
}

void vector_destroy (Vector * v) {
  if (v == NULL) {
    // yell something?
    return;
  }
  free (v->l);
  free (v);
}

void vector_sort (Vector * v, int (*f)(const void *, const void *)) {
  qsort (v->l, v->o, v->s, f);
}

void * vector_search (Vector * v, const void * k, int (*f)(const void *, const void *)) {
  void ** r = bsearch (k, v->l, v->o, v->s, f);
  return NULL == r ? NULL : *r;
}

int in_vector (const Vector * v, const void * k) {
  int
    i = 0,
    s = vector_size (v);
  //printf ("offset: %d; size: %d\n", v->o, s);
  while (i < s) {
    //printf ("%s: memcmp between i:%d and %p is %d\n", __FUNCTION__, i, k, memcmp (v->l + i * v->s, k, v->s));
    if (memcmp (v->l + i * v->s, k, v->s) == 0) {
      return i;
    }
    i++;
  }
  return -1;
}

void _vector_remove (Vector * v, void * val, size_t s) {
  int i = 0;
  if (s != v->s) {
    // yell something and/or explode
    fprintf (stderr, "invalid vector assignment: vector stores units of size %d, but something of size %d was attempted to be removed.\n", v->s, s);
    exit(7);
  }
//   vector_debug_print_final (v, __FUNCTION__);
  while (i < v->o) {
    if (memcmp (v->l + i * v->s, val, s) == 0) {
      vector_erase (v, i);
    }
    i++;
  }
//   vector_debug_print_final (v, __FUNCTION__);
}

void _vector_assign (Vector * v, int i, void * d, size_t s) {
  void * l = NULL;
  if (s != v->s) {
    // yell something and/or explode
    fprintf (stderr, "invalid vector assignment: vector stores units of size %d, but something of size %d was attempted to be placed inside.\n", v->s, s);
    exit(5);
  }
//   vector_debug_print_final (v, __FUNCTION__);
  //printf ("attempting to place something at offset %d (when a/o is %d/%d)\n", i, v->a, v->o);
  if (i < 0) {
    return;
  } else if (i >= v->a) {
    while (i >= v->a) {
      //printf ("about to try resizing (to %d)\n", v->a * 2);
      _vector_resize (v, v->a * 2, NULL);
      //printf ("done w/ resize\n");
    }
  }
  l = v->l + i * v->s;
  memcpy (l, d, v->s);
  //printf ("%s: value of %d/%p is %p\n", __FUNCTION__, i, l, *(void **)l);
  // v->o is the NUMBER OF ITEMS in the vector, not the offset of the highest in-use index; we don't actually care what i was in this context.
  // FIXME: if i as an index in the vector is already in use, then we shouldn't increase v->o.
  v->o++;
  if (v->o > v->a) {
    printf ("vector offset (%d) > allocated memory (%d). this should not be possible and is very bad. (so bad we're going to blow up everything in the hope you notice. let's assume this will be taken out after beta)\n", v->o, v->a);
    exit(1);
  }
  //printf ("done w/ %s; new size: %d/%d\n", __FUNCTION__, v->o, v->a);
//   vector_debug_print_final (v, __FUNCTION__);
}

void * _vector_at (const Vector * v, int i, size_t s) {
  if (s != v->s) {
    // yell something and/or explode
    fprintf (stderr, "invalid vector retrieval: vector stores units of size %d, but attempted to get something of size %d.\n", v->s, s);
    exit(6);
  }
  if (i < 0 || i >= v->a) {
    // this chunk of memory should always be a block of zeros as large as the target needs to zero itself out entirely. We'd normally return NULL here, except because of the macros involved in actually calling this function it's impossible to avoid dereferencing the address returned, which obviously makes it a bad idea to return NULL.
    return v->l + v->a * v->s;
  }
  //printf ("%s: got %p from %d\n", __FUNCTION__, v->l + i * v->s, v->o - 1);
  //printf ("%s: value of %d/%p is %p\n", __FUNCTION__, i, v->l + i * v->s, *(void **)(v->l + i * v->s));
  return v->l + i * v->s;
}

int vector_capacity (const Vector * v) {
  return v->a;
}

void vector_clear (Vector * v) {
  while (!vector_empty (v)) {
    vector_erase (v, v->o - 1);
  }
}

bool vector_empty (const Vector * v) {
  return (v->o == 0)
    ? TRUE
    : FALSE;
}

/* hey so this function (as well as a lot of other functions with the same
 * bug, namely a test against v->o to see if the index is out of bounds) is
 * totally not going to work right on any vector which is filled in a
 * non-sequential manner. Vaguely, what needs to be done is to alter the
 * struct so that it is possible to differentiate between "number of indices
 * filled" and "the indices which have values in them". Right now v->o is the
 * former and that means using it is not a good way to test what is the
 * highest in-use index in the vector.
 */
void vector_erase (Vector * v, int i) {
  int j = i;
  if (i < 0 || i >= v->o) {
    return;
  }
  while (j < v->o) {
    memcpy (v->l + j * v->s, v->l + (j + 1) * v->s, v->s);
    j++;
  }
  memset (v->l + v->o * v->s, '\0', v->s);
  v->o--;
}

void _vector_resize (Vector * v, int n, void * val) {
  int i = 0;
  void * l2 = NULL;
//   vector_debug_print_final (v, __FUNCTION__);
  if (n == v->a) {
    //printf ("no point in resizing\n");
    return;
  }
  if (n < v->a) {
    //printf ("shrinking vector to %d\n", n);
    v->l = realloc (v->l, (n + 1) * v->s);
    v->a = n;
    v->o = n;
    // clear final overrun entry to ensure overflow by one returns NULL
    //printf ("clearing final overrun (at %p/+%d)\n", v->l + v->a, v->a);
    memset (v->l + v->a * v->s, '\0', v->s);
  } else {
    v->a = (v->a * 2 > n) ? v->a * 2 : n;
    //printf ("growing vector to %d\n", v->a);
    l2 = realloc (v->l, (v->a + 1) * v->s);
    if (l2 == NULL) {
      fprintf (stderr, "out of memory. sorry, nothing to do but crash. hope you saved recently. :/\n");
      exit (1);
    }
    v->l = l2;
    i = v->o;
    while (i <= v->a) {
      //printf ("overwriting memory at %p (+ %d)\n", v->l + i * v->s, i);
      if (val == NULL) {
        memset (v->l + (i++ * v->s), '\0', v->s);
      } else {
        memcpy (v->l + (i++ * v->s), val, v->s);
      }
    }
  }
//   vector_debug_print_final (v, __FUNCTION__);
}

int vector_size (const Vector * v) {
  return v->o;
}

/*
static void vector_debug_print_final (const Vector * v, const char * f) {
  int l = v->s;
  printf ("%s: (%d byte%s, in steps of %d) 0X", f, v->s, (v->s == 1 ? "" : "s"), sizeof (unsigned short));
  while (l > 0) {
    printf ("%02hx", *(const unsigned short *)(v->l + v->a * v->s + (v->s - l)));
    l -= sizeof (unsigned short);
  }
  printf ("\n");
}
*/