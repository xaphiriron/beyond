#include "xph_log.h"

#define LOGBUFSIZE 256

static char
	buffer[LOGBUFSIZE];
static unsigned int
	logLevel = E_ERR;
void logLine (const char * file, const unsigned int line, LogLevel level, const char * log, ...)
{
	va_list
		vargs;
	int
		l;
	if (!(level & logLevel))
		return;
	l = snprintf (buffer, LOGBUFSIZE, "%s:%d: ", file, line);
	switch (level)
	{
		case E_ERR:
			strncat (buffer, "\033[31;1m", LOGBUFSIZE - l);
			break;
		case E_WARN:
			strncat (buffer, "\033[33;1m", LOGBUFSIZE - l);
			break;
		case E_INFO:
			strncat (buffer, "\033[32;1m", LOGBUFSIZE - l);
			break;
		case E_DEBUG:
			strncat (buffer, "\033[34m", LOGBUFSIZE - l);
			break;
		case E_FUNCLABEL:
			strncat (buffer, "\033[35m", LOGBUFSIZE - l);
			break;
		default:
			strncat (buffer, "\033[35m", LOGBUFSIZE - l);
			break;
	}
	l = strlen (buffer);
	va_start (vargs, log);
	l += vsnprintf (buffer+l, LOGBUFSIZE - l, log, vargs);
	va_end (vargs);
	strncat (buffer, "\033[0m\n", LOGBUFSIZE - l);
	fprintf (stderr, buffer);
}

void logSetLevel (LogLevel level)
{
	logLevel = level;
}
