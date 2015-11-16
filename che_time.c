#include <che_time.h>

#define CHE_TIME_NS_IN_S (1 * 1000 * 1000 * 1000)

#if __APPLE__
#include <sys/sysctl.h>
int che_time_uptime(che_time_t *t)
{
	struct timeval uptime;
	size_t len = sizeof(uptime);
	sysctlbyname("kern.boottime", &uptime, &len, NULL, 0);
	t->s = uptime.tv_sec;
	t->ns = uptime.tv_usec * 1000;
	return 0;
}
#else
#include <time.h>
int che_time_uptime(che_time_t *t)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return -1;
	t->s = ts.tv_sec;
	t->ns = ts.tv_nsec;
	return 0;
}
#endif

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

int che_time_get_s(const che_time_t *t)
{
	return t->s;
}

uint64_t che_time_get_ns(const che_time_t *t)
{
	return t->s * CHE_TIME_NS_IN_S + t->ns;
}
