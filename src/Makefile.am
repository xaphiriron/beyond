## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -Wall -Werror -O3
#AM_CPPFLAGS = -Wall -Werror -O3 -DMEM_DEBUG
# AM_CPPFLAGS = -Wall -Werror -fprofile-generate -ftest-coverage
# AM_LDFLAGS = -fprofile-generate -lgcov

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
  shapes.c shapes.h \
	turtle3d.c turtle3d.h \
  dynarr.c dynarr.h \
  lsystem.c lsystem.h \
  object.c object.h	\
  video.c video.h	\
  timer.c timer.h	\
  accumulator.c accumulator.h \
  entity.c entity.h	\
  world_position.c world_position.h \
	worldgen.c worldgen.h \
  component_position.c component_position.h	\
  component_ground.c component_ground.h		\
  component_integrate.c component_integrate.h	\
  component_camera.c component_camera.h		\
  component_input.c component_input.h		\
  component_walking.c component_walking.h	\
  component_plant.c component_plant.h		\
  hex_utility.h hex_utility.c \
  hex.c hex.h \
  hex_draw.c hex_draw.h \
  ground_draw.c ground_draw.h \
  camera_draw.c camera_draw.h \
  system.c system.h	\
  beyond.c beyond.h
beyond_LDADD =	\
  libline.la	\
  libsph.la		\
  libvector.la	\
  libquat.la	\
  libfcmp.la	\
  -lm -lGL -lSDL

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