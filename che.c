#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "che_log.h"

#define CHE_STACK_LEVELS 32
#define CHE_MEMORY_SIZE  4096
#define CHE_PROGRAM_START 0x200

#define CHE_DBG_OPCODES

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
} che_machine_t;

static void che_machine_init(che_machine_t *m)
{
	memset(&m->r,0, sizeof(che_regs_t));
	m->pc = CHE_PROGRAM_START;/* loaded programs must start here */
	m->sp = 0;
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

    #ifdef CHE_DBG_OPCODES
    che_log("opcode=%x",instruction);
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
	default:
		goto err;
	}
	return 0;
err:
	printf("Unrecognized instruction %04X at 0x%03X\n", instruction, m->pc);
	return -1;
}

static
int che_run(che_machine_t *m)
{
	for (;;) {
		if (che_cycle(m) != 0)
			return -1;
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

	return 0;
}

