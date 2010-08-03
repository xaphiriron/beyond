## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -Wall -Werror
# AM_CPPFLAGS = -Wall -Werror -fprofile-generate -ftest-coverage
# AM_LDFLAGS = -fprofile-generate -lgcov

TESTS = check_check
check_PROGRAMS = check_check
check_check_SOURCES = \
  check_check.c \
  check_line.c \
  check_float.c \
  check_turtle.c \
  check_entity.c \
  check_video.c \
  check_time.c \
  entitydebug.c \
  $(top_builddir)/src/turtle.c \
  $(top_builddir)/src/point.c \
  $(top_builddir)/src/list.c \
  $(top_builddir)/src/timer.c \
  $(top_builddir)/src/entity.c \
  $(top_builddir)/src/video.c
check_check_CFLAGS = @CHECK_CFLAGS@
check_check_LDADD = \
  $(top_builddir)/src/libline.la \
  $(top_builddir)/src/libvector.la \
  $(top_builddir)/src/libfcmp.la \
  -lm -lGL -lSDL \
  @CHECK_LIBS@