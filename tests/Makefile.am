## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -Wall -Werror -fstrict-aliasing -O3
# AM_CPPFLAGS = -Wall -Werror -fprofile-generate -ftest-coverage
# AM_LDFLAGS = -fprofile-generate -lgcov

TESTS = \
  check_check \
  check_dynarr \
	check_lsystem \
	check_turtle3d \
  check_object \
  check_entity \
  check_tiles \
  check_worldpos
check_PROGRAMS = \
  check_check \
  check_dynarr \
	check_lsystem \
	check_turtle3d \
  check_object \
  check_entity \
  check_tiles \
  check_worldpos

check_check_SOURCES = \
  check_check.c \
  check_line.c \
  check_float.c \
  check_turtle.c \
  check_video.c \
  check_time.c \
  $(top_builddir)/src/turtle.c \
  $(top_builddir)/src/point.c \
  $(top_builddir)/src/dynarr.c \
	$(top_builddir)/src/xph_log.c \
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

check_dynarr_SOURCES = \
  check_dynarr.c \
  $(top_builddir)/src/dynarr.c \
	$(top_builddir)/src/xph_log.c
check_dynarr_CFLAGS = @CHECK_CFLAGS@
check_dynarr_LDADD = @CHECK_LIBS@

check_lsystem_SOURCES = \
	check_lsystem.c \
	$(top_builddir)/src/dynarr.c \
	$(top_builddir)/src/xph_log.c \
	$(top_builddir)/src/lsystem.c
check_lsystem_CFLAGS = @CHECK_CFLAGS@
check_lsystem_LDADD = @CHECK_LIBS@

check_turtle3d_SOURCES = \
	check_turtle3d.c \
	$(top_builddir)/src/turtle3d.c \
	$(top_builddir)/src/dynarr.c \
	$(top_builddir)/src/xph_log.c
check_turtle3d_CFLAGS = @CHECK_CFLAGS@
check_turtle3d_LDADD = \
	$(top_builddir)/src/libquat.la \
	$(top_builddir)/src/libvector.la \
	$(top_builddir)/src/libfcmp.la \
	@CHECK_LIBS@

check_object_SOURCES = \
  check_object.c \
  object_debug.c \
  $(top_builddir)/src/object.c \
	$(top_builddir)/src/xph_log.c \
  $(top_builddir)/src/dynarr.c
check_object_CFLAGS = @CHECK_CFLAGS@
check_object_LDADD = @CHECK_LIBS@

check_entity_SOURCES = \
  check_entity.c \
  component_debug.c \
  $(top_builddir)/src/dynarr.c \
	$(top_builddir)/src/xph_log.c \
  $(top_builddir)/src/object.c \
  $(top_builddir)/src/timer.c \
  $(top_builddir)/src/entity.c
check_entity_CFLAGS = @CHECK_CFLAGS@
check_entity_LDADD = @CHECK_LIBS@

check_tiles_SOURCES = \
  check_tiles.c \
  $(top_builddir)/src/hex.c \
  $(top_builddir)/src/hex_utility.c \
  $(top_builddir)/src/dynarr.c \
	$(top_builddir)/src/xph_log.c
check_tiles_CFLAGS = @CHECK_CFLAGS@
check_tiles_LDADD = \
  $(top_builddir)/src/libvector.la \
  $(top_builddir)/src/libfcmp.la \
  @CHECK_LIBS@

check_worldpos_SOURCES = \
  check_worldpos.c \
  $(top_builddir)/src/world_position.c \
  $(top_builddir)/src/hex_utility.c
check_worldpos_CFLAGS = @CHECK_CFLAGS@
check_worldpos_LDADD =  \
  $(top_builddir)/src/libvector.la \
  $(top_builddir)/src/libfcmp.la \
  @CHECK_LIBS@
