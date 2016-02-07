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
#include "che_io_console.h"
#include "che_io_sdl.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

static
int che_file_load(const char *filename, uint8_t *dst, size_t max_size)
{
	int fd, off = 0;

	/* Open file */
	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return -1;

	/* Read contents to memory */
	for (;;) {
		int rd, max_read;
		max_read = max_size - off;
		if (max_read == 0)
			break;
		rd = read(fd, dst + off, max_read);
		if (rd == -1)
			return -1;
		if (rd == 0)
			break;
		off += rd;
	}

	/* Close file */
	close(fd);
	return 0;
}

static che_machine_t machine;
#ifdef CHE_USE_SDL
static che_io_sdl_t io_sdl;
#else /* CHE_USE_SDL */
static che_io_console_t io_console;
#endif /* CHE_USE_SDL */

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("ERROR: specify the program filename to be loaded\n");
		return -1;
	}

	#ifdef CHE_USE_SDL
	che_io_t *io = che_io_sdl_init(&io_sdl);
	#else /* CHE_USE_SDL */
	che_io_t *io = che_io_console_init(&io_console);
	#endif /* CHE_USE_SDL */
	if (che_file_load(argv[1], machine.mem + CHE_MACHINE_PROGRAM_START,
	                  sizeof(machine.mem) - CHE_MACHINE_PROGRAM_START) != 0) {
		printf("ERROR: loading file\n");
		return -1;
	}

	che_machine_init(&machine, io);

	che_machine_run(&machine);
	
	che_machine_end(&machine);

	return 0;
}
