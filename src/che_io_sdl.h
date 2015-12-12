#ifndef CHE_IO_SDL_H
#define CHE_IO_SDL_H

#include <config.h>

#ifdef CHE_USE_SDL

#include <stdint.h>
#include <stdbool.h>
#include <che_io.h>
#include <SDL.h>

#define CHE_IO_SDL_SAMPLES_NUM 21

typedef struct che_io_sdl_t
{
	/* Base class */
	che_io_t io;

	/* Own members */
	uint8_t      *data;
	bool          draw_pending;
	int           w;
	int           h;
	uint8_t       pix_size;
	bool          extended;
	bool          x_wrap;
	bool          y_wrap;
	uint16_t      keymask;
	SDL_Window   *window;
	SDL_Renderer *renderer;
	uint8_t       color_r;
	uint8_t       color_g;
	uint8_t       color_b;

	/* Phosphor data */
	uint8_t       ph_level;
	bool          ph_changed;
	uint8_t      *ph_prev_bitmap;
	uint8_t      *ph_map;

	/* Audio */
	int     cur_sample;
	int16_t samples[CHE_IO_SDL_SAMPLES_NUM];
	bool    playing;
} che_io_sdl_t;

che_io_t *che_io_sdl_init(che_io_sdl_t *c);

#endif /* CHE_USE_SDL */

#endif /* CHE_IO_SDL_H */
