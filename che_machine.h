#ifndef CHE_MACHINE_H
#define CHE_MACHINE_H

#include <che_scr.h>

#define CHE_MACHINE_STACK_LEVELS  32
#define CHE_MACHINE_MEMORY_SIZE   4096
#define CHE_MACHINE_PROGRAM_START 0x200

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

	/* Key support */
	uint16_t   keymask;

	/* Display */
	che_scr_t  screen;
} che_machine_t;

int che_machine_init(che_machine_t *m);

int che_machine_run(che_machine_t *m);

void che_machine_end(che_machine_t *m);

#endif /* CHE_MACHINE_H */