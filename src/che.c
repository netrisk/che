#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "che_machine.h"

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

static che_machine_t machine;

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("ERROR: specify the program filename to be loaded\n");
		return -1;
	}

	che_machine_init(&machine);
	if (che_machine_file_load(&machine, argv[1],
                                  CHE_MACHINE_PROGRAM_START) != 0) {
		printf("ERROR: loading file\n");
		return -1;
	}

	che_machine_run(&machine);
	
	che_machine_end(&machine);

	return 0;
}
