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

#include "xph_memory.h"
#include "xph_log.h"
#include <dirent.h>
#include <unistd.h>

#define DIR_SEPARATOR '/'

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
		* cache = NULL;
	static size_t
		cacheSizeAllocated = 32;
	while (cache == NULL || strlen (base) + strlen (filename) + 2 > cacheSizeAllocated)
	{
		xph_free (cache);
		cacheSizeAllocated *= 2;
		cache = xph_alloc (cacheSizeAllocated);
	}
	xph_canonPath (filename, cache, cacheSizeAllocated);
	return cache;
}

bool xph_findBaseDir (const char * programPath)
{
	char
		* nextPath,
		* fullPath;
	DIR
		* active;
	struct dirent
		* dirent;
	bool
		hasDataDir;
	fullPath = xph_alloc (strlen (programPath) + 1);
	strcpy (fullPath, programPath);
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
			xph_free (fullPath);
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
