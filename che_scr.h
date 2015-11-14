#ifndef CHE_SCR_H
#define CHE_SCR_H

#include <stdbool.h>
#include "che_scr_platform.h"

int che_scr_init(che_scr_t *s, int witdth, int height);

/* Return true if a collision has happened */
bool che_scr_draw_sprite(che_scr_t *s, uint8_t *buf, int h, int x, int y);

void che_scr_clear(che_scr_t *s);

void che_scr_draw_screen(che_scr_t *s);

void che_scr_end(che_scr_t *s);

#endif /* CHE_SCR_H */
