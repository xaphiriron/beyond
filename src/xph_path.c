/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#include "xph_path.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "xph_log.h"
#include <dirent.h>
#include <unistd.h>

#define PATHBUFFERSIZE	256
#define DIR_SEPARATOR '/'

static char
	SystemPath[PATHBUFFERSIZE],
	FullPathBuffer[PATHBUFFERSIZE];


static char
	* base = NULL;

int xph_chdir (const char * path)
{
	int
		r;
	r = chdir (path);
	if (r == 0)
	{
		if (base)
			free (base);
		base = getcwd (NULL, 0);
	}
	printf ("set working directory to \"%s\"\n", base);
	return r;
}

char * xph_canonPath (const char * filename, char * buffer, size_t size)
{
	assert (buffer);
	memset (buffer, 0, size);
	if (strlen (base) + strlen (filename) + 1 > size)
		return NULL;

	if (strstr (filename, ".."))
	{
		strncpy (buffer, "no stop trying to put .. in paths >:E", size);
		return buffer;
	}
	strcpy (buffer, base);
	buffer[strlen(buffer)] = DIR_SEPARATOR;
	strcat (buffer, filename);
	return buffer;
}

char * xph_canonCachePath (const char * filename)
{
	static char
		cache[PATHBUFFERSIZE];
	xph_canonPath (filename, cache, PATHBUFFERSIZE);
	return cache;
}

bool xph_findBaseDir (const char * programPath)
{
	char
		fullPath[PATHBUFFERSIZE];
	char
		* nextPath;
	DIR
		* active;
	struct dirent
		* dirent;
	bool
		hasDataDir;
	strncpy (fullPath, programPath, PATHBUFFERSIZE - 1);
	nextPath = strrchr (fullPath, DIR_SEPARATOR);
	if (!nextPath)
	{
		fprintf (stderr, "Program placed in invalid directory or given malformed directory string\n");
		exit (1);
	}
	*nextPath = 0;
	while (1)
	{
		hasDataDir = false;
		active = opendir (fullPath);
		if (active == NULL)
		{
			perror ("Unable to continue checking dirs: ");
			exit (1);
		}
		while ((dirent = readdir (active)))
		{
			if (strcmp (dirent->d_name, "data") == 0 && (dirent->d_type == DT_UNKNOWN || dirent->d_type == DT_DIR))
				hasDataDir = true;
		}
		closedir (active);
		if (hasDataDir)
		{
			xph_chdir (fullPath);
			return true;
		}
		if (!(nextPath = strrchr (fullPath, DIR_SEPARATOR)))
		{
			fprintf (stderr, "Could not find a valid working directory!\n");
			exit (1);
		}
		*nextPath = 0;
	}
}

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
