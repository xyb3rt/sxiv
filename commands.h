typedef struct {
	KeySym ksym;
	Bool reload;
	const char *cmdline;
} command_t;

static command_t commands[] = {
	/* ctrl-...     reload?  command, '#' is replaced by filename */
	{  XK_comma,    True,    "jpegtran -rotate 270 -copy all -outfile # #" },
	{  XK_period,   True,    "jpegtran -rotate 90 -copy all -outfile # #" },
	{  XK_less,     True,    "mogrify -rotate -90 #" },
	{  XK_greater,  True,    "mogrify -rotate +90 #" }
};
