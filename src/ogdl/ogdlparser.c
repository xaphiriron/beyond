/** \file ogdlparser.c

    Non-recursive OGDL parser: character stream to events.
    
    Needs only one consecutive ungetc().
    
    Usefull for 8-bit streams that are ASCII transparent,
    such as plain ASCII, ISO-? variants and UTF-8.
    
    TO DO:
      - fine grained events (related to formatting)
       
    Author: R. Veen
    Date: 26 Oct 2002
    Modified: see Changelog
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ogdl.h"

static const char * error_msg[] = {
    "",
    "malloc() returns null in graphHandler",
    "MAXLEVELS reached in graphHandler()",
    "level < 0 in graphHandler()",
    "graph at current level is null in graphHandler()",
    "mixed TABS and SPACES in indentation",
    "text buffer overflow at word() [1]",    
    "text buffer overflow at word() [2]",
    "text buffer overflow at quoted() [1]",
    "text buffer overflow at quoted() [2]", 
    "text buffer overflow at block() [1]",
    "text buffer overflow at block() [2]",
    "text buffer overflow at block() [3]",
    "text buffer overflow at block() [4]",      
    "MAXGROUPS reached",
    "groupIndex going to -1",
    "line(): max LEVELS reached",
};

#define NERRORS 17

static void error(OgdlParser p, int n)
{
    (*p->errorHandler)(p,n);
}

const char * OgdlParser_getErrorMessage(int n)
{
    if (n>=NERRORS) return "";
    return error_msg[n];
} 

void OgdlParser_error(OgdlParser p, int n)
{
    printf("ogdlparser.c: [ERR %d] %s at line %d\n",n,error_msg[n],p->line);
    exit(1);
}

/** Constructor. Returns a pointer to a newly allocated memory structure */

OgdlParser OgdlParser_new()
{
    OgdlParser p;
    
    p = (void *) malloc(sizeof(struct OgdlParser));
    if (!p) return NULL;
    
    p->level = 0;
    p->line_level = 0;
    p->saved_space = 0;
    p->saved_newline = 0;
    p->indentation[0] = 0;
    p->groupIndex = 0;
    p->g = 0;
    p->handler = OgdlParser_graphHandler;
    p->errorHandler = OgdlParser_error;

    p->tabs=-1;
    p->line=0;
    p->is_comment = 0;
    
    return p;
}

OgdlParser OgdlParser_reuse(OgdlParser p)
{
    if (!p) return 0;
  
    p->level = 0;
    p->line_level = 0;
    p->saved_space = 0;
    p->saved_newline = 0;
    p->indentation[0] = 0;
    p->groupIndex = 0;

    if (p->g) {
        if (p->g[0])
	    Graph_free(p->g[0]);
	free (p->g);
    }

    p->g = 0;  	                      
    // p->handler: maintain the same
    p->tabs=8;
    p->line=0;
    p->is_comment = 0;
    
    return p;
}

/** Destructor */

void OgdlParser_free (OgdlParser p)
{
    if (p->g) {
        if (p->g[0])
	    Graph_free(p->g[0]);
	free (p->g);
    }
    
    if (p) 
        free(p);
}

/** Changes the default event handler of the parser */

void OgdlParser_setHandler (OgdlParser p, void (*h)(OgdlParser p, int, int, char*))
{
    p->handler = h;
}

/** Changes the default error handler of the parser */

void OgdlParser_setErrorHandler (OgdlParser p, void (*h)(OgdlParser p, int n))
{
    p->errorHandler = h;
}

/** this is the default handler; it just prints the events
    that have content.
*/

void OgdlParser_printHandler(OgdlParser p, int lv, int type, char *s)
{
    int i;
    if (!type) return;
    
    for (i=0; i<lv*4; i++)
        putchar(' ');
    puts(s);
}

/** An event handler that creates a Graph nested structure holding
    the entire OGDL stream.
    
    XXX: errors are not reported to the parser!!! Errors in general
    to be reconsidered.
*/

