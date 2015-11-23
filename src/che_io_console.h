#ifndef CHE_IO_CONSOLE_H
#define CHE_IO_CONSOLE_H

#include <stdint.h>
#include <stdbool.h>
#include <che_io.h>

typedef struct che_io_console_t
{
	che_io_t io;

	uint8_t *data;
	bool     changed;
	int      w;
	int      h;
	bool     x_wrap;
	bool     y_wrap;
	uint16_t keymask;
} che_io_console_t;

che_io_t *che_io_console_init(che_io_console_t *c);

#endif /* CHE_IO_CONSOLE_H */
