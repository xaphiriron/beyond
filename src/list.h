#ifndef LIST_H
#define LIST_H

#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xph_memory.h"
// #include "log.h"
#include "bool.h"

/* all this code is outdated and untested; unit tests need to be written for, among many other things, list adding and removing, list enqueuing and dequeuing, list pushing and popping, and list sorting and searching.
 */

struct list {
  void * items;
  int offset, length;
  size_t size;
};

/* FIXME: all these are fundamentally flawed-- they return sizeof (void *) as values, regardless of what the list->size is. this isn't much of a practical problem provided you only use them to store values with a size <= sizeof (void *) (so you should use this to store pointers to structures, not the structures themselves)
 */

struct list * listCreate (int length, size_t size);
void listDestroy (struct list *l);

int listAdd (struct list * l, const void * d);	/* add an item to the end of the list, return offset */
bool listRemove (struct list * l, const void * d);	/* remove an arbitrary item*/

void listPush (struct list * l, const void * d);	/* alias of listAdd */
void * listPop (struct list * l);		/* returns and removes the final list entry */

int listOffset (const struct list * l, const void * p);
void * listIndex (const struct list * l, int o);
void listResize (struct list * l);

void listSort (struct list * l, int func (const void *, const void *));
void * listSearch (const struct list * l, const void * key, int func (const void *, const void *));

void * list_getFirstThenDestroy (struct list *);

bool isListValid (struct list ** l);

bool isListEmpty (const struct list * l);
int listItemCount (const struct list * l);
#endif /* LIST_H */