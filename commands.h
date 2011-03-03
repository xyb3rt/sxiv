typedef struct {
	KeySym ksym;
	Bool reload;
	const char *cmdline;
} command_t;

static command_t commands[] = {
	/* key          reload?  command, '#' is replaced by filename */
	{  XK_a,        True,    "jpegtran -rotate 270 -copy all -outfile # #" },
	{  XK_s,        True,    "jpegtran -rotate 90 -copy all -outfile # #" },
	{  XK_A,        True,    "mogrify -rotate -90 #" },
	{  XK_S,        True,    "mogrify -rotate +90 #" }
};