void OgdlParser_graphHandler(OgdlParser p, int level, int type, char *s)
{
    int i;
    Graph g;

    /* format events ignored */
    if (!type) return;
    
    /* empty nodes are ignored */
    if (!strlen(p->buf)) return;
    
    if (!p->g) { 
        /* initialize */
        p->g = malloc(sizeof(Graph) * LEVELS);
        if (!p->g) { error(p,1); return; }
        for (i=1; i<LEVELS; i++)
            p->g[i]=0;
        p->g[0] = Graph_new("__root__");
    }
    
    /* sanity checks */
    if (level>=(LEVELS-1))   { error(p,2); return; }
    if (level < 0)           { error(p,3); return; }
    if (p->g[level] == NULL) { error(p,4); return; }

    /* create a new node and add it to current level */
    g = Graph_new(p->buf);
    Graph_addNode(p->g[level],g);
    p->g[level+1]=g;

}

static void event(OgdlParser p, int type, char *s)
{
    (*p->handler)(p,p->level,type,s);
}

/* Character classes:

   1 = WORD
   2 = SPACE
   3 = BREAK
   4 = END
   
*/

static int _charType ( int c )
{
    if ( c == ' ' || c == '\t' )  return 2;
    if ( c == '\n' || c == '\r' ) return 3;
    if ( c < ' ' )                return 4;
    if ( c >= 0xa0 )              return 1;
    if ( c < 0x7f )               return 1;
    return 4;
}

/* Character input functions */

static int getChar(OgdlParser p)
{
    if (p->src_type) 
        return ((char*)p->src)[p->src_index++];
    else
        return p->last_char = getc((FILE*)p->src);
}

static void unGetChar(OgdlParser p)
{
    if (p->src_type)
        p->src_index--;
    else
        if (p->last_char != EOF)		// XXX is this compatible with EOS ?
            ungetc(p->last_char,(FILE*)p->src);
}

/* character classes */

/*static int isCharSeparator(int c)
{
    return ((c == ',') || (c == ';')) ? 1 : 0;
}*/

static int isCharSpace(int c)
{
    return ((c == ' ') || (c == '\t')) ? 1 : 0;
}

static int isCharBreak(int c)
{
    return ((c == '\n') || (c == '\r')) ? 1 : 0;
}

/*static int isCharString(int c)
{
    return (isCharSpace(c) || isCharBreak(c)) ? 0 : 1;
}*/

/* A space() implementation that handles intermixed spaces and tabs. 

static int space(OgdlParser p, int check)
{
    int c, i;

    if (p->saved_space > 0) {
        i = p->saved_space;
        p->saved_space = 0;
        return i;
    }

    i = 0;
    while (1) {
        c = getChar(p);
        if (c == ' ') 
            i++;
        else if (c == '\t') 
            i = (i/p->tabs+1)*p->tabs;
        else
            break;
    }
    
    unGetChar(p);   
    return i;
}

*/

/* space() has control over p->tabs, and controls that tabs and spaces 
   are not mixed in indentation 

   Returns:
   
   0..n : number of spaces or tabs
   -9 : error   
*/

static int space(OgdlParser p, int check)
{
    int c, i, tabs=0, sps=0;

    if (p->saved_space > 0) {
        i = p->saved_space;
        p->saved_space = 0;
        return i;
    }

    i = 0;
    while (1) {
        c = getChar(p);
        if (c == ' ') {
            i++;
            sps++;
        }
        else if (c == '\t') {
            i++;
            tabs++;
        }
        else
            break;
    }
    
    if (check) {
        if ( (sps && tabs) || (p->tabs == ' ' && tabs) || (p->tabs == '\t' && sps)) 
            { error(p,5); return -9; }
        else if (i)
            p->tabs = sps?' ':'\t'; 
    }

    unGetChar(p);   
    return i;
}


/** newline : (CR LF) | CR | LF */

