#include <che_scr.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CHE_SCR_TEST

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

/* TODO: Maybe this function can be in general code, not platform code */
bool che_scr_draw_sprite(che_scr_t *s, uint8_t *buf, int h, int x, int y)
{
	uint8_t *spr_ptr;
	uint8_t spr_data[16]; /* Height of 16 at most */
	int x_cut, y_cut, h_real;
	int i;

	/* Wrap coordinates around the screen */
	if (x >= s->w)
		x = x % s->w;
	if (y >= s->h)
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
		int rsh = x & 7;
		int pos = s->w * (y + i) + (x >> 3);
		uint8_t spr_byte = (spr_ptr[i] >> rsh);
		uint8_t xor_byte = s->data[pos] ^ spr_byte;
			
		if (!collision) {
			uint8_t or_byte = s->data[pos] | spr_byte;
			collision = or_byte != xor_byte;
		}
		s->data[pos] = xor_byte;
		if (!x_cut && rsh) {
			int lsh = 8 - rsh;
			spr_byte = (spr_ptr[i] << lsh);
			xor_byte = s->data[pos + 1] ^ spr_byte;
			if (!collision) {
				uint8_t or_byte = s->data[pos + 1] | spr_byte;
				collision = or_byte != xor_byte;
			}
			s->data[pos + 1] = xor_byte;
		}
	}

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
		che_scr_draw_sprite(s, &sprite, 1, x, x);
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
			if ((s->data[y * s->w + (x >> 3)] >> (7 - (x & 7))) & 1)
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
