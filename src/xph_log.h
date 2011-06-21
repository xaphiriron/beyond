#ifndef XPH_LOGS_H
#define XPH_LOGS_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "bool.h"

#define ERROR(x,...)			logLine (__FILE__, __LINE__, E_ERR, x, __VA_ARGS__)
#define WARNING(x,...)			logLine (__FILE__, __LINE__, E_WARN, x, __VA_ARGS__)
#define INFO(x,...)			logLine (__FILE__, __LINE__, E_INFO, x, __VA_ARGS__)
#define DEBUG(x,...)			logLine (__FILE__, __LINE__, E_DEBUG, x, __VA_ARGS__)
#define FUNCOPEN()			logLine (__FILE__, __LINE__, E_FUNCLABEL, "%s...", __FUNCTION__)
#define FUNCCLOSE()			logLine (__FILE__, __LINE__, E_FUNCLABEL, "...%s", __FUNCTION__)
#define LOG(level,x,...)		logLine (__FILE__, __LINE__, level, x, __VA_ARGS__)

typedef enum logLevels
{
	E_NONE = 0x00,

	E_ERR = 0x01,
	E_WARN = 0x02,
	E_INFO = 0x04,
	E_DEBUG = 0x08,
	E_FUNCLABEL = 0x10,

	E_ALL = 0xff
} LogLevel;

typedef enum logLoudness
{
	LOG_QUIET = FALSE,
	LOG_LOUD = TRUE,
} LogLoudness;

void logLine (const char * file, const unsigned int line, LogLevel level, const char * log, ...);

void logSetLevel (LogLevel level);
void logSetLoudness (LogLoudness loud);

#endif /* XPH_LOGS_H */