include $(top_srcdir)/Makefile.inc
bellbird_LDFLAGS = -L$(top_builddir) -lbellbird \
                   -L$(top_builddir)/lang/cmulex  -lbellbird_cmulex \
                   -L$(top_builddir)/lang/usenglish -lbellbird_usenglish \
                   -lm

compile_regexes_SOURCES = compile_regexes.c ../src/regex/regcomp.c

bin_PROGRAMS = bellbird compile_regexes
