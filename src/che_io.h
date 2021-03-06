#ifndef CHE_IO_H
#define CHE_IO_H

#include <stdint.h>
#include <stdbool.h>

struct che_io_t;
typedef struct che_io_t che_io_t;

typedef struct che_io_ops_t
{
	int (*init)(che_io_t *io);

	int  (*scr_extended)(che_io_t *io, bool extended);

	/* Return true if a collision has happened */
	bool (*scr_sprite)(che_io_t *io, uint8_t *buf, int h, int x, int y);

	void (*scr_clear)(che_io_t *io);

	void (*scr_scroll_down)(che_io_t *io, int lines);

	void (*scr_scroll_left)(che_io_t *io);

	void (*scr_scroll_right)(che_io_t *io);

	void (*scr_render)(che_io_t *io);

	void (*scr_flip)(che_io_t *io);

	uint16_t (*keymask_get)(che_io_t *io);

	void (*tone_start)(che_io_t *io);

	void (*tone_stop)(che_io_t *io);

	void (*end)(che_io_t *io);
} che_io_ops_t;

struct che_io_t
{
	const che_io_ops_t *ops;
	const char         *name;
};

static inline
void che_io_obj_init(che_io_t *io, const che_io_ops_t *ops, const char *name)
{
	io->ops = ops;
	io->name = name;
}

static inline
int che_io_init(che_io_t *io)
{
	return io->ops->init(io);
}

static inline
int che_io_scr_extended(che_io_t *io, bool extended)
{
	if (!io->ops->scr_extended)
		return -1;
	return io->ops->scr_extended(io, extended);
}

static inline
bool che_io_scr_sprite(che_io_t *io, uint8_t *buf, int h, int x, int y)
{
	return io->ops->scr_sprite(io, buf, h, x, y);
}

static inline
void che_io_scr_clear(che_io_t *io)
{
	io->ops->scr_clear(io);
}

static inline
void che_io_scr_scroll_down(che_io_t *io, int lines)
{
	if (io->ops->scr_scroll_down)
		io->ops->scr_scroll_down(io, lines);
	/* TODO: fail if not */
}

static inline
void che_io_scr_scroll_left(che_io_t *io)
{
	if (io->ops->scr_scroll_left)
		io->ops->scr_scroll_left(io);
	/* TODO: fail if not */
}

static inline
void che_io_scr_scroll_right(che_io_t *io)
{
	if (io->ops->scr_scroll_right)
		io->ops->scr_scroll_right(io);
	/* TODO: fail if not */
}

static inline
void che_io_scr_render(che_io_t *io)
{
	io->ops->scr_render(io);
}

static inline
void che_io_scr_flip(che_io_t *io)
{
	io->ops->scr_flip(io);
}

static inline
uint16_t che_io_keymask_get(che_io_t *io)
{
	return io->ops->keymask_get(io);
}

static inline
void che_io_tone_start(che_io_t *io)
{
	if (io->ops->tone_start)
		io->ops->tone_start(io);
}

static inline
void che_io_tone_stop(che_io_t *io)
{
	if (io->ops->tone_stop)
		io->ops->tone_stop(io);
}

static inline
void che_io_end(che_io_t *io)
{
	io->ops->end(io);
}
#endif /* CHE_IO_H */
