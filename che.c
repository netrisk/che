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

/* macros to get some fields from opcodes */

/* Get the X from opcodes of type:
 * ?X??
 * i.e 6XNN -> Extract the value of X
 */
#define CHE_GET_OPCODE_X(_opcode) ((_opcode >> 8) & 0xf)
/* Get the Y from opcodes of type:
 * ?X??
 * i.e 8XY0 -> Extract the value of Y
 */
#define CHE_GET_OPCODE_Y(_opcode) ((_opcode >> 4) & 0xf)

/* Get the NN from opcodes that use it.
 * ie. 6XNN
 */
#define CHE_GET_OPCODE_NN(_opcode) (uint8_t)(_opcode & 0x00FF)

#define CHE_GET_OPCODE_NNN(_opcode) (uint16_t)(_opcode & 0x0FFF)

#define CHE_GET_NIBBLE(_opcode, _n) ((_opcode >> (_n << 2)) & 0xf)

#define CHE_VF(_regs) _regs.v[15]
#define CHE_V0(_regs) _regs.v[0]

#define CHE_NEXT_INSTRUCTION(_pc) _pc+=2
#define CHE_SKIP_NEXT_INSTRUCTION(_pc) _pc+=4

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
	uint16_t opcode = m->mem[m->pc] << 8 | m->mem[m->pc + 1];
	uint8_t first_nibble = opcode >> 12;

	#ifdef CHE_DBG_OPCODES
	che_log("opcode=%04x",opcode);
	#endif /* CHE_DBG_OPCODES */

	switch (first_nibble) {
	case 0:
		if (opcode == 0x00EE) {
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
		m->pc = opcode & 0xfff;
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
		m->pc = opcode & 0xfff;
		break;
	case 3: /* 3XNN skips the next instruction if VX=NN*/
		#ifdef CHE_DBG_OPCODES
		che_log("Skip next instruction if register[%u] == %x",
		        CHE_GET_OPCODE_X(opcode),
		        CHE_GET_OPCODE_NN(opcode));
		#endif /* CHE_DBG_OPCODES */
		if( m->r.v[CHE_GET_OPCODE_X(opcode)] == CHE_GET_OPCODE_NN(opcode) ) {
			#ifdef CHE_DBG_OPCODES
			che_log("Skipping next instruction");
			#endif /* CHE_DBG_OPCODES */
			CHE_SKIP_NEXT_INSTRUCTION(m->pc);
		} else {
			#ifdef CHE_DBG_OPCODES
			che_log("Not skipping next instruction");
			#endif /* CHE_DBG_OPCODES */
			CHE_NEXT_INSTRUCTION(m->pc);
		}
		break;
	case 4: /* 4XNN Skip the next instruction if VX != NN */
		#ifdef CHE_DBG_OPCODES
		che_log("Skip next instruction if register[%u] != %x",
		        CHE_GET_OPCODE_X(opcode),
		        CHE_GET_OPCODE_NN(opcode));
		#endif /* CHE_DBG_OPCODES */
		if( m->r.v[CHE_GET_OPCODE_X(opcode)] != CHE_GET_OPCODE_NN(opcode) ) {
			#ifdef CHE_DBG_OPCODES
				che_log("Skipping next instruction");
				#endif /* CHE_DBG_OPCODES */
			CHE_SKIP_NEXT_INSTRUCTION(m->pc);
		} else {
			#ifdef CHE_DBG_OPCODES
			che_log("Not skipping next instruction");
			#endif /* CHE_DBG_OPCODES */
			CHE_NEXT_INSTRUCTION(m->pc);
		}
		break;
	case 5: /* 5XY0	Skips the next instruction if VX equals VY. */
        if( m->r.v[CHE_GET_OPCODE_X(opcode)] == m->r.v[CHE_GET_OPCODE_Y(opcode)] ) {
			CHE_SKIP_NEXT_INSTRUCTION(m->pc);
        } else {
			CHE_NEXT_INSTRUCTION(m->pc);
        }
	    break;
	case 6: /* 6XNN Set VX register to NN value */
		m->r.v[CHE_GET_OPCODE_X(opcode)] = CHE_GET_OPCODE_NN(opcode);
		#ifdef CHE_DBG_OPCODES
		che_log("setting register[%u] to value:%x",CHE_GET_OPCODE_X(opcode),
		        CHE_GET_OPCODE_NN(opcode));
		#endif /* CHE_DBG_OPCODES */
		CHE_NEXT_INSTRUCTION(m->pc);
		break;
	case 7:/* 7XNN Adds NN to VX */
        m->r.v[CHE_GET_OPCODE_X(opcode)] += CHE_GET_OPCODE_NN(opcode);
		CHE_NEXT_INSTRUCTION(m->pc);
	    break;
	case 8: 
	    switch(CHE_GET_NIBBLE(opcode,0))
        {
            case 0: /* 8XY0 sets Vx to Vy */
                 m->r.v[CHE_GET_OPCODE_X(opcode)] = m->r.v[CHE_GET_OPCODE_Y(opcode)];
                break;
            case 1: /* 8xy1 sets Vx to Vx OR Vy */
                 m->r.v[CHE_GET_OPCODE_X(opcode)] |= m->r.v[CHE_GET_OPCODE_Y(opcode)];
                break;
            case 2: /* 8xy2 Set Vx to Vx AND Vy */
                 m->r.v[CHE_GET_OPCODE_X(opcode)] &= m->r.v[CHE_GET_OPCODE_Y(opcode)];
                break;
            case 3: /* XOR */
                 m->r.v[CHE_GET_OPCODE_X(opcode)] ^= m->r.v[CHE_GET_OPCODE_Y(opcode)];
                break;
            case 4: { /* 8xy4 Adds Vy To Vx.Vf to 1 when there's a carry 
                         and 0 when not.*/ 
                uint8_t vy = m->r.v[CHE_GET_OPCODE_Y(opcode)];
                uint16_t vx = m->r.v[CHE_GET_OPCODE_X(opcode)];

                    vx+=vy;
                    if( vx > 0xff ) {
                        CHE_VF(m->r) = 1; /* carry present */
                    }
                    m->r.v[CHE_GET_OPCODE_X(opcode)] = (uint8_t)(vx & 0x00FF);
                }
                break;
            case 5: { /* 8xy5 Vy is substracted from vx. Vf set to 0 when 
                       borrow, and 1 when not. */
                    uint8_t vy = m->r.v[CHE_GET_OPCODE_Y(opcode)];
                    int16_t vx = m->r.v[CHE_GET_OPCODE_X(opcode)];

                    vx -= vy; /* ¿should I leave to zero of to absolute
                                 plus vf flag */
                    if( vx < 0 ) {
                        CHE_VF(m->r) = 0;
                        vx = -vx; /* remove the sign */
                    } else {
                        CHE_VF(m->r) = 1;
                    }
                    m->r.v[CHE_GET_OPCODE_X(opcode)] = (uint8_t)(vx & 0x00FF);
                }
                break;
            case 6: /* 8xy6 shifts VX right by one.
                       Vf is set to the value of the lsb of vx before the 
                       shift */
                    CHE_VF(m->r) = m->r.v[CHE_GET_OPCODE_X(opcode)] & 0x01;
                    m->r.v[CHE_GET_OPCODE_X(opcode)] >>= 1;
                break;
            case 7: {/* 8xy7 sets vx to vy minus vx. Vf set to 0 when there's 
                      a borrow and 1 when there isn't */
                    uint8_t vy = m->r.v[CHE_GET_OPCODE_Y(opcode)];
                    uint8_t vx = m->r.v[CHE_GET_OPCODE_X(opcode)];
                    int16_t res = vy - vx;

                    if( res > 0 ) {
                        CHE_VF(m->r) = 1;
                    } else {
                        CHE_VF(m->r) = 0;
                        res = -res; /* discard the sign */
                    }
                    m->r.v[CHE_GET_OPCODE_X(opcode)] = (uint8_t)(res & 0x00FF);
                    }
                break;
            case 8: /* 8xye shifts vx left by one.VF is set to the value of the 
                       msb of vx before the shift */ 
                CHE_VF(m->r) = m->r.v[CHE_GET_OPCODE_X(opcode)] & 0x80;
                m->r.v[CHE_GET_OPCODE_X(opcode)] <<= 1;
                break;
            default:
                goto err;
            }
		CHE_NEXT_INSTRUCTION(m->pc);
		break;
	case 9: /* 9XY0	Skips the next instruction if VX doesn't equal VY. */
        if( m->r.v[CHE_GET_OPCODE_X(opcode)] != m->r.v[CHE_GET_OPCODE_Y(opcode)] ) {
			CHE_SKIP_NEXT_INSTRUCTION(m->pc);
        } else {
			CHE_NEXT_INSTRUCTION(m->pc);
        }
	    break;
	case 0xa: /* ANNN	Sets I to the address NNN */
        m->r.i = CHE_GET_OPCODE_NNN(opcode);
		CHE_NEXT_INSTRUCTION(m->pc);
	    break;
	case 0xb: /* BNNN	Jumps to the address NNN plus V0. */
	    m->pc = CHE_V0(m->r) + CHE_GET_OPCODE_NNN(opcode);
	    break;
	case 0xd:
		/* DXYN: Draw sprite located at I of height N at X, Y */
		/* TODO: draw_sprite call is untested here */
		m->r.v[0xf] = che_scr_draw_sprite(&m->screen,
		                                  m->mem + m->r.i,
		                                  CHE_GET_NIBBLE(opcode, 0),
		                                  m->r.v[CHE_GET_NIBBLE(opcode, 2)],
		                                  m->r.v[CHE_GET_NIBBLE(opcode, 1)]);
		CHE_NEXT_INSTRUCTION(m->pc);
		break;
	case 0xf: {
            uint8_t lowest_byte = opcode & 0xff;
            /* Deal with timers */
            if (lowest_byte == 0x07) {
                m->r.v[CHE_GET_OPCODE_X(opcode)] = m->delay_timer;
            } else if (lowest_byte == 0x15) {
                m->delay_timer = m->r.v[CHE_GET_OPCODE_X(opcode)];
            } else if (lowest_byte == 0x18) {
                m->sound_timer = m->r.v[CHE_GET_OPCODE_X(opcode)];
            } else if( lowest_byte == 0x1e) { /* Adds VX to I.[3]
               VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't.
               This is undocumented feature of the CHIP-8 and used by Spacefight 2091! game*/
                
            } else {
                goto err;
            }
            CHE_NEXT_INSTRUCTION(m->pc);
        }
		break;
	default:
		goto err;
	}
	return 0;
err:
	printf("Unrecognized opcode %04X at 0x%03X\n", opcode, m->pc);
	return -1;
}

