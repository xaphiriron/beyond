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
beyond_SOURCES = \
  xph_memory.c xph_memory.h \
  list.c list.h \
  point.c point.h \
  turtle.c turtle.h \
  entity.c entity.h \
  video.c video.h \
  turtledraw.c turtledraw.h \
  input.c input.h \
  timer.c timer.h \
  accumulator.c accumulator.h \
  physics.c physics.h \
  world.c world.h \
  system.c system.h \
  beyond.c beyond.h
beyond_LDADD = \
  libline.la \
  libvector.la \
  libfcmp.la \
  -lm -lGL -lSDL
