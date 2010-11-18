#include "cpv.h"

#include "xph_memory.h"
/*
static void vector_debug_print_final (const Vector * v, const char * f);
*/
static bool index_used (const Vector * v, int i);
static void index_set_used (Vector * v, int i);
static void index_set_free (Vector * v, int i);

static bool index_used (const Vector * v, int i) {
  int
    k,
    l;
  if (v->order == VECTOR_SEQUENTIAL) {
    return i < v->o;
  }
  assert (v->f != NULL);
  k = i / CHAR_BIT;
  l = i % CHAR_BIT;
  return (v->f[k] & (0x01 << l))
    ? TRUE
    : FALSE;
}

static void index_set_used (Vector * v, int i) {
  int
    k, l,
    m, n;
  //printf ("%s (%p, %d)... (size: %d)\n", __FUNCTION__, v, i, v->o);
  if (v->order == VECTOR_SEQUENTIAL && i > v->o) {
    //printf ("%s: the vector is now non-sequential! (%d byte%s of index buffer)\n", __FUNCTION__, v->a / CHAR_BIT + 1, (v->a / CHAR_BIT + 1 == 1) ? "" : "s");
    v->order = VECTOR_NONSEQUENTIAL;
    v->f = calloc (1, v->a / CHAR_BIT + 1);
    k = v->o / CHAR_BIT;
    l = v->o % CHAR_BIT;
    m = n = 0;
    while (m < k) {
      //printf ("%s: setting indices %d through %d as used\n", __FUNCTION__, m * CHAR_BIT, (m + 1) * CHAR_BIT);
      v->f[m] = -1;
      m++;
    }
    while (n < l) {
      //printf ("%s: setting index %d as used\n", __FUNCTION__, m * CHAR_BIT + n);
      v->f[m] |= (0x01 << n);
      n++;
    }
  } else if (v->order == VECTOR_SEQUENTIAL) {
    return;
  }
  assert (v->f != NULL);
  k = i / CHAR_BIT;
  l = i % CHAR_BIT;
  //printf ("%s: setting index %d as used ([%d]:%d)\n", __FUNCTION__, k * CHAR_BIT + l, k, l);
  v->f[k] |= (0x01 << l);
  assert (index_used (v, i));
}

static void index_set_free (Vector * v, int i) {
  int
    k, l,
    m, n;
  if (v->order == VECTOR_SEQUENTIAL && i != v->o) {
    //printf ("%s: the vector is now non-sequential!\n", __FUNCTION__);
    v->order = VECTOR_NONSEQUENTIAL;
    v->f = calloc (1, v->a / CHAR_BIT + 1);
    k = v->o / CHAR_BIT;
    l = v->o % CHAR_BIT;
    m = n = 0;
    while (m < k) {
      //printf ("%s: setting indices %d through %d as used\n", __FUNCTION__, m * CHAR_BIT, (m + 1) * CHAR_BIT);
      v->f[m] = -1;
      m++;
    }
    while (n < l) {
      //printf ("%s: setting index %d as used\n", __FUNCTION__, m * CHAR_BIT + n);
      v->f[m] |= (0x01 << n);
      n++;
    }
  } else if (v->order == VECTOR_SEQUENTIAL) {
    return;
  }
  assert (v->f != NULL);
  k = i / CHAR_BIT;
  l = i % CHAR_BIT;
  v->f[k] &= ~(0x01 << l);
  assert (!index_used (v, i));
}


int vector_index_last (const Vector * v) {
  int
    k = v->a / CHAR_BIT + 1,
    l = 0;
  if (v->order == VECTOR_SEQUENTIAL) {
    //printf ("%s (%p): vector is sequential with %d indices\n", __FUNCTION__, v, v->o);
    return v->o ? v->o - 1 : -1;
  }
  while (k > 0) {
    k--;
    if (v->f[k]) {
      //printf ("%s (%p): [%d] isn't empty\n", __FUNCTION__, v, k);
      l = CHAR_BIT;
      while (l-- > 0) {
        if (v->f[k] & (0x01 << l)) {
          //printf ("%s (%p): final in-use index: [%d]:%d (%d)\n", __FUNCTION__, v, k, l, k * CHAR_BIT + l);
          return k * CHAR_BIT + l;
        }
      }
    }
  }
  //printf ("%s (%p): vector is empty\n", __FUNCTION__, v);
  return -1;
}

int vector_index_first (const Vector * v) {
  int
    k = 0,
    l = 0;
  if (v->order == VECTOR_SEQUENTIAL) {
    return v->o ? 0 : -1;
  }
  while (k < v->a / CHAR_BIT + 1) {
    if (v->f[k]) {
      l = 0;
      while (l < CHAR_BIT) {
        if (v->f[k] & (0x01 << l)) {
          //printf ("first in-use index: %d\n", k * CHAR_BIT + l);
          return k * CHAR_BIT + l;
        }
        l++;
      }
    }
    k++;
  }
  return -1;
}

