/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "graph_common.h"

#include <string.h>

void graph_parseNodeArgs (Graph node, void (*callback)(void *, Graph), void * data)
{
	Graph
		arg;
	int
		i = 0;
	char
		pathbuffer[32];
	while (1)
	{
		sprintf (pathbuffer, "[%d]", i++);
		arg = Graph_get (node, pathbuffer);
		if (!arg)
			break;
		callback (data, arg);
	}
}

int arg_match (const char * matches[], const char * val)
{
	int
		i = 0;
	while (matches[i][0] != 0)
	{
		if (!strcmp (matches[i], val))
			return i;
		i++;
	}

	return -1;
}