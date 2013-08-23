ACLOCAL_AMFLAGS = -I m4
SUBDIRS = include src lang . main
EXTRA_DIST = README ACKNOWLEDGEMENTS COPYING README poembellbirds.txt

lib_LTLIBRARIES = libbellbird.la

libbellbird_la_SOURCES =

libbellbird_la_LDFLAGS = -version-info @LIBBELLBIRD_VERSION@

libbellbird_la_LIBADD = $(top_builddir)/src/libbellbird_src.la \
                        $(top_builddir)/lang/cmulex/libbellbird_cmulex.la \
                        -lm


dist_noinst_SCRIPTS = autogen.sh
