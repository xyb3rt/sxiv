#define _POSIX_C_SOURCE 200112L
#define _FEATURE_CONFIG

#include <stdio.h>

#include "config.h"

int n = 0;

inline void put_lib_flag(const char *flag, int needed) {
	if (needed)
		printf("%s%s", n++ ? " " : "", flag);
}

int main(int argc, char **argv) {
	put_lib_flag("-lexif", EXIF_SUPPORT);
	put_lib_flag("-lgif",  GIF_SUPPORT);

	if (n)
		printf("\n");

	return 0;
}
