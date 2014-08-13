include $(top_srcdir)/Makefile.inc

noinst_LTLIBRARIES = libbellbird_src_commonsynth.la

libbellbird_src_commonsynth_la_SOURCES = HTS_misc.c bell_driver.c

# mlsacore, mlsafunc.c and cholesky are included
# in more than one compilation unit.
# Ordinarily we might use normal functions but we use static functions
# as experience has showed us this code is highly performance critical
# and the compiler likes to inline and improve performance of these
# functions if we declare them static.
EXTRA_DIST = mlsacore.c mlsafunc.c cholesky.c