static int newline(OgdlParser p)
{
    int c;

    if (p->saved_newline) {
        p->saved_newline = 0;
	p->is_comment = 0;
        return 1;
    }

    c = getChar(p);

    if (c == '\r') {
        c = getChar(p);
        if (c != '\n')
            unGetChar(p);
        p->line++;
	p->is_comment = 0;
        return 1;
    } else if (c == '\n') {
        p->line++;
	p->is_comment = 0;
        return 1;
    }

    unGetChar(p);
    return 0;
}

static int eos(OgdlParser p)
{
    int c;

    c = getChar(p);
    unGetChar(p);                   /* XXX check what happens with EOF */
    return ( _charType(c) == 4 ) ? 1 : 0;
}

/*
    Returns:
    
    0...n : number of characters
    -9 : error
*/

static int word(OgdlParser p)
{
    int c, i = 0, lv=2;

    if ( p->is_comment ) lv = 3;

    while (1) {
        c = getChar(p);
	if (i==1 && (c==' ' || c=='\t') && p->buf[0]=='#' ) lv=3; 
        if ( _charType(c) < lv ) {
            if (i>=BUFFER) { error(p,6); return -9; }
            p->buf[i++] = c;
        } else
            break;
    }

    if (i>=BUFFER) { error(p,7); return -9; }
    p->buf[i] = 0;
    unGetChar(p);
    
    /* if word is '#', turn on is_comment, until next newline */
    if ( !strcmp("#",p->buf) ) p->is_comment = 1;
    
    return i;
}

/*
    Returns:
    
    0 : not a quoted string
    1 : is quoted string (now in p->buf)
    -9 : error
*/

static int quoted(OgdlParser p)
{
    int i=0, q, c, cc=0, flag = 0, n;

    n  = p->indentation[p->level];

    q = getChar(p);

    if (q != '"' && q != '\'') {
        unGetChar(p);
        return 0;
    }

    while (1) {
        c = getChar(p);
        if ((c == -1) || (c == q && cc != '\\'))
            break;

        if (flag) {
            if (isCharSpace(c)) {
                flag--;
                cc = c;
                continue;
            }
            else
                flag = 0;
        }
        
        if (i>=BUFFER) { error(p,8); return -9; }
        p->buf[i++] = c;
        cc = c;
        if (c == '\n')
            flag = n+1;
    }

    if (i>=BUFFER) { error(p,9); return -9; }
    p->buf[i] = 0;

    return 1;
}

/*
    Returns:
    
    0 : not a block
    1 : is a block (now in p->buf)
    -9 : error
*/

static int block(OgdlParser p, int n)
{
    int c, i = 0, j, m, ind = -1;

    c = getChar(p);

    if (c != '\\') {
        unGetChar(p);
        return 0;
    }

    if (!newline(p))
        return 0;   /* illegal or we need 2 ungetc's */

    /* all lines indented at least n are this block, and also less indented empty lines */

    for (;;) {
        m = space(p,1);
        if (m==-9) return -9;
        
        if (m < n) {
            if (newline(p)) {
                if (i>=BUFFER) { error(p,10); return -9; }
                p->buf[i++] = '\n';
            } else {
                p->saved_space = m;
                break;
            }
        } else {
            if (ind < 0)
                ind = m;                /* set ind to the indentation level of the first line */
            if (i>=BUFFER) { error(p,11); return -9; }
            for (j = ind; j < m; j++)     /* add those spaces that are not indentation */
                p->buf[i++] = ' ';
            while (1) {
                c = getChar(p);
                if (i>=BUFFER) { error(p,12); return -9; }
                if (c == EOF || c == '\n') {
                    p->buf[i++] = '\n';
                    break;
                }
                p->buf[i++] = c;
            }
            if (c == EOF)
                break;

        }
    }

    if (i>=BUFFER) { error(p,13); return -9; }
    p->buf[i] = 0;
    
    /* chomp (eliminate last break) */
    i--;
    if (i<0) return 1;
    if (isCharBreak(p->buf[i-1]))
        p->buf[i]=0;
    i--;
    if (i<0) return 1;
    if (isCharBreak(p->buf[i-1]))
        p->buf[i]=0;    
    
    return 1;
}

