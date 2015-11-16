#ifndef CHE_TIME_H
#define CHE_TIME_H

#include <stdint.h>

typedef struct che_time_t 
{
	int      s;
	uint64_t ns;
} che_time_t;

int che_time_uptime(che_time_t *t);

void che_time_add(che_time_t *a, const che_time_t *b);

void che_time_sub(che_time_t *a, const che_time_t *b);

int che_time_cmp(const che_time_t *a, const che_time_t *b);

#endif /* CHE_TIME_H */
