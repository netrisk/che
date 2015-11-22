#include <che_time.h>

#ifdef CHE_TIME_APPLE

#include <time.h>
#include <mach/clock.h>
#include <mach/mach.h>

int che_time_uptime(che_time_t *t)
{
    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    t->s = mts.tv_sec;
	t->ns = mts.tv_nsec;
	return 0;
}

void che_time_sleep(const che_time_t *t)
{
	struct timespec sleep_time =
		{ t->s, t->ns };
	while (nanosleep(&sleep_time, &sleep_time) != 0);
}

#endif /* CHE_TIME_APPLE */
