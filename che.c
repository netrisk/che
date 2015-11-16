#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "che_log.h"
#include "che_scr.h"
#include "che_time.h"

#define CHE_STACK_LEVELS  32
#define CHE_MEMORY_SIZE   4096
#define CHE_PROGRAM_START 0x200

/* Temporization */
#define CHE_TICK_HZ       60
#define CHE_KIPS          1000 /* 1 MHz */
#define CHE_TICK_CYCLES   (CHE_KIPS * 1000 / CHE_TICK_HZ)
#define CHE_TICK_TIME_NS  ((1 * 1000 * 1000 * 1000) / CHE_TICK_HZ)

/* To check if a key is pressed */
#define CHE_KEY_PRESSED(_keymask, _key) ((_keymask >> _key) & 1)

#define CHE_DBG_STATS 
/* #define CHE_DBG_OPCODES */

#ifdef CHE_DBG_STATS
    #if __APPLE__
        #include <sys/sysctl.h>
    #else /* other platforms like linux */
        #include <sys/sysinfo.h>
    #endif /* __APPLE__ */
#endif /* CHE_DBG_STATS */

/* macros to get some fields from opcodes */

/* Get the X from opcodes of type:
 * ?X??
 * i.e 6XNN -> Extract the value of X
 */
#define CHE_GET_OPCODE_X(_opcode) ((_opcode >> 8) & 0xf)

/* Get the NN from opcodes that use it.
 * ie. 6XNN
 */
#define CHE_GET_OPCODE_NN(_opcode) (uint8_t)(_opcode & 0x00FF)

typedef struct che_regs_t
{
	uint8_t  v[16];
	uint16_t i;
} che_regs_t;

typedef struct che_machine_t
{
	/* Registers and program counter */
	che_regs_t r;
	uint16_t   pc;

	/* Stack */
	uint16_t   stack[CHE_STACK_LEVELS];
	int        sp;

	/* Memory */
	uint8_t    mem[CHE_MEMORY_SIZE];

	/* Timers */
	uint8_t    delay_timer;
	uint8_t    sound_timer;

	/* Key support */
	uint16_t   keymask;

	/* Display */
	che_scr_t  screen;
} che_machine_t;

static int che_machine_init(che_machine_t *m)
{
	memset(&m->r,0, sizeof(che_regs_t));
	m->pc = CHE_PROGRAM_START;/* loaded programs must start here */
	m->sp = 0;
	m->keymask = 0;
	m->delay_timer = 0;
	m->sound_timer = 0;
	return che_scr_init(&m->screen, 64, 32);
}

static void che_machine_end(che_machine_t *m)
{
	che_scr_end(&m->screen);
}

static
int che_machine_file_load(che_machine_t *m, const char *filename,
                          uint16_t addr)
{
	int fd;

	/* Open file */
	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return -1;

	/* Read contents to memory */
	for (;;) {
		int rd, max_read;
		max_read = sizeof(m->mem) - addr;
		if (max_read == 0)
			break;
		rd = read(fd, m->mem + addr, max_read);
		if (rd == -1)
			return -1;
		if (rd == 0)
			break;
		addr += rd;
	}

	/* Close file */
	close(fd);
	return 0;
}


