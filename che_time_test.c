#include <stdio.h>
#include <che_time.h>

static const che_time_t che_time_tick = { 3, 0};

static inline
void che_sleep_tick(che_time_t *last_sleep_time)
{
	che_time_t next_sleep_time;
	che_time_t now;

	che_time_add(&next_sleep_time, last_sleep_time, &che_time_tick);

	che_time_uptime(&now);
	if (che_time_cmp(&now, &next_sleep_time) < 0) {
		che_time_t che_sleep_time;
		che_time_sub(&che_sleep_time, &next_sleep_time, &now);
        fprintf(stdout,"sleep --> secs=%d usecs=%llu\n",che_sleep_time.s,
                che_sleep_time.ns);
		che_time_sleep(&che_sleep_time);
	} else {
		fprintf(stdout,"WARNING: Tick lost\n");
	}
	*last_sleep_time = next_sleep_time;
}

int main(int argc,char** argv)
{
    che_time_t last_sleep_time;
	che_time_uptime(&last_sleep_time);

    for(;;) { 
        che_sleep_tick(&last_sleep_time);
    }

    return 0;
}