/* node ::= word | quoted | block 


   returns:
      0  : Not a node
      1  : More
      -1 :
      -9 : error
*/

static int node(OgdlParser p)
{
    int i=0,len,j;

    /* block indentation starts at current line indentation + 1, 
       not the indentation of the last node on the line. */
    
    j = block(p,p->indentation[p->line_level]+1); 
    if (j<0) return -9;
    
    if (j) {
        event(p,1,p->buf);
        return -1;
    }
    
    j = quoted(p);
    if (j==-9) return -9;
    
    if (!j) {
        j = word(p);
        if (!j || j==-9) return j;
    }
    
    len = strlen(p->buf);
    if (!len) 
        strcpy(p->buf,"''");    /* avoid error */
    else 
        i = p->buf[len-1];
       
    if ( len == 1 && i == '(' ) {
        if (p->groupIndex>(GROUPS-1))
            { error(p,14); return -9; }
        p->groups[p->groupIndex++] = p->level;
        event(p,0,"(");
        return 1;
    }
    else if (len == 1 && i == ')') {
        if (!p->groupIndex)
            { error(p,15); return -9; }
        
        p->level = p->groups[--(p->groupIndex)];
        
        event(p,0,")");
        return 1;
    }

    if ( i == ',' )
        p->buf[--len] = 0;
    
    if (len != 0)
        event(p,1, p->buf);

    if ( i == ',' ) {
        if (p->groupIndex>0) 
            p->level = p->groups[p->groupIndex-1];
        else 
            p->level = p->line_level;
    }
    else
        p->level++;   

    return 1;
}

/** line() : space? ( node ( space node )* )? space? newline

    returns:
      0 : EOS or error
      1 : more
*/

static int line(OgdlParser p)
{
    int i;
    
    i = space(p,1);
    if (i == -9) return 0;          /* error */
  
    if ( newline(p) ) return 1;     /* empty line */
    if ( eos (p) )    return 0;     /* end of stream */

    if ( p->level == 0 ) {
        p->indentation[0]=i;
        p->line_level=0;
    }
    else {
        if ( i > p->indentation[p->line_level] ) {
            p->line_level++;
            if (p->line_level>=LEVELS)
                { error(p,16); return 0; }
            p->indentation[p->line_level] = i;
        }
        else {
            if ( i < p->indentation[p->line_level] ) {
                while (p->line_level > 0) {
                    if ( i >= p->indentation[p->line_level] )
                        break;
                    p->line_level--;  
                }
            }
        }
    }
    
    p->level = p->line_level;

    while ( (i=node(p)) > 0) 
        if ( space(p,0) == -9 ) return 0;    /* error */
             
    if (i == -9 || eos(p))      /* error || end of stream */
        return 0;
    
    return 1;
}

/** Do the work */

int OgdlParser_parse (OgdlParser p, FILE *f)
{
    p->src = f;
    p->src_type = 0;	// file
    while ( line(p) );
    return 0;
}

int OgdlParser_parseString (OgdlParser p, char *s)
{
    p->src = s;
    p->src_type = 1;	// string
    p->src_index = 0;
    while ( line(p) );
    return 0;
}

Graph Ogdl_load (char * file)
{
    OgdlParser p;
    Graph g = 0;
    FILE *f;
    
    f = fopen(file,"r");
    if (!f) return 0;

    p = OgdlParser_new();
    if (!p) {
        fclose(f);
	return 0;
    }
    
    OgdlParser_parse(p,f);

    fclose(f);
    
    /* free the parser but not the graph */
    if (p->g) {
        g = p->g[0];
        free (p->g);
    }
    free(p);    
    
    return g;
}
