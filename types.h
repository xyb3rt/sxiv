#ifndef TYPES_H
#define TYPES_H

typedef enum {
	MODE_NORMAL = 0,
	MODE_THUMBS
} appmode_t;

typedef struct {
	char key;
	int reload;
	const char *cmdline;
} command_t;

typedef enum {
	DIR_LEFT = 0,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN
} direction_t;

typedef enum {
	SCALE_DOWN = 0,
	SCALE_FIT,
	SCALE_ZOOM
} scalemode_t;

typedef enum {
	CURSOR_ARROW = 0,
	CURSOR_NONE,
	CURSOR_HAND,
	CURSOR_WATCH
} cursor_t;

#endif /* TYPES_H */
