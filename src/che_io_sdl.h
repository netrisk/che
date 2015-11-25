#ifndef CHE_IO_SDL_H
#define CHE_IO_SDL_H

#include <che_cfg.h>

#ifdef CHE_CFG_SDL

#include <stdint.h>
#include <stdbool.h>
#include <che_io.h>
#include <SDL.h>

typedef struct che_io_sdl_t
{
	/* Base class */
	che_io_t io;

	/* Own members */
	uint8_t      *data;
	bool          changed;
	int           w;
	int           h;
	bool          x_wrap;
	bool          y_wrap;
	uint16_t      keymask;
	SDL_Window   *window;
	SDL_Renderer *renderer;

	/* Audio */
	int v;
} che_io_sdl_t;

che_io_t *che_io_sdl_init(che_io_sdl_t *c);

#endif /* CHE_CFG_SDL */

#endif /* CHE_IO_SDL_H */
