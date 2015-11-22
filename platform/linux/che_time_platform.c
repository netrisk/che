#include <che_time.h>

#ifdef CHE_TIME_LINUX
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

void che_time_sleep(const che_time_t *t)
{
	struct timespec sleep_time =
		{ t->s, t->ns };
	while (nanosleep(&sleep_time, &sleep_time) != 0);
}
#endif /* CHE_TIME_LINUX */
