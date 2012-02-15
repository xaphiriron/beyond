/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "xph_path.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "xph_log.h"

#define PATHBUFFERSIZE	1024

static char
	SystemPath[PATHBUFFERSIZE],
	FullPathBuffer[PATHBUFFERSIZE];

char * absolutePath (const char * relPath)
{
	strcpy (FullPathBuffer, SystemPath);
	if (strlen (FullPathBuffer) + strlen (SystemPath) + 1 > PATHBUFFERSIZE)
		ERROR ("About to overrun the path buffer; this is an exploit; whoops");
	/* this is ugly in the case relPath := "./whatev" || "../foo/bar", but it
	 * still works.
	 */
	strcat (FullPathBuffer, relPath);
	return FullPathBuffer;
}

/* this is designed to be called only from main with argv[0]
 */
void setSystemPath (const char * programPath)
{
	char
		* f = NULL;
	memset (SystemPath, 0, PATHBUFFERSIZE);
	strcpy (SystemPath, programPath);
	/* this is dumb; libtools places the actual executable in {base}/.libs and that's confusing so that's why this removes the executable name /and/ its base folder. there's got to be a way to do this better but i'd have to have the libtools dox to do it, and i don't.
	 *  - xph 2011 06 14
	 */
	f = strrchr (SystemPath, '/');
	while (*f != 0)
	{
		*f++ = 0;
	}
	f = strrchr (SystemPath, '/');
	f++;
	while (*f != 0)
	{
		*f++ = 0;
	}
	DEBUG ("Set system path to '%s'", SystemPath);
}
