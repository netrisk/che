#ifndef CHE_SCR_PLATFORM_H
#define CHE_SCR_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct che_scr_t
{
	uint8_t *data;
	bool     changed;
	int      w;
	int      h;
	bool     x_wrap;
	bool     y_wrap;
} che_scr_t;

#endif /* CHE_SCR_PLATFORM_H */

