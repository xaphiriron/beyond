/** \file ogdl.h */

#ifndef	_OGDL_H
#define _OGDL_H

#include "stdio.h"

#define VERSION "20041124"

enum { ERROR_none, ERROR_notFound, ERROR_malloc, ERROR_realloc, ERROR_argumentOutOfRange, ERROR_noObject, ERROR_argumentIsNull };


/** Graph */

typedef struct Graph {
    char * name;
    char * type;
    int    size;
    int    size_max;
    struct Graph **nodes;
} * Graph;

Graph   Graph_new          (char * name);
void    Graph_free         (Graph g);
Graph   Graph_get          (Graph g, char * path);
char *  Graph_getString    (Graph g, char * path);
int     Graph_set          (Graph g, char * path, Graph node);
Graph   Graph_md           (Graph g, char * path);
Graph   Graph_getNode      (Graph g, char * name);
int     Graph_setName      (Graph g, char *s);
int     Graph_addNode      (Graph g, Graph node);
void    Graph_print        (Graph g);
void    Graph_fprint       (Graph g, FILE *fp, int maxlevel, int nspaces, int mode);
int     Graph_printString  (const char *s, int indent, int pending_break);
int     Graph_fprintString (FILE *fp, const char *s, int indent, int pending_break);

/** Path */

char *  Path_element       (char * path, char * buf);

#define LEVELS 128
#define GROUPS 128
#define BUFFER 65534    /* lower that int16 maxvalue, just in case */
#define OGDL_EOS '\f'   /* XXX any char < 0x20 except NL, CR, TAB */

/** OgdlParser */

typedef struct OgdlParser {
    int last_char;
    int groups[GROUPS];
    int groupIndex;
    char buf[BUFFER];    // XXX dynamic allocation
    int level;
    int line;
    int line_level;
    int saved_space;
    int saved_newline;
    int tabs;           /* number of spaces per tab */
    int indentation[LEVELS];
    void (*handler)(struct OgdlParser *, int, int, char *);
    void (*errorHandler)(struct OgdlParser *, int);
    
    void *src;
    int  src_index;
    int  src_type;
    
    Graph *g;
    int is_comment;
} * OgdlParser;

OgdlParser   OgdlParser_new              (void);
OgdlParser   OgdlParser_reuse            (OgdlParser p);
void         OgdlParser_free             (OgdlParser p);
void         OgdlParser_setHandler       (OgdlParser p, void (*h)(OgdlParser,int,int,char *));
int          OgdlParser_parse            (OgdlParser p, FILE * f);
int          OgdlParser_parseString      (OgdlParser p, char * s);
void         OgdlParser_graphHandler     (OgdlParser p, int level, int type, char * s);
void         OgdlParser_printHandler     (OgdlParser p, int level, int type, char * s);
Graph        Ogdl_load                   (char * fileName);
void         OgdlParser_error            (OgdlParser p, int n);
void         OgdlParser_setErrorHandler  (OgdlParser p, void (*h)(OgdlParser p, int n));
const char * OgdlParser_getErrorMessage  (int n);

#define OGDL_ERROR_TABS_SPACES    5

/** OgdlLog */

typedef struct OgdlLog {
    FILE * f;
    OgdlParser p;
} * OgdlLog;

OgdlLog        OgdlLog_new      (char *fileName);
void           OgdlLog_free     (OgdlLog l);
unsigned long  OgdlLog_add      (OgdlLog l, Graph g);
Graph          OgdlLog_get      (OgdlLog l, unsigned long offset);
Graph          OgdlLog_next     (OgdlLog l);
unsigned long  OgdlLog_position (OgdlLog l);

#endif
