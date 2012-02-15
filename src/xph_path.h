/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_PATH_H
#define XPH_PATH_H

void setSystemPath (const char * programPath);

char * absolutePath (const char * relPath);

#endif /* XPH_PATH_H */