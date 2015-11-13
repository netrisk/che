#ifndef CHE_H
#define CHE_H

#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define CHE_STACK_LEVELS 32

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
	uint8_t    mem[4096];
} che_machine_t;

static void che_machine_init(che_machine_t *m)
{
	bzero(&m->r, sizeof(che_regs_t));
	m->pc = 0x200;
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
	switch (first_nibble) {
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
	default:
		printf("Unrecognized instruction %04X\n", instruction);
		return -1;
	}
	return 0;
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
		printf("ERROR: specify the filename\n");
		return -1;
	}

	che_machine_init(&machine);
	if (che_machine_file_load(&machine, argv[1], 0x200) != 0) {
		printf("ERROR: loading file\n");
		return -1;
	}

	che_run(&machine);

	return 0;
}

#endif /* CHE_H */
