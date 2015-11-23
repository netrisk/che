#ifndef CHE_IO_SDL_H
#define CHE_IO_SDL_H

#include <che_cfg.h>

#ifdef CHE_CFG_SDL

#include <stdint.h>
#include <stdbool.h>
#include <che_io.h>

typedef struct che_io_sdl_t
{
	che_io_t io;

	uint8_t *data;
	bool     changed;
	int      w;
	int      h;
	bool     x_wrap;
	bool     y_wrap;
	uint16_t keymask;
} che_io_sdl_t;

che_io_t *che_io_sdl_init(che_io_sdl_t *c);

#endif /* CHE_CFG_SDL */

#endif /* CHE_IO_SDL_H */
