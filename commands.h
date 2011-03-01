#define FILENAME (const char*) 0x1

typedef struct {
	KeySym ksym;
	const char **cmdline;
	Bool reload;
} command_t;

static const char *cmdline_1[] = {
	"jpegtran", "-rotate", "270", "-copy", "all", "-outfile", FILENAME,
	FILENAME,	NULL };

static const char *cmdline_2[] = {
	"jpegtran", "-rotate",  "90", "-copy", "all", "-outfile", FILENAME,
	FILENAME, NULL };

static const char *cmdline_3[] = {
	"mogrify", "-rotate", "-90", FILENAME, NULL };

static const char *cmdline_4[] = {
	"mogrify", "-rotate", "+90", FILENAME, NULL };

static command_t commands[] = {
	/* key            command-line      reload? */
	{  XK_a,          cmdline_1,        True },
	{  XK_s,          cmdline_2,        True },
	{  XK_A,          cmdline_3,        True },
	{  XK_S,          cmdline_4,        True },
};
