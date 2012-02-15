/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_GRAPH_COMMON_H
#define XPH_GRAPH_COMMON_H

#include "ogdl/ogdl.h"

void graph_parseNodeArgs (Graph node, void (*callback)(void *, Graph), void * data);
int arg_match (const char * matches[], const char * val);

#endif /* XPH_GRAPH_COMMON_H */