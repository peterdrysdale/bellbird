include $(top_srcdir)/Makefile.inc
AM_CFLAGS += -I. -I$(LEXDEFS)

langcmulexdir = $(pkgincludedir)/lang/cmulex

langcmulex_HEADERS = cmu_lex.h

noinst_HEADERS = cmu_lts_model.h

lib_LTLIBRARIES = libbellbird_cmulex.la
libbellbird_cmulex_la_LDFLAGS = -version-info @LIBBELLBIRD_CMULEX_VERSION@

libbellbird_cmulex_la_SOURCES = cmu_lts_rules.c cmu_lts_model.c \
       cmu_lex.c cmu_lex_entries.c cmu_lex_data.c cmu_postlex.c

EXTRA_DIST = cmu_lex_data_raw.c cmu_lex_num_bytes.c 

