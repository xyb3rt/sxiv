#define _POSIX_C_SOURCE 200112L
#define _FEATURE_CONFIG

#include <stdio.h>
#include <string.h>

#include "config.h"

#define QUOTE(m) #m
#define PUT_MACRO(m) \
	printf("%s-D%s=%s", n++ ? " " : "", #m, QUOTE(m))

int n = 0;

inline void puts_if(const char *s, int c) {
	if (c)
		printf("%s%s", n++ ? " " : "", s);
}

inline void endl() {
	if (n) {
		printf("\n");
		n = 0;
	}
}

int main(int argc, char **argv) {
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-D")) {
			PUT_MACRO(EXIF_SUPPORT);
			PUT_MACRO(GIF_SUPPORT);
			endl();
		} else if (!strcmp(argv[i], "-l")) {
			puts_if("-lexif", EXIF_SUPPORT);
			puts_if("-lgif",  GIF_SUPPORT);
			endl();
		} else {
			fprintf(stderr, "%s: invalid argument: %s\n", argv[0], argv[i]);
			return 1;
		}
	}
	return 0;
}
