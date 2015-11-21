#include <che_time.h>

void che_time_sleep(const che_time_t *t)
{
	struct timespec sleep_time =
		{ t->s, t->ns };
	while (nanosleep(&sleep_time, &sleep_time) != 0);
}
