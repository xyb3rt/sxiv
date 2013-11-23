#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([sxiv],[1.1.2],[bug@report.address])
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-extra-portability])
AC_CONFIG_SRCDIR([src/commands.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([Imlib2], [imlib_image_get_height])
AC_CHECK_LIB([X11], [XSetWMProtocols])
AC_CHECK_LIB([gif], [DGifOpenFileName])

PKG_CHECK_MODULES([GUILE], [guile-2.0])
GUILE_FLAGS
AM_PROG_CC_C_O

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h locale.h stdlib.h string.h sys/time.h unistd.h utime.h X11/keysym.h Imlib2.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRCOLL
AC_CHECK_FUNCS([dup2 getcwd gettimeofday memmove memset mkdir select setlocale strchr strrchr utime])

AC_DEFINE([GIT_VERSION], ["git-20131005"], [git version])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 conf/Makefile])
AC_OUTPUT
