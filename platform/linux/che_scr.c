#include <che_scr.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* #define CHE_SCR_TEST */

int che_scr_init(che_scr_t *s, int width, int height)
{
	if (width % 8)
		return -1;
	s->data = malloc((width >> 3) * height);
	s->w = width;
	s->h = height;
	s->x_wrap = true;
	s->y_wrap = true;
	che_scr_clear(s);
	return 0;
}

/* TODO: Maybe this function can be in general code, not platform code */
/* TODO: Test collision code */
bool che_scr_draw_sprite(che_scr_t *s, uint8_t *buf, int h, int x, int y)
{
	bool collision = false;
	int h_visible;
	int i;

	/* Wrap coordinates around the screen */
	if (x >= s->w) {
		if (!s->x_wrap)
			return false;
		x = x % s->w;
	}
	if (y >= s->h) {
		if (!s->y_wrap)
			return false;
		y = y % s->h;
	}

	/* Cut the sprite if necessary */
	if (y + h < s->h)
		h_visible = h;
	else
		h_visible = s->h - y;

	/* TODO: if y wrapping is enabled maybe we should draw the bottom
	         of the sprite at the top of the screen */
	/* Draw content in memory */
	for (i = 0; i < h_visible; i++) {
		/* Leftmost byte bits */
		int rsh = x & 7;
		int pos = (s->w >> 3) * (y + i) + (x >> 3);
		uint8_t spr_byte = (buf[i] >> rsh);
		uint8_t xor_byte = s->data[pos] ^ spr_byte;
		if (!collision) {
			uint8_t or_byte = s->data[pos] | spr_byte;
			collision = or_byte != xor_byte;
		}
		s->data[pos] = xor_byte;
		/* Righmost byte bits */
		if (rsh) {
			pos = pos + 1;
			if (x >= s->w - 8) {
				if (!s->x_wrap)
					goto end;
				pos -= s->w >> 3;
			}
			int lsh = 8 - rsh;
			spr_byte = (buf[i] << lsh);
			xor_byte = s->data[pos] ^ spr_byte;
			if (!collision) {
				uint8_t or_byte = s->data[pos] | spr_byte;
				collision = or_byte != xor_byte;
			}
			s->data[pos] = xor_byte;
		}
	}

end:
	s->changed = true;
	return collision;
}

void che_scr_clear(che_scr_t *s)
{
	memset(s->data, 0, (s->w >> 3) * s->h);
	s->changed = true;

	#ifdef CHE_SCR_TEST
	uint8_t sprite = 0xff;
	int x;
	for (x = 0; x < s->w; x++)
		che_scr_draw_sprite(s, &sprite, 1, x + 16, x);
	#endif
}

void che_scr_draw_screen(che_scr_t *s)
{
	int x, y;

	if (!s->changed)
		return;
	s->changed = false;
	
	/* Clear the screen */
	printf("\e[1;1H\e[2J\n");

	for (y = 0; y < s->h; y++) {
		char line[256];
		int pos = 0;
		for (x = 0; x < s->w; x++) {
			char ch;
			if ((s->data[y * (s->w >> 3) + (x >> 3)] >> (7 - (x & 7))) & 1)
				ch = 'O';
			else
				ch = '.';
			line[pos++] = ch;
		}
		line[pos] = 0;
		printf("%s\n", line);
	}
	fflush(stdout);
}

void che_scr_end(che_scr_t *s)
{
	free(s->data);
}
