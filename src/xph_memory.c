#include "xph_memory.h"

#ifdef MEM_DEBUG
static struct mlist * mlist = NULL;

void * _xph_alloc (size_t size, char * exp) {
  struct mdata * n = NULL;
  void * a = NULL;
  int len = 0;
  //printf ("%s (%d, \"%s\")...\n", __FUNCTION__, size, exp);
  if (mlist == NULL) {
    mlist = calloc (1, sizeof (struct mlist));
    mlist->o = 0;
    mlist->l = 32;
    mlist->a = calloc (mlist->l, sizeof (struct mdata *));
  }
  n = calloc (1, sizeof (struct mdata));
  len = strlen (exp);
  n->exp = calloc (1, len + 1);
  strncpy (n->exp, exp, len);
  n->size = size;
  n->address = calloc (1, size);
  //printf ("  ... \"%s\", %d word%s at %p\n", n->exp, n->size, (n->size == 1 ? "" : "s"), n->address);
  if (mlist->o >= mlist->l) {
    //printf ("resizing mlist\n");
    a = calloc (sizeof (struct mdata *), mlist->l * 2);
    memcpy (a, mlist->a, sizeof (struct mdata *) * mlist->l);
    mlist->l *= 2;
    free (mlist->a);
    mlist->a = a;
    a = NULL;
/*
    printf ("new realloc size: %d\n", sizeof (struct mdata *) * mlist->l);
    a = realloc (mlist->a, sizeof (struct mdata *) * mlist->l);
    if (a == NULL) {
      fprintf (stderr, "Out of memory (tried to allocate %d bytes). There's really nothing we can do here except crash, sorry. Hope you saved recently! :D", sizeof (struct mdata *) * mlist->l);
      exit (1);
    }
    //printf ("realloc: old space: %p; new space: %p\n", mlist->a, a);
    //memset (mlist->a + mlist->l, '\0', mlist->l);
    //free (mlist->a); lol no, don't do this
    mlist->a = a;
    a = NULL;
*/
  }
  mlist->a[mlist->o++] = n;
  qsort (mlist->a, mlist->o, sizeof (struct mdata *), memory_sort);
  return n->address;
}

void * xph_realloc (void * d, size_t size) {
  //printf ("%s (%p, %d)...\n", __FUNCTION__, d, size);
  struct mdata
    ** k = NULL,
    * j = NULL;
  void * a = NULL;
  assert (mlist != NULL);
  k = bsearch (d, mlist->a, mlist->o, sizeof (struct mdata *), memory_search);
  if (k == NULL) {
    return d;
  }
  j = *k;
  a = realloc (j->address, size);
  if (a == NULL) {
    fprintf (stderr, "Out of memory (tried to allocate %d bytes, %d absolutely). There's really nothing we can do here except crash, sorry. Hope you saved recently! :D\n", size - j->size, size);
    exit (1);
  }
  j->address = a;
  qsort (mlist->a, mlist->o, sizeof (struct mdata *), memory_sort);
  if (size > j->size) {
    memset (j->address + j->size, '\0', size - j->size);
  }
  j->size = size;
  //printf ("...%s ()\n", __FUNCTION__);
  return j->address;
}

void xph_free (void * d) {
  //printf ("%s (%p)...\n", __FUNCTION__, d);
  struct mdata
    ** last = NULL,
    ** k = NULL;
  assert (mlist != NULL);
  k = bsearch (d, mlist->a, mlist->o, sizeof (struct mdata *), memory_search);
  if (k == NULL) {
    //fprintf (stderr, "Attempted to free %p, which is not allocated.\n", d);
    return;
  }
  free ((*k)->address);
  free ((*k)->exp);
  free (*k);
  last = &mlist->a[--mlist->o];
  *k = *last;
  memset (last, '\0', sizeof (struct mdata *));
  qsort (mlist->a, mlist->o, sizeof (struct mdata *), memory_sort);
  //printf ("...%s ()\n", __FUNCTION__);
}

void xph_audit () {
  int i = 0;
  struct mdata * k = NULL;
  if (mlist->o == 0) {
    printf ("No memory allocated.\n");
    return;
  }
  printf ("%d address%s allocated.\n", mlist->o, mlist->o == 1 ? "" : "es");
  while (i < mlist->o) {
    k = mlist->a[i++];
    printf ("%4d bytes @ %p, \"%s\"\n", k->size, k->address, k->exp);
  }
}

struct mdata * xph_isallocated (void * d) {
  //printf ("%s (%p)...\n", __FUNCTION__, d);
  struct mdata ** k = bsearch (d, mlist->a, mlist->o, sizeof (struct mdata *), memory_search);
  if (k == NULL) {
    //printf ("...%s () [early]\n", __FUNCTION__);
    return NULL;
  }
  //printf ("...%s ()\n", __FUNCTION__);
  return *k;
}

int memory_sort (const void * a, const void * b) {
  return
    (char *)((*(struct mdata **)a)->address) -
    (char *)((*(struct mdata **)b)->address);
}

int memory_search (const void * key, const void * datum) {
  return
    (char *)key -
    (char *)((*(struct mdata **)datum)->address);
}

#endif /* MEM_DEBUG */