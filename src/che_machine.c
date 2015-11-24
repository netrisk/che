#include <che_machine.h>
#include <che_scr.h>
#include <che_log.h>
#include <che_time.h>
#include <che_cycle.h>
#include <che_rand.h>

/* Temporization */
#define CHE_TICK_HZ       60
#define CHE_KIPS          1000 /* 1 MHz */
#define CHE_TICK_CYCLES   (CHE_KIPS * 1000 / CHE_TICK_HZ)
#define CHE_TICK_TIME_NS  ((1 * 1000 * 1000 * 1000) / CHE_TICK_HZ)

/* #define CHE_MACHINE_DBG_STATS */

static const che_machine_char_t che_machine_char_table[16] =
{
	{ 0xf0, 0x90, 0x90, 0x90, 0xf0 }, /* 0 */
	{ 0x20, 0x60, 0x20, 0x20, 0x70 }, /* 1 */
	{ 0xf0, 0x10, 0xf0, 0x80, 0xf0 }, /* 2 */
	{ 0xf0, 0x10, 0xf0, 0x10, 0xf0 }, /* 3 */
	{ 0x90, 0x90, 0xf0, 0x10, 0x10 }, /* 4 */
	{ 0xf0, 0x80, 0xf0, 0x10, 0xf0 }, /* 5 */
	{ 0xf0, 0x80, 0xf0, 0x90, 0xf0 }, /* 6 */
	{ 0xf0, 0x10, 0x20, 0x40, 0x40 }, /* 7 */
	{ 0xf0, 0x90, 0xf0, 0x90, 0xf0 }, /* 8 */
	{ 0xf0, 0x90, 0xf0, 0x10, 0xf0 }, /* 9 */
	{ 0xf0, 0x90, 0xf0, 0x90, 0x90 }, /* a */
	{ 0xe0, 0x90, 0xe0, 0x90, 0xe0 }, /* b */
	{ 0xf0, 0x80, 0x80, 0x80, 0xf0 }, /* c */
	{ 0xf0, 0x90, 0x90, 0x90, 0xf0 }, /* d */
	{ 0xf0, 0x80, 0xf0, 0x80, 0xf0 }, /* e */
	{ 0xf0, 0x80, 0xf0, 0x80, 0x80 }, /* f */
};

static const che_time_t che_time_tick = { 0, CHE_TICK_TIME_NS };

int che_machine_init(che_machine_t *m, che_io_t *io)
{
	memset(&m->r,0, sizeof(che_regs_t));
	m->pc = CHE_MACHINE_PROGRAM_START;/* loaded programs start here */
	m->sp = 0;
	m->keymask = 0;
	m->delay_timer = 0;
	m->sound_timer = 0;
	che_rand_init(&m->rand, 0);
	memcpy(m->mem + CHE_MACHINE_CHAR_TABLE_POS, che_machine_char_table,
               sizeof(che_machine_char_table));
	m->io = io;
	return che_io_init(io, 64, 32);
}

static inline
void che_sleep_tick(che_time_t *last_sleep_time)
{
	che_time_t next_sleep_time;
	che_time_add(&next_sleep_time, last_sleep_time, &che_time_tick);
	che_time_t now;
	che_time_uptime(&now);
	if (che_time_cmp(&now, &next_sleep_time) < 0) {
		che_time_t che_sleep_time;
		che_time_sub(&che_sleep_time, &next_sleep_time, &now);
		che_time_sleep(&che_sleep_time);
	} else {
		che_log("WARNING: Tick lost");
	}
	*last_sleep_time = next_sleep_time;
}

int che_machine_run(che_machine_t *m)
{
	che_time_t last_sleep_time;
	che_time_uptime(&last_sleep_time);

	for (;;) {
		/* Get the pressed keys */
		m->keymask = che_io_keymask_get(m->io);

		/* Execute cycles */
		int tick_cycles = 0;
		while (tick_cycles++ < CHE_TICK_CYCLES) {
			if (che_cycle(m) != 0)
				return -1;
		}
		/* Render screen, to do all CPU intensive work before sleeping */
		che_io_scr_render(m->io);

		#ifdef CHE_MACHINE_DBG_STATS
		static che_time_t last_uptime = { 0, 0 };
		static uint32_t cycles_per_second = 0;
		static uint32_t ticks_per_second = 0;
		cycles_per_second += tick_cycles;
		ticks_per_second++;
		che_time_t now;
		che_time_uptime(&now);
		/* TODO: do this properly */
		if (last_uptime.s == 0) {
			last_uptime = now;
		} else {
			if (last_uptime.s != now.s) {
				last_uptime = now;
				che_log("TICKS: %3d - KIPS: %d",
				        ticks_per_second,
				        cycles_per_second / 1000);
				cycles_per_second = 0;
				ticks_per_second = 0;
			}
		}
		#endif /* CHE_MACHINE_DBG_STATS */

		/* Decrement counter for next ticks */
		if (m->delay_timer)
			m->delay_timer--;
		if (m->sound_timer)
			m->sound_timer--;

		/* Sleep until the next tick */
		che_sleep_tick(&last_sleep_time);

		/* Flip screen on the exact time that we wake up */
		che_io_scr_flip(m->io);
	}
	return 0;
}

void che_machine_end(che_machine_t *m)
{
	che_io_end(m->io);
}
