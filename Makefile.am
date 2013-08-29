ACLOCAL_AMFLAGS = -I m4
SUBDIRS = include src lang/cmulex . lang/usenglish main
EXTRA_DIST = ACKNOWLEDGEMENTS COPYING Makefile.inc README.md poembellbirds.txt

lib_LTLIBRARIES = libbellbird.la

libbellbird_la_SOURCES =

libbellbird_la_LDFLAGS = -version-info @LIBBELLBIRD_VERSION@

libbellbird_la_LIBADD = $(top_builddir)/src/libbellbird_src.la \
                        $(top_builddir)/lang/cmulex/libbellbird_cmulex.la \
                        -lm


dist_noinst_SCRIPTS = autogen.sh
