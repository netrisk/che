#include <che_time.h>
#include <time.h>

#define CHE_TIME_NS_IN_S (1 * 1000 * 1000 * 1000)

int che_time_uptime(che_time_t *t)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	t->s = ts.tv_sec;
	t->ns = ts.tv_nsec;
	return 0;
}

void che_time_add(che_time_t *a, const che_time_t *b)
{
	a->s += b->s;
	a->ns += b->ns;
	if (a->ns >= CHE_TIME_NS_IN_S) {
		a->s++;
		a->ns -= CHE_TIME_NS_IN_S;
	}
}

void che_time_sub(che_time_t *a, const che_time_t *b)
{
	a->s -= b->s;
	if (a->ns < b->ns) {
		a->ns += CHE_TIME_NS_IN_S;
		a->s--;
	}
	a->ns -= b->ns;
}

int che_time_cmp(const che_time_t *a, const che_time_t *b)
{
	if (a->s > b->s)
		return 1;
	if (a->s < b->s)
		return -1;
	if (a->ns > b->ns)
		return 1;
	if (a->ns < b->ns)
		return -1;
	return 0;
}
