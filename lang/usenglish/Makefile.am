include $(top_srcdir)/Makefile.inc
AM_CFLAGS += -I.

lib_LTLIBRARIES = libbellbird_usenglish.la
libbellbird_usenglish_la_LDFLAGS = -version-info @LIBBELLBIRD_USENGLISH_VERSION@

libbellbird_usenglish_la_SOURCES = us_int_accent_cart.c us_int_tone_cart.c us_f0_model.c \
       us_dur_stats.c us_durz_cart.c us_f0lr.c \
       us_phoneset.c us_ffeatures.c us_ffeatures_hts.c us_phrasing_cart.c \
       us_gpos.c us_text.c us_expand.c \
       us_nums_cart.c us_aswd.c usenglish.c us_pos_cart.c

dist_bin_SCRIPTS = make_us_regexes us_pos.tree

regexes:
	./make_us_regexes $(BINDIR) >us_regexes.h

us_pos_cart.c: 
	$(ESTDIR)/../festival/bin/festival -b $(FLITEDIR)/tools/make_cart.scm $(FLITEDIR)/tools/make_pos.scm 

