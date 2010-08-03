#include "list.h"

struct list * listCreate (int length, size_t size) {
  struct list * l = xph_alloc (sizeof (struct list), "list");
  if (length < 2) {
//     logger (E_VERBOSE, "requested list with < 2 slots.");
    length = 2;
  }
  l->length = length;
  if (size <= 0) {
//     logger (E_WARN, "requested list with zero size.");
    size = 0;
  } else if (size > sizeof (void *)) {
//     logger (E_WARN, "%s: requested list with %d size, which is larger than void *. this won't work properly.", __FUNCTION__, size);
  }
  l->size = size;
  l->items = xph_alloc (size * length, "list items");
  memset (l->items, '\0', size * length);
  l->offset = 0;
  return l;
}

void listDestroy (struct list *l) {
  if (!isListEmpty (l)) {
//     logger (E_MEMINFO, "freed list %p that still has %d slot%s full.", l, listItemCount (l), listItemCount (l) == 1 ? "" : "s");
  }
  xph_free (l->items);
  xph_free (l);
}

int listAdd (struct list * l, const void * d) {
  void * v = NULL;
  if (l->offset >= l->length) {
    listResize (l);
  }
  v = (char *)l->items + l->size * l->offset++;
  memcpy (v, &d, l->size);
  return l->offset - 1;
}

bool listRemove (struct list * l, const void * d) {
  int i = 0;
  void
    * check = NULL,
    * last = NULL;
  while (i < l->offset) {
    check = (char *)l->items + l->size * i;
    if (memcmp (check, &d, l->size) == 0) {
      /* set the removed value to the final value... */
      last = (char *)l->items + l->size * --l->offset;
      memmove (check, last, l->size);
      /* ...and set the final value to NULL */
      memset (last, '\0', l->size);
      /* (if the removed value IS the final value, the memmove does nothing and
       *  the memset still wipes it.)
       */
      return TRUE;
    }
    i++;
  }
//   logger (E_WARN, "tried to remove item %p from list %p, which did not exist.", d, l);
  return FALSE;
}

void listPush (struct list * l, const void * d) {
  listAdd (l, d);
}

void * listPop (struct list * l) {
  void ** d = NULL;
  if (l->offset == 0) {
    return NULL;
  }
  d = *(void **)((char *)l->items + l->size * (l->offset - 1));
  listRemove (l, d);
  return d;
}

int listOffset (const struct list * l, const void * p) {
  void * q = NULL;
  int i = 0;
  while (i < l->offset) {
    q = listIndex (l, i);
    if (memcmp (&q, &p, l->size) == 0) {
      return i;
    }
    i++;
  }
//   logger (E_WARN, "%s: looking @ %p for something like \"%x\", but couldn't find it...", __FUNCTION__, l, p);
  return -1;
}

void * listIndex (const struct list * l, int i) {
  if (l == NULL)
    return NULL;
  return i < 0 || i >= l->offset
    ? NULL
    : *(void **)((char *)l->items + l->size * i);
}

void listResize (struct list * l) {
//   void * new = NULL;
  l->items = xph_realloc (l->items, l->size * l->length * 2);
  memset (l->items + (l->size * l->length), '\0', l->size * l->length);
//   new = xph_alloc (l->size * l->length * 2, "list items");
//   logger (E_MEMINFO, "resizing list %p to have %d slots (total %d bytes); old loc: %p; new loc: %p", l, l->length * 2, l->length * l->size * 2, l->items, new);
//   memmove (new, l->items, l->size * l->length);
//   xph_free (l->items);
//   l->items = new;
  l->length *= 2;
}

void listSort (struct list * l, int func (const void *, const void *)) {
  qsort (l->items, l->offset, l->size, func);
}

void * listSearch (const struct list * l, const void * key, int func (const void *, const void *)) {
  void ** r = bsearch (key, l->items, l->offset, l->size, func);
  return NULL == r ? NULL : *r;
}

void * list_getFirstThenDestroy (struct list * l) {
  void * r = listIndex (l, 0);
  listDestroy (l);
  return r;
}

bool isListValid (struct list ** l) {
  if (l == NULL || *l == NULL || isListEmpty (*l)) {
    return FALSE;
  }
  return TRUE;
}

bool isListEmpty (const struct list * l) {
  return listItemCount (l) == 0
   ? TRUE
   : FALSE;
}

int listItemCount (const struct list * l) {
  if (l == NULL)
    return 0;
  return l->offset;
}
