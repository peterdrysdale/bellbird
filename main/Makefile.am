include $(top_srcdir)/Makefile.inc
#LDFLAGS = -L$(top_builddir)/lang -L$(top_builddir) -lbellbird -lbellbird
bellbird_LDFLAGS = -L$(top_builddir) -lbellbird \
                   -L$(top_builddir)/lang/cmulex  -lbellbird_cmulex \
                   -L$(top_builddir)/lang/usenglish -lbellbird_usenglish \
                   -lm
compile_regexes_LDFLAGS = -L$(top_builddir) -lbellbird \
                   -L$(top_builddir)/lang/cmulex  -lbellbird_cmulex \
                   -L$(top_builddir)/lang/usenglish -lbellbird_usenglish \
                   -lm
bin_PROGRAMS = bellbird compile_regexes
