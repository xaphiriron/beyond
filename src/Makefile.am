## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -Wall -Werror -DMEM_DEBUG
# AM_CPPFLAGS = -Wall -Werror -fprofile-generate -ftest-coverage
# AM_LDFLAGS = -fprofile-generate -lgcov

lib_LTLIBRARIES = libfcmp.la libvector.la libline.la
libfcmp_la_SOURCES = fcmp.c fcmp.h
libvector_la_SOURCES = vector.c vector.h
libline_la_SOURCES = line.c line.h
#libline_la_LIBADD = libvector.la


bin_PROGRAMS = beyond
#bin_PROGRAMS = beyond logo
beyond_SOURCES =	\
  xph_memory.c xph_memory.h \
  cpv.c cpv.h		\
  object.c object.h	\
  video.c video.h	\
  input.c input.h	\
  timer.c timer.h	\
  accumulator.c accumulator.h \
  physics.c physics.h	\
  entity.c entity.h	\
  component_position.c component_position.h	\
  component_integrate.c component_integrate.h	\
  component_camera.c component_camera.h		\
  component_collide.c component_collide.h	\
  hex.c hex.h		\
  world.c world.h	\
  system.c system.h	\
  beyond.c beyond.h
beyond_LDADD =	\
  libline.la	\
  libvector.la	\
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