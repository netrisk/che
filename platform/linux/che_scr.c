#include <che_scr.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int che_scr_init(che_scr_t *s, int width, int height)
{
	if (width % 8)
		return -1;
	s->data = malloc((width >> 3) * height);
	s->w = width;
	s->h = height;
	che_scr_clear(s);
	return 0;
}

/* TODO: Test this code, it's completely untested */
/* TODO: Maybe this function can general code, not platform code */
bool che_scr_draw_sprite(che_scr_t *s, uint8_t *buf, int h, int x, int y)
{
	uint8_t *spr_ptr;
	uint8_t spr_data[16]; /* Height of 16 at most */
	int x_cut, y_cut, h_real;
	int i;

	/* Wrap coordinates around the screen */
	if (x > s->w)
		x = x % s->w;
	if (y > s->h)
		y = y % s->h;

	/* Cut the sprite if necessary */
	x_cut = x + 8 - s->w;
	y_cut = y + h - s->h;
	if (x_cut < 0)
		x_cut = 0;
	if (y_cut < 0)
		y_cut = 0;
	h_real = h - y_cut;
	if (x_cut == 0 && y_cut == 0) {
		spr_ptr = buf;
	} else {
		spr_ptr = spr_data;
		for (i = 0; i < h_real; i++)
			spr_data[i] = (buf[i] >> x_cut) << x_cut;
	}

	/* Draw content in memory */
	bool collision = false;
	for (i = 0; i < h_real; i++) {
		int pos = s->w * (y + i) + x;
		uint8_t xor_byte = s->data[pos] ^ spr_ptr[i];
		if (!collision) {
			uint8_t or_byte = s->data[pos] | spr_ptr[i];
			collision = or_byte != xor_byte;
		}
		s->data[pos] = xor_byte;
	}

	s->changed = true;
	return collision;
}

void che_scr_clear(che_scr_t *s)
{
	memset(s->data, 0, (s->w >> 3) * s->h);
	s->changed = true;
}

void che_scr_draw_screen(che_scr_t *s)
{
	if (!s->changed)
		return;
	s->changed = false;
	printf("Screen changed\n");
}

void che_scr_end(che_scr_t *s)
{
	free(s->data);
}
