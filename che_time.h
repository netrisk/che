#ifndef CHE_TIME_H
#define CHE_TIME_H

#include <stdint.h>

typedef struct che_time_t 
{
	int      s;
	uint64_t ns;
} che_time_t;

int che_time_uptime(che_time_t *t);

void che_time_add(che_time_t *dst, const che_time_t *a, const che_time_t *b);

void che_time_sub(che_time_t *dst, const che_time_t *a, const che_time_t *b);

int che_time_cmp(const che_time_t *a, const che_time_t *b);

int che_time_get_s(const che_time_t *t);

uint64_t che_time_get_ns(const che_time_t *t);

#endif /* CHE_TIME_H */
