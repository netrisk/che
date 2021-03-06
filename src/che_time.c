#include <che_time.h>

#define CHE_TIME_NS_IN_S (1 * 1000 * 1000 * 1000)

void che_time_add(che_time_t *dst, const che_time_t *a, const che_time_t *b)
{
	dst->s = a->s + b->s;
	dst->ns = a->ns + b->ns;
	if (dst->ns >= CHE_TIME_NS_IN_S) {
		dst->s++;
		dst->ns -= CHE_TIME_NS_IN_S;
	}
}

void che_time_sub(che_time_t *dst, const che_time_t *a, const che_time_t *b)
{
	dst->s = a->s - b->s;
	dst->ns = a->ns;
	if (dst->ns < b->ns) {
		dst->ns += CHE_TIME_NS_IN_S;
		dst->s--;
	}
	dst->ns -= b->ns;
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


