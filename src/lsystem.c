#include "lsystem.h"


LSYSTEM * lsystem_create () {
  LSYSTEM * l = xph_alloc (sizeof (LSYSTEM));
  l->p = dynarr_create (4, sizeof (PRODUCTION *));
  return l;
}

void lsystem_destroy (LSYSTEM * l) {
  while (!dynarr_isEmpty (l->p)) {
    production_destroy (*(PRODUCTION **)dynarr_pop (l->p));
  }
  dynarr_destroy (l->p);
  xph_free (l);
}

PRODUCTION * production_create (const char l, const char * exp) {
  PRODUCTION * p = xph_alloc (sizeof (PRODUCTION));
  p->l = l;
  p->exp = dynarr_create (2, sizeof (char *));
  production_addRule (p, exp);
  return p;
}

void production_destroy (PRODUCTION * p) {
  while (!dynarr_isEmpty (p->exp)) {
    xph_free (*(char **)dynarr_pop (p->exp));
  }
  dynarr_destroy (p->exp);
  xph_free (p);
}

void production_addRule (PRODUCTION * p, const char * exp) {
  char * pexp = xph_alloc_name (strlen (exp) + 1, "PRODUCTION->exp[]");
  strcpy (pexp, exp);
  //printf ("adding rule \"%c\" -> \"%s\" (total of %d)\n", p->l, pexp, dynarr_size (p->exp) + 1);
  dynarr_push (p->exp, pexp);
}

bool lsystem_isDefined (const LSYSTEM * l, const char s) {
  return (*(char **)dynarr_search (l->p, production_search, s) == NULL)
    ? false
    : true;
}

PRODUCTION * lsystem_getProduction (const LSYSTEM * l, char s) {
  return *(PRODUCTION **)dynarr_search (l->p, production_search, s);
}

int lsystem_addProduction (LSYSTEM * l, const char s, const char * p) {
  PRODUCTION * sp = lsystem_getProduction (l, s);
  if (sp == NULL) {
    sp = production_create (s, p);
    dynarr_push (l->p, sp);
    dynarr_sort (l->p, production_sort);
  } else {
    production_addRule (sp, p);
  }
  return dynarr_size (sp->exp);
}

void lsystem_clearProductions (LSYSTEM * l, const char s) {
  PRODUCTION * sp = lsystem_getProduction (l, s);
  int index;
  if (sp == NULL) {
    return;
  }
  index = in_dynarr (l->p, sp);
  if (index >= 0) {
    dynarr_unset (l->p, index);
    dynarr_condense (l->p);
  }
  production_destroy (sp);
}

char * lsystem_getRandProduction (const LSYSTEM * l, const char s) {
  PRODUCTION * sp = lsystem_getProduction (l, s);
  int i = 0;
  char
    * r = NULL,
    * v = NULL;
  //printf ("%s (%p, \'%c\')...\n", __FUNCTION__, l, s);
  if (sp == NULL) {
    r = xph_alloc_name (2, "LSYSTEM production");
    r[0] = s;
    r[1] = '\0';
    return r;
  }
  i = dynarr_size (sp->exp);
  if (i == 1) {
    v = *(char **)dynarr_front (sp->exp);
    //printf ("got \"%s\" as the 0-th index of %p\n", v, sp->exp);
    r = xph_alloc (strlen (v) + 1);
    strcpy (r, v);
    return r;
  }
  i = rand () % i;
  v = *(char **)dynarr_at (sp->exp, i);
  //printf ("got \"%s\" as the %d-th index of %p\n", v, i, sp->exp);
  r = xph_alloc (strlen (v) + 1);
  strcpy (r, v);
  //printf ("...%s\n", __FUNCTION__);
  return r;
}

char * lsystem_iterate (const char * seed, const LSYSTEM * l, int i) {
  int
    j = 0,
    sl = strlen (seed),
    pl = 0,
    o = 0,
    a = sl * 2;
  char
    * base = xph_alloc_name (sl + 1, "LSYSTEM base"),
    * production = NULL,
    * expansion = NULL;
  //printf ("%s (\"%s\", %p, %d)...\n", __FUNCTION__, seed, l, i);
  strncpy (base, seed, sl + 1);
  if (i <= 0) {
    //printf ("...%s (v. early)\n", __FUNCTION__);
    return base;
  }
  while (i-- > 0) {
    //printf ("iterating over \"%s\", %d to go...\n", base, i);
    expansion = xph_alloc_name (a, "LSYSTEM expansion");
    memset (expansion, '\0', a);
    j = 0;
    sl = strlen (base);
    while (j < sl) {
      production = lsystem_getRandProduction (l, base[j++]);
      //printf ("got production rule '%c' -> \"%s\" at index %d\n", base[j-1], production, j-1);
      pl = strlen (production);
      while (o + pl + 1 > a) {
        expansion = xph_realloc (expansion, a * 2);
        memset (expansion + a, '\0', a);
        a *= 2;
      }
      strncat (expansion, production, pl + 1);
      o += pl;
      xph_free (production);
    }
    //printf ("full expansion: \"%s\"\n", expansion);
    if (strcmp (base, expansion) == 0) {
      //printf ("We're running in circles here. (\"%s\")\n", expansion);
      xph_free (base);
      //printf ("...%s (early)\n", __FUNCTION__);
      return expansion;
    }
    xph_free (base);
    base = expansion;
  }
  //printf ("...%s\n", __FUNCTION__);
  return expansion;
}

int production_sort (const void * a, const void * b) {
  return (*(PRODUCTION **)a)->l - (*(PRODUCTION **)b)->l;
}

int production_search (const void * keyp, const void * datum) {
  char
    key;
  memcpy (&key, keyp, 1);
  return key - (*(PRODUCTION **)datum)->l;
}