static inline
int che_cycle(che_machine_t *m)
{
	uint16_t instruction = m->mem[m->pc] << 8 | m->mem[m->pc + 1];
	uint8_t first_nibble = instruction >> 12;
	uint8_t lowest_byte = instruction & 0xff;

    #ifdef CHE_DBG_OPCODES
    che_log("opcode=%04x",instruction);
	#endif /* CHE_DBG_OPCODES */

	switch (first_nibble) {
	case 0:
		if (instruction == 0x00EE) {
			/* Return from subroutine */
			if (m->sp == 0) {
				printf("Empty stack\n");
				return -1;
			}
			m->sp--;
			m->pc = m->stack[m->sp];
		} else {
			goto err;
		}
		break;
	case 1:
		/* 1NNN: Jump to NNN */
		m->pc = instruction & 0xfff;
	    #ifdef CHE_DBG_OPCODES
	    che_log("Jumping to address=%x",m->pc);
	    #endif /* CHE_DBG_OPCODES */
		break;
	case 2:
		/* 2NNN: Call NNN Sub */
		if (m->sp >= CHE_STACK_LEVELS) {
			printf("Stack overflow\n");
			return -1;
		}
		m->stack[m->sp++] = m->pc + 2;
		m->pc = instruction & 0xfff;
		break;
	case 3: /* 3XNN skips the next instruction if VX=NN*/
	    #ifdef CHE_DBG_OPCODES
        che_log("Skip next instruction if register[%u] == %x",CHE_GET_OPCODE_X(instruction),
                CHE_GET_OPCODE_NN(instruction));
	    #endif /* CHE_DBG_OPCODES */
        if( m->r.v[CHE_GET_OPCODE_X(instruction)] == CHE_GET_OPCODE_NN(instruction) ) {
            #ifdef CHE_DBG_OPCODES
	        che_log("Skipping next instruction");
	        #endif /* CHE_DBG_OPCODES */
            m->pc += 4; /* skip the next instruction */   
        } else {
            #ifdef CHE_DBG_OPCODES
	        che_log("Not skipping next instruction");
	        #endif /* CHE_DBG_OPCODES */
            m->pc += 2; /* go for next instruction */
        }
        break;
    case 4: /* 4XNN Skip the next instruction if VX != NN */
        #ifdef CHE_DBG_OPCODES
        che_log("Skip next instruction if register[%u] != %x",CHE_GET_OPCODE_X(instruction),
                CHE_GET_OPCODE_NN(instruction));
	    #endif /* CHE_DBG_OPCODES */
        if( m->r.v[CHE_GET_OPCODE_X(instruction)] != CHE_GET_OPCODE_NN(instruction) ) {
            #ifdef CHE_DBG_OPCODES
	        che_log("Skipping next instruction");
	        #endif /* CHE_DBG_OPCODES */
            m->pc += 4;
        } else {
            #ifdef CHE_DBG_OPCODES
	        che_log("Not skipping next instruction");
	        #endif /* CHE_DBG_OPCODES */
            m->pc += 2;
        }
        break;
    case 6: /* 6XNN Set VX register to NN value */
        m->r.v[CHE_GET_OPCODE_X(instruction)] = CHE_GET_OPCODE_NN(instruction);
        #ifdef CHE_DBG_OPCODES
        che_log("setting register[%u] to value:%x",CHE_GET_OPCODE_X(instruction),
                CHE_GET_OPCODE_NN(instruction));
	    #endif /* CHE_DBG_OPCODES */
        m->pc += 2; /* next instruction */
        break;
    case 8:
        break;
	case 0xf:
		if (lowest_byte == 0x07) {
			m->r.v[CHE_GET_OPCODE_X(instruction)] = m->delay_timer;
		} else if (lowest_byte == 0x15) {
			m->delay_timer = m->r.v[CHE_GET_OPCODE_X(instruction)];
		} else if (lowest_byte == 0x18) {
			m->sound_timer = m->r.v[CHE_GET_OPCODE_X(instruction)];
		} else {
			goto err;
		}
		m->pc += 2;
		break;
	default:
		goto err;
	}
	return 0;
err:
	printf("Unrecognized instruction %04X at 0x%03X\n", instruction, m->pc);
	return -1;
}

#if __APPLE__
typedef struct sysinfo {
    uint32_t uptime;
}sysinfo;
#endif /* __APPLE__ */

static
int che_run(che_machine_t *m)
{
	che_time_t che_last_uptime;
	che_time_uptime(&che_last_uptime);
	//static const che_time_t che_time_tick = { 0, CHE_TICK_TIME_NS };
	for (;;) {
		int tick_cycles = 0;
		while (tick_cycles++ < CHE_TICK_CYCLES) {
			if (che_cycle(m) != 0)
				return -1;
		}
		che_scr_draw_screen(&m->screen);

		#ifdef CHE_DBG_STATS
		/* TODO: this will only work in linux */
		static uint32_t last_uptime = 0;
		static uint32_t cycles_per_second = 0;
		static uint32_t ticks_per_second = 0;
		cycles_per_second += tick_cycles;
		ticks_per_second++;
		struct sysinfo info;
        #if __APPLE__
        struct timeval uptime;
        size_t len = sizeof(uptime);
        sysctlbyname("kern.boottime",&uptime,&len,NULL,0);
        info.uptime = uptime.tv_sec;
        #else
		sysinfo(&info);
		#endif /* __APPLE__ */
		if (last_uptime == 0) {
			last_uptime = info.uptime;
		} else {
			if (last_uptime != info.uptime) {
				last_uptime = info.uptime;
				che_log("TICKS: %3d - KIPS: %d",
                                        ticks_per_second,
                                        cycles_per_second / 1000);
				cycles_per_second = 0;
				ticks_per_second = 0;
			}
		}
		#endif /* CHE_DBG_STATS */

		/* Sleep until the next tick */
		/* TODO: just a quick approach, the sleep time should be
                         correctly calculated with the uptime */
		//che_time_t next_uptime = che_last_uptime;
		//che_time_add(&next_uptime, &che_time_tick);
		//che_time_t now;
		//che_time_uptime(&now);
		//if (che_time_cmp(&now, &next_uptime) < 0) {
		//	che_time_t che_sleep_time = next_uptime;
		//	che_time_sub(&che_sleep_time, &now);
		//	struct timespec sleep_time = { 0, che_sleep_time.ns };
		//	while (nanosleep(&sleep_time, &sleep_time) != 0);
		//}

		struct timespec sleep_time = { 0, CHE_TICK_TIME_NS };
		while (nanosleep(&sleep_time, &sleep_time) != 0);
		
		if (m->delay_timer)
			m->delay_timer--;
		if (m->sound_timer)
			m->sound_timer--;
	}
	return 0;
}

static che_machine_t machine;

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("ERROR: specify the program filename to be loaded\n");
		return -1;
	}

	che_machine_init(&machine);
	if (che_machine_file_load(&machine, argv[1], CHE_PROGRAM_START) != 0) {
		printf("ERROR: loading file\n");
		return -1;
	}

	che_run(&machine);
	
	che_machine_end(&machine);

	return 0;
}
