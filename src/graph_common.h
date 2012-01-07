#ifndef XPH_GRAPH_COMMON_H
#define XPH_GRAPH_COMMON_H

#include "ogdl/ogdl.h"

void graph_parseNodeArgs (Graph node, void (*callback)(void *, Graph), void * data);
int arg_match (const char * matches[], const char * val);

#endif /* XPH_GRAPH_COMMON_H */