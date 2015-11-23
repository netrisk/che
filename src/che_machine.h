#ifndef CHE_MACHINE_H
#define CHE_MACHINE_H

#include <stdint.h>
#include <che_rand.h>
#include <che_scr.h>
#include <che_io.h>

#define CHE_MACHINE_STACK_LEVELS   32
#define CHE_MACHINE_MEMORY_SIZE    4096
#define CHE_MACHINE_PROGRAM_START  0x200
#define CHE_MACHINE_CHAR_TABLE_POS 0x0

typedef uint8_t che_machine_char_t[5];

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
	uint16_t   stack[CHE_MACHINE_STACK_LEVELS];
	int        sp;

	/* Memory */
	uint8_t    mem[CHE_MACHINE_MEMORY_SIZE];

	/* Timers */
	uint8_t    delay_timer;
	uint8_t    sound_timer;

	/* Video and keyboard IO */
	che_io_t  *io;

	/* Key support */
	uint16_t   keymask;

	/* Random number generator */
	che_rand_t rand;
} che_machine_t;

int che_machine_init(che_machine_t *m, che_io_t *io);

int che_machine_run(che_machine_t *m);

void che_machine_end(che_machine_t *m);

#endif /* CHE_MACHINE_H */
