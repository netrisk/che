#include <che_io_sdl.h>

#ifdef CHE_CFG_SDL

#include <che_util.h>
#include <SDL.h>

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
	c->changed = true;
	return collision;
}

static
void che_io_sdl_clear(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	memset(c->data, 0, (c->w >> 3) * c->h);
	c->changed = true;

	#ifdef CHE_SCR_TEST
	uint8_t sprite = 0xff;
	int x;
	for (x = 0; x < c->w; x++)
		che_io_sdl_sprite(io, &sprite, 1, x + 16, x);
	#endif
}

static
int che_io_sdl_io_init(che_io_t *io, int width, int height)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	if (width % 8)
		return -1;
	c->data = malloc((width >> 3) * height);
	c->w = width;
	c->h = height;
	c->x_wrap = true;
	c->y_wrap = true;
	c->keymask = 0;
	che_io_sdl_clear(io);

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(640, 320, 0, &c->window, &c->renderer);

	return 0;
}

static
void che_io_sdl_render(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	int x, y;

	if (!c->changed)
		return;

	SDL_Rect rect = { 0, 0, 10, 10 };

	SDL_RenderClear(c->renderer);
	for (y = 0; y < c->h; y++) {
		for (x = 0; x < c->w; x++) {
			if ((c->data[y * (c->w >> 3) + (x >> 3)] >> (7 - (x & 7))) & 1) {
				rect.x = x * 10;
				rect.y = y * 10;
				SDL_RenderFillRect(c->renderer, &rect);
			}
		}
	}
}

static
void che_io_sdl_flip(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	if (!c->changed)
		return;
	c->changed = false;
	SDL_RenderPresent(c->renderer);
}

static
void che_io_sdl_end(che_io_t *io)
{
	che_io_sdl_t *c = che_containerof(io, che_io_sdl_t, io);
	free(c->data);
}

static const che_io_ops_t che_io_sdl_ops =
{
	.init        = che_io_sdl_io_init,
	.scr_sprite  = che_io_sdl_sprite,
	.scr_clear   = che_io_sdl_clear,
	.scr_render  = che_io_sdl_render,
	.scr_flip    = che_io_sdl_flip,
	.keymask_get = che_io_sdl_keymask_get,
	.end         = che_io_sdl_end,
};

che_io_t *che_io_sdl_init(che_io_sdl_t *c)
{
	che_io_obj_init(&c->io, &che_io_sdl_ops, "sdl");
	return &c->io;
}

#endif /* CHE_CFG_SDL */