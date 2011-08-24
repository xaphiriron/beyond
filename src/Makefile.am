## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -Wall -Werror -O3
#AM_CPPFLAGS = -Wall -Werror -O3 -DMEM_DEBUG

#AM_CPPFLAGS = -Wall -Werror -fprofile-generate -ftest-coverage
#AM_LDFLAGS = -fprofile-generate -lgcov

lib_LTLIBRARIES = libfcmp.la libvector.la libquat.la libline.la libsph.la
libfcmp_la_SOURCES = fcmp.c fcmp.h
libvector_la_SOURCES = vector.c vector.h
libquat_la_SOURCES = quaternion.c quaternion.h
libline_la_SOURCES = line.c line.h
libsph_la_SOURCES = sph.c sph.h
#libline_la_LIBADD = libvector.la


bin_PROGRAMS = beyond
#bin_PROGRAMS = beyond logo
beyond_SOURCES = \
  xph_memory.c xph_memory.h \
	xph_log.c xph_log.h \
	path.c path.h \
  shapes.c shapes.h \
	turtle3d.c turtle3d.h \
	texture.c texture.h \
  dynarr.c dynarr.h \
  lsystem.c lsystem.h \
  object.c object.h	\
  video.c video.h	\
  timer.c timer.h	\
  accumulator.c accumulator.h \
  entity.c entity.h	\
	map.c map.h \
	worldgen.c worldgen.h \
	worldgen_graph.c worldgen_graph.h \
  component_position.c component_position.h	\
  component_integrate.c component_integrate.h	\
  component_camera.c component_camera.h		\
  component_input.c component_input.h		\
	component_ui.c component_ui.h \
  component_walking.c component_walking.h	\
  component_plant.c component_plant.h		\
  hex_utility.h hex_utility.c \
	sprite.c sprite.h \
	font.c font.h \
	system.c system.h \
	beyond.c
beyond_LDADD = \
	-lm -lGL -lSDL -lglpng \
	libline.la \
	libsph.la \
	libvector.la \
	libquat.la \
	libfcmp.la

# logo_SOURCES = \
#   xph_memory.c xph_memory.h \
#   cpv.c cpv.h		\
#   object.c object.h	\
#   timer.c timer.h	\
#   video.c video.h	\
#   input.c input.h	\
#   accumulator.c accumulator.h \
#   physics.c physics.h	\
#   point.c point.h	\
#   world.c world.h	\
#   hex.c hex.h		\
#   turtle.c turtle.h	\
#   turtledraw.c turtledraw.h \
#   system.c system.h	\
#   logo.c
# logo_LDADD =	\
#   libline.la	\
#   libvector.la	\
#   libfcmp.la	\
#   -lm -lGL -lSDL