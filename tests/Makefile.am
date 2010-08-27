## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -Wall -Werror
# AM_CPPFLAGS = -Wall -Werror -fprofile-generate -ftest-coverage
# AM_LDFLAGS = -fprofile-generate -lgcov

TESTS = \
  check_check \
  check_vector \
  check_object \
  check_entity
check_PROGRAMS = \
  check_check \
  check_vector \
  check_object \
  check_entity

check_check_SOURCES = \
  check_check.c \
  check_line.c \
  check_float.c \
  check_turtle.c \
  check_video.c \
  check_time.c \
  $(top_builddir)/src/turtle.c \
  $(top_builddir)/src/point.c \
  $(top_builddir)/src/cpv.c \
  $(top_builddir)/src/object.c \
  $(top_builddir)/src/timer.c \
  $(top_builddir)/src/video.c
check_check_CFLAGS = @CHECK_CFLAGS@
check_check_LDADD = \
  $(top_builddir)/src/libline.la \
  $(top_builddir)/src/libvector.la \
  $(top_builddir)/src/libfcmp.la \
  -lm -lGL -lSDL \
  @CHECK_LIBS@

check_vector_SOURCES = \
  check_vector.c \
  $(top_builddir)/src/cpv.c
check_vector_CFLAGS = @CHECK_CFLAGS@
check_vector_LDADD = @CHECK_LIBS@

check_object_SOURCES = \
  check_object.c \
  object_debug.c \
  $(top_builddir)/src/object.c \
  $(top_builddir)/src/cpv.c
check_object_CFLAGS = @CHECK_CFLAGS@
check_object_LDADD = @CHECK_LIBS@

check_entity_SOURCES = \
  check_entity.c \
  component_debug.c \
  $(top_builddir)/src/cpv.c \
  $(top_builddir)/src/object.c \
  $(top_builddir)/src/entity.c
check_entity_CFLAGS = @CHECK_CFLAGS@
check_entity_LDADD = @CHECK_LIBS@
