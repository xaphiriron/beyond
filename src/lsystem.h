#ifndef LSYSTEM_H
#define LSYSTEM_H

#include <string.h>
#include "dynarr.h"
#include "xph_memory.h"

typedef struct lsystem {
  Dynarr p;
} LSYSTEM;

typedef struct production {
  char l;
  Dynarr exp;
} PRODUCTION;

LSYSTEM * lsystem_create ();
void lsystem_destroy (LSYSTEM * l);
PRODUCTION * production_create (const char l, const char * exp);
void production_destroy (PRODUCTION * p);
void production_addRule (PRODUCTION * p, const char * exp);

bool lsystem_isDefined (const LSYSTEM * l, const char s);

PRODUCTION * lsystem_getProduction (const LSYSTEM * l, char s);
int lsystem_addProduction (LSYSTEM * l, const char s, const char * p);
void lsystem_clearProductions (LSYSTEM * l, const char s);
char * lsystem_getRandProduction (const LSYSTEM * l, const char s);

char * lsystem_iterate (const char * seed, const LSYSTEM * l, int i);

int production_sort (const void * a, const void * b);
int production_search (const void * key, const void * datum);

#endif /* LSYSTEM_H */