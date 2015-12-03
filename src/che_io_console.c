#include <che_io_console.h>
#include <che_util.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef CHE_LINUX
#define CHE_IO_CONSOLE_LINUX
#endif /* CHE_LINUX */

#ifdef CHE_IO_CONSOLE_LINUX
#include <termios.h>

#define STDIN_FILENO fileno(stdin)

static const uint8_t che_key_map[16] = {
	'x' /* X */,
	'1' /* 1 */, '2' /* 2 */, '3' /* 3 */,
	'q' /* Q */, 'w' /* W */, 'e' /* E */,
	'a' /* A */, 's' /* S */, 'd' /* D */,
	'z' /* Z */, 'c' /* C */, '4' /* 4 */,
	'r' /* R */, 'f' /* F */, 'v' /* V */
};

static void che_io_console_nonblock(bool enable)
{
	struct termios ttystate;

	/* Get the terminal state */
	tcgetattr(STDIN_FILENO, &ttystate);

	if (enable) {
		/* Turn off canonical mode */
		ttystate.c_lflag &= ~ICANON;
		ttystate.c_lflag &= ~ECHO;
		ttystate.c_cc[VMIN] = 1;
	} else {
		/* Turn on canonical mode */
		ttystate.c_lflag |= ICANON;
		ttystate.c_lflag |= ECHO;
	}
	/* Set the terminal attributes */
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

static int che_io_console_kbhit()
{
	struct timeval tv;
	fd_set fds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
	select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &fds);
}
#endif

static
uint16_t che_io_console_keymask_get(che_io_t *io)
{
	#ifdef CHE_IO_CONSOLE_LINUX
	int i, c;
	
	c = che_io_console_kbhit();
	if (!c)
		return 0;

	c = fgetc(stdin);
	for (i = 0; i < 16; i++)
		if (che_key_map[i] == c)
			return 1 << i;
	#endif /* CHE_IO_CONSOLE_LINUX */
	return 0;
}

/* #define CHE_SCR_TEST */

/* TODO: Maybe this function can be in general code, not platform code */
/* TODO: Test collision code */
static
bool che_io_console_sprite(che_io_t *io, uint8_t *buf, int h, int x, int y)
{
	che_io_console_t *c = che_containerof(io, che_io_console_t, io);
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
void che_io_console_clear(che_io_t *io)
{
	che_io_console_t *c = che_containerof(io, che_io_console_t, io);
	memset(c->data, 0, (c->w >> 3) * c->h);
	c->changed = true;

	#ifdef CHE_SCR_TEST
	uint8_t sprite = 0xff;
	int x;
	for (x = 0; x < c->w; x++)
		che_io_console_sprite(io, &sprite, 1, x + 16, x);
	#endif
}

static
int che_io_console_io_init(che_io_t *io, int width, int height)
{
	che_io_console_t *c = che_containerof(io, che_io_console_t, io);
	if (width % 8)
		return -1;
	c->data = malloc((width >> 3) * height);
	c->w = width;
	c->h = height;
	c->x_wrap = true;
	c->y_wrap = true;
	che_io_console_clear(io);
	#ifdef CHE_IO_CONSOLE_LINUX
	che_io_console_nonblock(true);
	#endif
	return 0;
}

static
void che_io_console_render(che_io_t *io)
{
}

static
void che_io_console_flip(che_io_t *io)
{
	che_io_console_t *c = che_containerof(io, che_io_console_t, io);
	int x, y;

	if (!c->changed)
		return;
	c->changed = false;
	
	/* Clear the screen */
	printf("\e[1;1H\e[2J\n");

	for (y = 0; y < c->h; y++) {
		char line[256];
		int pos = 0;
		for (x = 0; x < c->w; x++) {
			char ch;
			if ((c->data[y * (c->w >> 3) + (x >> 3)] >> (7 - (x & 7))) & 1)
				ch = 'O';
			else
				ch = ' ';
			line[pos++] = ch;
		}
		line[pos] = 0;
		printf("%s\n", line);
	}
	fflush(stdout);
}

static
void che_io_console_end(che_io_t *io)
{
	che_io_console_t *c = che_containerof(io, che_io_console_t, io);
	free(c->data);
	#ifdef CHE_IO_CONSOLE_LINUX
	che_io_console_nonblock(false);
	#endif
}

static const che_io_ops_t che_io_console_ops =
{
	.init        = che_io_console_io_init,
	.scr_sprite  = che_io_console_sprite,
	.scr_clear   = che_io_console_clear,
	.scr_render  = che_io_console_render,
	.scr_flip    = che_io_console_flip,
	.keymask_get = che_io_console_keymask_get,
	.end         = che_io_console_end,
};

che_io_t *che_io_console_init(che_io_console_t *c)
{
	che_io_obj_init(&c->io, &che_io_console_ops, "console");
	return &c->io;
}
