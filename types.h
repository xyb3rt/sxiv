#ifndef TYPES_H
#define TYPES_H

typedef enum {
	false,
	true
} bool;

typedef enum {
	BO_BIG_ENDIAN,
	BO_LITTLE_ENDIAN
} byteorder_t;

typedef enum {
	MODE_IMAGE,
	MODE_THUMB
} appmode_t;

typedef enum {
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN
} direction_t;

typedef enum {
	FLIP_HORIZONTAL,
	FLIP_VERTICAL
} flipdir_t;

typedef enum {
	SCALE_DOWN,
	SCALE_FIT,
	SCALE_WIDTH,
	SCALE_HEIGHT,
	SCALE_ZOOM
} scalemode_t;

typedef enum {
	CURSOR_ARROW,
	CURSOR_NONE,
	CURSOR_HAND,
	CURSOR_WATCH
} cursor_t;

typedef struct {
	const char *name; /* as given by user */
	const char *path; /* always absolute */
	const char *base;
	bool loaded;
} fileinfo_t;

/* timeouts in milliseconds: */
enum {
	TO_REDRAW_RESIZE = 75,
	TO_REDRAW_THUMBS = 200,
	TO_CURSOR_HIDE   = 1200
};

typedef void (*timeout_f)(void);

#endif /* TYPES_H */
