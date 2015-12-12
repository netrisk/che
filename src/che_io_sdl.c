#include <che_io_sdl.h>

#ifdef CHE_USE_SDL

#include <che_util.h>
#include <SDL.h>
#include <SDL_audio.h>

#define CHE_IO_SDL_FREQUENCY 11025

/* This is a 525 Hz tone sampled at 11025 Hz */
static uint16_t che_io_sdl_tone_chunk[CHE_IO_SDL_SAMPLES_NUM] = {
	0, 9658, 18458, 25618, 30502, 32675, 31945, 28377, 22287, 14217, 4884,
	-4884, -14217, -22287, -28377, -31945, -32675, -30502, -25618, -18458,
	-9658 };

static const uint8_t che_key_map[16] = {
	27 /* X */,
	30 /* 1 */, 31 /* 2 */, 32 /* 3 */,
	20 /* Q */, 26 /* W */,  8 /* E */,
	 4 /* A */, 22 /* S */,  7 /* D */,
	29 /* Z */,  6 /* C */, 33 /* 4 */,
	21 /* R */,  9 /* F */, 25 /* V */
};

static
uint16_t che_io_sdl_keymask_get(che_io_t *io)
{
	che_io_sdl_t *s = che_containerof(io, che_io_sdl_t, io);
	int i;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		/* Keyboard event */
		case SDL_KEYDOWN:
		for (i = 0; i < 16; i++)
			if (event.key.keysym.scancode == che_key_map[i])
				s->keymask |= 1 << i;
		break;
		case SDL_KEYUP:
		for (i = 0; i < 16; i++)
			if (event.key.keysym.scancode == che_key_map[i])
				s->keymask &= ~(1 << i);
		break;
		/* SDL_QUIT event (window close) */
		case SDL_QUIT:
			exit(0);
			break;
		default:
		break;
		}
	}
	return s->keymask;
}

/* #define CHE_SCR_TEST */

/* TODO: Maybe this function can be in general code, not platform code */
/* TODO: Test collision code */
static
bool che_io_sdl_sprite(che_io_t *io, uint8_t *buf, int h, int x, int y)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	bool collision = false;
	int h_visible;
	int i;

	/* Wrap coordinates around the screen */
	if (x >= c->w) {
		if (!c->x_wrap)
			return false;
		x = x % c->w;
	}
	if (y >= c->h) {
		if (!c->y_wrap)
			return false;
		y = y % c->h;
	}

	/* Cut the sprite if necessary */
	if (y + h < c->h)
		h_visible = h;
	else
		h_visible = c->h - y;

	/* TODO: if y wrapping is enabled maybe we should draw the bottom
	         of the sprite at the top of the screen */
	/* Draw content in memory */
	for (i = 0; i < h_visible; i++) {
		/* Leftmost byte bits */
		int rsh = x & 7;
		int pos = (c->w >> 3) * (y + i) + (x >> 3);
		uint8_t spr_byte = (buf[i] >> rsh);
		uint8_t xor_byte = c->data[pos] ^ spr_byte;
		if (!collision) {
			uint8_t or_byte = c->data[pos] | spr_byte;
			collision = or_byte != xor_byte;
		}
		c->data[pos] = xor_byte;
		/* Righmost byte bits */
		if (rsh) {
			pos = pos + 1;
			if (x >= c->w - 8) {
				if (!c->x_wrap)
					goto end;
				pos -= c->w >> 3;
			}
			int lsh = 8 - rsh;
			spr_byte = (buf[i] << lsh);
			xor_byte = c->data[pos] ^ spr_byte;
			if (!collision) {
				uint8_t or_byte = c->data[pos] | spr_byte;
				collision = or_byte != xor_byte;
			}
			c->data[pos] = xor_byte;
		}
	}

end:
	c->draw_pending = true;
	return collision;
}

static
void che_io_sdl_clear_internal(che_io_sdl_t *s)
{
	memset(s->data, 0, (s->w >> 3) * s->h);
	s->draw_pending = true;
}

