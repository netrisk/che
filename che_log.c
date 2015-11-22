#include <stdarg.h>
#include <stdio.h>

void _che_log(const char *format, const char *file, int line, ...)
{
	va_list ap;
	va_start(ap, line);
	fprintf(stderr, "%-15s (%4d): ", file, line);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}
