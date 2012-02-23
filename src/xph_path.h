/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_PATH_H
#define XPH_PATH_H

#include <stdbool.h>
#include <string.h>

int xph_chdir (const char * path);
char * xph_canonPath (const char * filename, char * buffer, size_t size);
char * xph_canonCachePath (const char * filename);


bool xph_findBaseDir (const char * programPath);

#endif /* XPH_PATH_H */