static
void che_io_sdl_clear(che_io_t *io)
{
	che_io_sdl_t *s = che_containerof(io, che_io_sdl_t, io);
	che_io_sdl_clear_internal(s);

	#ifdef CHE_SCR_TEST
	uint8_t sprite = 0xff;
	int x;
	for (x = 0; x < c->w; x++)
		che_io_sdl_sprite(io, &sprite, 1, x + 16, x);
	#endif
}

/* Sound system */

static
void che_io_sdl_audio_callback(void *param, uint8_t *_stream, int length)
{
	che_io_sdl_t *s = (che_io_sdl_t *)param;
	int16_t *stream = (int16_t *)_stream;
	
	/* TODO: this could be done much better, because with this API and
	         implementation the tone has to be multiple of 256 samples
	         and even the duration depends on when the playing variable
	         is set */

	SDL_LockAudio();
	if (s->playing) {
		int i;
		int samples_rem = length >> 1;
		for (i = 0; i < samples_rem; i++) {
			stream[i] = s->samples[s->cur_sample++];
			if (s->cur_sample >= 21)
				s->cur_sample = 0;
		}
	} else {
		memset(_stream, 0, length);
	}
	SDL_UnlockAudio();
}

static void che_io_sdl_set_tone_generate(che_io_sdl_t *s, int volume)
{
	int i;
	for (i = 0; i < CHE_IO_SDL_SAMPLES_NUM; i++)
		s->samples[i] = (che_io_sdl_tone_chunk[i] * volume) / 100;
}

static void che_io_sdl_screen_alloc(che_io_sdl_t *s, int width, int height,
                                    int pix_size)
{
	/* Screen and pixel size */
	s->w = width;
	s->h = height;
	s->pix_size = pix_size;

	/* Memory management */
	s->data = malloc((s->w >> 3) * s->h);
	s->ph_prev_bitmap = malloc((s->w >> 3) * s->h);
	s->ph_map = malloc(s->w * s->h);
	memset(s->ph_prev_bitmap, 0, (s->w >> 3) * s->h);
	memset(s->ph_map, 0, s->w * s->h);
	che_io_sdl_clear_internal(s);
}

static int che_io_sdl_screen_free(che_io_sdl_t *s)
{
	free(s->data);
	free(s->ph_prev_bitmap);
	free(s->ph_map);
	return 0;
}

static
int che_io_sdl_io_init(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	c->x_wrap = true;
	c->y_wrap = true;
	c->keymask = 0;
	c->color_r = 20;
	c->color_g = 255;
	c->color_b = 20;

	/* Allocate screen for normal mode by default */
	che_io_sdl_screen_alloc(c, 64, 32, 10);	

	/* Video */
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(640, 320, 0, &c->window, &c->renderer);
	SDL_SetWindowTitle(c->window, "che CHIP-8 emulator");

	/* Audio */
	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioSpec desiredSpec;
	SDL_AudioSpec obtainedSpec;

	desiredSpec.freq = CHE_IO_SDL_FREQUENCY;
	desiredSpec.format = AUDIO_S16SYS;
	desiredSpec.channels = 1;
	desiredSpec.samples = 256;
	desiredSpec.callback = che_io_sdl_audio_callback;
	desiredSpec.userdata = c;

	/* Phosphor support */
	c->ph_level = 1;
	c->ph_changed = false;

	/* TODO: look for result */
	SDL_OpenAudio(&desiredSpec, &obtainedSpec);
	che_io_sdl_set_tone_generate(c, 30);
	c->cur_sample = 0;
	c->playing = false;
	SDL_PauseAudio(0);
	return 0;
}

static
int che_io_sdl_extended(che_io_t *io, bool extended)
{
	che_io_sdl_t *s = che_containerof(io, che_io_sdl_t, io);
	int w, h, pix_size;
	if (extended) {
		w = 128;
		h = 64;
		pix_size = 5;
	} else {
		w = 64;
		h = 32;
		pix_size = 10;
	}
	che_io_sdl_screen_free(s);
	che_io_sdl_screen_alloc(s, w, h, pix_size);
	return 0;
}

static void che_io_sdl_tone_start(che_io_t *io)
{
	che_io_sdl_t *s = che_containerof(io, che_io_sdl_t, io);

	/* TODO: this is not good, as timing is not exact */

	SDL_LockAudio();
	s->playing = true;
	SDL_UnlockAudio();
}