static const che_time_t che_time_tick = { 0, CHE_TICK_TIME_NS };

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
		struct timespec sleep_time =
			{ 0, che_time_get_ns(&che_sleep_time) };
		while (nanosleep(&sleep_time, &sleep_time) != 0);
	} else {
		che_log("WARNING: Tick lost");
	}
	*last_sleep_time = next_sleep_time;
}

static
int che_run(che_machine_t *m)
{
	che_time_t last_sleep_time;
	che_time_uptime(&last_sleep_time);

	for (;;) {
		int tick_cycles = 0;
		while (tick_cycles++ < CHE_TICK_CYCLES) {
			if (che_cycle(m) != 0)
				return -1;
		}
		che_scr_draw_screen(&m->screen);

		#ifdef CHE_DBG_STATS
		static che_time_t last_uptime = { 0, 0 };
		static uint32_t cycles_per_second = 0;
		static uint32_t ticks_per_second = 0;
		cycles_per_second += tick_cycles;
		ticks_per_second++;
		che_time_t now;
		che_time_uptime(&now);
		if (che_time_get_s(&last_uptime) == 0) {
			last_uptime = now;
		} else {
			if (che_time_get_s(&last_uptime) != che_time_get_s(&now)) {
				last_uptime = now;
				che_log("TICKS: %3d - KIPS: %d",
				        ticks_per_second,
				        cycles_per_second / 1000);
				cycles_per_second = 0;
				ticks_per_second = 0;
			}
		}
		#endif /* CHE_DBG_STATS */

		/* Sleep until the next tick */
		che_sleep_tick(&last_sleep_time);

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
