#define _POSIX_C_SOURCE 200112L
#define _FEATURE_CONFIG

#include <stdio.h>

#include "config.h"

#define QUOTE(m) #m
#define PUT_MACRO(m) \
	printf(" -D%s=%s", #m, QUOTE(m))

int puts_if(const char *s, int c) {
	return c ? printf(" %s", s) : 0;
}

int main(int argc, char **argv) {
	int i;
	unsigned int n = 0;

	for (i = 1; i < argc; i++) {
		switch ((argv[i][0] != '-' || argv[i][2] != '\0') ? -1 : argv[i][1]) {
			case 'D':
				n += PUT_MACRO(EXIF_SUPPORT);
				n += PUT_MACRO(GIF_SUPPORT);
				break;
			case 'l':
				n += puts_if("-lexif", EXIF_SUPPORT);
				n += puts_if("-lgif",  GIF_SUPPORT);
				break;
			default:
				fprintf(stderr, "%s: invalid argument: %s\n", argv[0], argv[i]);
				return 1;
		}
	}
	if (n > 0)
		printf("\n");
	return 0;
}