static void che_io_sdl_tone_stop(che_io_t *io)
{
	che_io_sdl_t *s = che_containerof(io, che_io_sdl_t, io);

	/* TODO: this is not good, as timing is not exact */

	SDL_LockAudio();
	s->playing = false;
	SDL_UnlockAudio();
}

static
void che_io_sdl_render_phosphor(che_io_sdl_t *s)
{
	SDL_Rect rect = { 0, 0, s->pix_size, s->pix_size };
	uint8_t *ph_pix;
	bool now_on;
	bool prev_on;
	int x, y;

	/* Check if phosphor is activated */
	if (!s->ph_level)
		return;

	/* Check if any pixels have phosphor left */
	s->ph_changed = false;
	for (y = 0; y < s->h; y++) {
		for (x = 0; x < s->w; x++) {
			ph_pix = &s->ph_map[(y * s->w) + x];
			now_on = (s->data[y * (s->w >> 3) + (x >> 3)] >> (7 - (x & 7))) & 1;
			prev_on = (s->ph_prev_bitmap[y * (s->w >> 3) + (x >> 3)] >> (7 - (x & 7))) & 1;
			if (prev_on && !now_on)
				*ph_pix = s->ph_level;
			if (*ph_pix) {
				rect.x = x * s->pix_size;
				rect.y = y * s->pix_size;
				uint8_t intens_r = ((*ph_pix * s->color_r) / (s->ph_level + 1));
				uint8_t intens_g = ((*ph_pix * s->color_g) / (s->ph_level + 1));
				uint8_t intens_b = ((*ph_pix * s->color_b) / (s->ph_level + 1));
				SDL_SetRenderDrawColor(s->renderer, intens_r,
				                       intens_g, intens_b, 255);
				SDL_RenderFillRect(s->renderer, &rect);
				(*ph_pix)--;
				s->ph_changed = true;
			}
		}
	}
	memcpy(s->ph_prev_bitmap, s->data, (s->w >> 3) * s->h);
}

static
void che_io_sdl_render(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	int x, y;

	if (!c->draw_pending)
		return;

	/* Clear the screen */
	SDL_SetRenderDrawColor(c->renderer, 0, 0, 0, 255);
	SDL_RenderClear(c->renderer);

	/* Render phosphor */
	che_io_sdl_render_phosphor(c);

	/* Draw actual pixels */
	SDL_Rect rect = { 0, 0, c->pix_size, c->pix_size };
	SDL_SetRenderDrawColor(c->renderer, c->color_r, c->color_g,
	                       c->color_b, 255); 
	for (y = 0; y < c->h; y++) {
		for (x = 0; x < c->w; x++) {
			if ((c->data[y * (c->w >> 3) + (x >> 3)] >> (7 - (x & 7))) & 1) {
				rect.x = x * c->pix_size;
				rect.y = y * c->pix_size;
				SDL_RenderFillRect(c->renderer, &rect);
			}
		}
	}
}

static
void che_io_sdl_flip(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	if (!c->draw_pending && !c->ph_changed)
		return;
	c->draw_pending = c->ph_changed;
	SDL_RenderPresent(c->renderer);
}

static
void che_io_sdl_end(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	che_io_sdl_screen_free(c);
	SDL_CloseAudio();
}

static const che_io_ops_t che_io_sdl_ops =
{
	.init         = che_io_sdl_io_init,
	.scr_extended = che_io_sdl_extended,
	.scr_sprite   = che_io_sdl_sprite,
	.scr_clear    = che_io_sdl_clear,
	.scr_render   = che_io_sdl_render,
	.scr_flip     = che_io_sdl_flip,
	.keymask_get  = che_io_sdl_keymask_get,
	.tone_start   = che_io_sdl_tone_start,
	.tone_stop    = che_io_sdl_tone_stop,
	.end          = che_io_sdl_end,
};

che_io_t *che_io_sdl_init(che_io_sdl_t *c)
{
	che_io_obj_init(&c->io, &che_io_sdl_ops, "sdl");
	return &c->io;
}

#endif /* CHE_USE_SDL */
