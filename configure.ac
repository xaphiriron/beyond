#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([beyond], [0.0.0.2], [xaxster@gmail.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror 1.10])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([GL], [glBegin])
AC_CHECK_LIB([SDL], [SDL_Init])

# Checks for header files.
AC_CHECK_HEADERS([float.h stdlib.h string.h limits.h sys/time.h])
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_TIME

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([atexit gettimeofday memmove memset sqrt])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])
AC_OUTPUT
