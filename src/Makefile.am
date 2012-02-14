## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I/usr/include/freetype2 -Wall -Werror -O3
#AM_CPPFLAGS = -Wall -Werror -O3 -DMEM_DEBUG

#AM_CPPFLAGS = -Wall -Werror -fprofile-generate -ftest-coverage
#AM_LDFLAGS = -fprofile-generate -lgcov

lib_LTLIBRARIES = libfcmp.la libvector.la libquat.la libline.la libmatrix.la libcollide.la libogdl.la
libfcmp_la_SOURCES = fcmp.c fcmp.h
libvector_la_SOURCES = vector.c vector.h
libquat_la_SOURCES = quaternion.c quaternion.h
libline_la_SOURCES = line.c line.h
#libline_la_LIBADD = libvector.la
#libsph_la_SOURCES = sph.c sph.h
libmatrix_la_SOURCES = matrix.c matrix.h
libcollide_la_SOURCES = collision.c collision.h
libogdl_la_SOURCES = ogdl/graph.c ogdl/path.c ogdl/ogdlparser.c ogdl/ogdllog.c ogdl/ogdl.h


bin_PROGRAMS = beyond
#bin_PROGRAMS = beyond logo
beyond_SOURCES = \
	xph_memory.c xph_memory.h \
	xph_log.c xph_log.h \
	xph_path.c xph_path.h \
	graph_common.c graph_common.h \
	shapes.c shapes.h \
	turtle3d.c turtle3d.h \
	texture.c texture.h \
	dynarr.c dynarr.h \
	lsystem.c lsystem.h \
	video.c video.h	\
	xph_timer.c xph_timer.h	\
	\
	matspec.c matspec.h \
	hex_utility.h hex_utility.c \
	map.c map.h \
	worldgen.c worldgen.h \
	\
	sprite.c sprite.h \
	font.c font.h \
	\
	entity.c entity.h \
	comp_menu.c comp_menu.h \
	comp_textlabel.c comp_textlabel.h \
	comp_clickable.c comp_clickable.h \
	comp_optlayout.c comp_optlayout.h \
	comp_player.c comp_player.h \
	comp_worldmap.c comp_worldmap.h \
	comp_gui.c comp_gui.h \
	comp_chaser.c comp_chaser.h \
	comp_plant.c comp_plant.h \
	comp_body.c comp_body.h \
	comp_arch.c comp_arch.h \
	component_position.c component_position.h	\
	component_camera.c component_camera.h		\
	component_input.c component_input.h		\
	component_walking.c component_walking.h	\
	\
	system.c system.h \
	beyond.c
beyond_LDADD = \
	-lm -lGL -lSDL -lglpng \
	-lfreetype \
	libogdl.la \
	libcollide.la \
	libmatrix.la \
	libline.la \
	libvector.la \
	libquat.la \
	libfcmp.la