Vector * vector_create (int c, size_t size) {
  Vector * v = xph_alloc (sizeof (Vector));
  v->s = size;
  v->o = 0;
  v->a = c;
  v->l = calloc (c + 1, size);
  v->order = VECTOR_SEQUENTIAL;
  v->f = NULL;
  return v;
}

void vector_wipe (Vector * v, void free_func (void *)) {
  void * p = NULL;
  if (free_func != NULL && v->s != sizeof (void *)) {
    fprintf (stderr, "%s called with free function on a vector that doesn't store pointers.\n", __FUNCTION__);
    exit (8); // yes let's keep making up arbitrary exit codes, good idea
  }
  while (vector_pop_back (p, v)) {
    if (free_func != NULL) {
      free_func (p);
    }
  }
}

void vector_destroy (Vector * v) {
  if (v == NULL) {
    return;
  }
  if (v->f != NULL) {
    free (v->f);
  }
  free (v->l);
  xph_free (v);
}

void vector_sort (Vector * v, int (*f)(const void *, const void *)) {
  if (v->order == VECTOR_NONSEQUENTIAL) {
    printf ("%s: cannot sort non-sequential vector\n", __FUNCTION__);
    return;
  }
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
  bool r = FALSE;
  if (s != v->s) {
    fprintf (stderr, "invalid vector assignment: vector stores units of size %d, but something of size %d was attempted to be placed inside.\n", v->s, s);
    exit(5);
  }
  if (i < 0) {
    return;
  } else if (i >= v->a) {
    while (i >= v->a) {
      _vector_resize (v, v->a * 2, NULL);
    }
  }
  //printf ("%s: assigning to index %d\n", __FUNCTION__, i);
  memcpy (v->l + i * v->s, d, v->s);
  if (!index_used (v, i)) {
    r = TRUE;
  }
  index_set_used (v, i);
  if (r == TRUE) {
    //printf ("%s: index %d unused; setting used\n", __FUNCTION__, i);
    v->o++;
  }
  if (v->o > v->a) {
    printf ("vector size (%d) > allocated memory (%d). this should not be possible and is very bad. (so bad we're going to blow up everything in the hope you notice. let's assume this will be taken out after beta)\n", v->o, v->a);
    exit(1);
  }
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

void vector_erase (Vector * v, int i) {
  int j = i;
  if (i < 0 ||
      (v->order == VECTOR_SEQUENTIAL && i >= v->o) ||
      (v->order == VECTOR_NONSEQUENTIAL && index_used (v, i) == FALSE)) {
    return;
  }
  if (v->order == VECTOR_SEQUENTIAL) {
    while (j < v->o) {
      memcpy (v->l + j * v->s, v->l + (j + 1) * v->s, v->s);
      j++;
    }
    memset (v->l + v->o * v->s, '\0', v->s);
  } else {
    memcpy (v->l + j * v->s, v->l + v->a * v->s, v->s);
    index_set_free (v, i);
  }
  v->o--;
}

void _vector_resize (Vector * v, int n, void * val) {
  int
    i = 0,
    oa = v->a;
  int j, k, mask = 0;
  void * l2 = NULL;
  char * f2 = NULL;
  if (n == v->a) {
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
    l2 = realloc (v->l, (v->a + 1) * v->s);
    if (l2 == NULL) {
      fprintf (stderr, "out of memory. sorry, nothing to do but crash. hope you saved recently. :/\n");
      exit (1);
    }/* else if (v->l != l2) {
      free (v->l);
    }*/
    // if I try to if (v->l != l2) free (v->l) I get a segfault...?
    v->l = l2;
    i = vector_index_last (v) + 1;
    while (i <= v->a) {
      //printf ("overwriting memory at %p (+ %d)\n", v->l + i * v->s, i);
      if (val == NULL) {
        memset (v->l + (i++ * v->s), '\0', v->s);
      } else {
        memcpy (v->l + (i++ * v->s), val, v->s);
      }
    }
    if (v->order == VECTOR_NONSEQUENTIAL) {
      f2 = realloc (v->f, v->a / CHAR_BIT + 1);
      if (f2 == NULL) {
        fprintf (stderr, "out of memory. sorry, nothing to do but crash. hope you saved recently. :/\n");
        exit (1);
      }/* else if (f2 != v->f) {
        free (v->f);
      }*/
      v->f = f2;
      j = oa / CHAR_BIT;
      k = oa % CHAR_BIT;
      i = 0;
      while (i <= k) {
        mask |= 0x01 << i;
        i++;
      }
      v->f[j] &= mask;
      k = 0;
      while (j < v->a / CHAR_BIT + 1) {
        v->f[j] = 0;
        j++;
      }
    }
  }
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