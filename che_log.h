#ifndef CHE_LOG_H
#define CHE_LOG_H

#include <stdarg.h>
#include <string.h>

#ifdef __FILE_BASENAME__
#define FILENAME __FILE_BASENAME__
#else
static inline
const char *_che_log_filebasename(const char *file)
{
	const char *slash = strrchr(file, '/');
	if (slash)
		return slash + 1;
	return file;
}
#define FILENAME _che_log_filebasename(__FILE__)
#endif

#define che_log(_format, ...) \
	_che_log(_format, FILENAME, __LINE__, ##__VA_ARGS__)

void _che_log(const char *format, const char *file, int line, ...);

#endif /* CHE_LOG_H */
