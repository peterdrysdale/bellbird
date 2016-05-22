ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lang/cmulex . main

EXTRA_DIST = ACKNOWLEDGEMENTS COPYING Makefile.inc README.md poembellbirds.txt \
             lang/usenglish/us_pos.tree lang/usenglish/make_us_regexes \
             commonsynth/mlsacore.c commonsynth/ldl.c
# mlsacore and ldl are included in more than one compilation unit.
# Ordinarily we might use normal functions but we use static functions
# as experience has showed us this code is highly performance critical
# and the compiler likes to inline and improve performance of these
# functions if we declare them static.

dist_noinst_SCRIPTS = autogen.sh

AM_CFLAGS = -I$(top_srcdir)/include
AM_CFLAGS += -I./audio/ -I$(AUDIOINCLUDE) $(AUDIODEFS)
AM_CFLAGS += -I./utils/ $(SOCKETS)
AM_CFLAGS += -I./regex/
AM_CFLAGS += -I./commonsynth/
AM_CFLAGS += -I./hts/
AM_CFLAGS += -I./cg/

noinst_HEADERS = src/regex/cst_regex_defs.h \
                 src/regex/hs_reg.h \
		 src/cg/cst_cg_map.h \
		 src/cg/cst_spamf0.h \
		 src/hts/HTS_hidden.h

lib_LTLIBRARIES = libbellbird.la libbellbird_usenglish.la

libbellbird_la_LDFLAGS = -version-info @LIBBELLBIRD_VERSION@

libbellbird_la_LIBADD = $(top_builddir)/lang/cmulex/libbellbird_cmulex.la \
                        -lm \
                        $(AUDIOLIBS)

libbellbird_la_SOURCES = src/audio/bell_au_buff.c \
                             src/audio/bell_audio.c \
                             src/audio/au_alsa.c \
                             src/audio/au_android.c \
                             src/audio/au_win32.c \
                             src/audio/au_none.c \
                             src/utils/cst_alloc.c \
                             src/utils/cst_error.c \
                             src/utils/cst_string.c \
                             src/utils/cst_tokenstream.c \
                             src/utils/cst_val.c \
                             src/utils/cst_features.c \
                             src/utils/cst_endian.c \
                             src/utils/cst_val_const.c \
                             src/utils/cst_val_user.c \
                             src/utils/bell_file.c \
                             src/regex/cst_regex.c \
                             src/regex/regexec.c \
                             src/commonsynth/HTS_misc.c \
                             src/commonsynth/bell_driver.c \
                             src/hts/HTS_engine.c \
                             src/hts/HTS_gstream.c \
                             src/hts/HTS_model.c \
                             src/hts/HTS_pstream.c \
                             src/hts/HTS_sstream.c \
                             src/hts/HTS_vocoder.c \
                             src/cg/bell_track.c \
                             src/cg/cst_cg.c \
                             src/cg/cst_mlsa.c \
                             src/cg/cst_mlpg.c \
                             src/cg/cst_cg_load_voice.c \
                             src/cg/cst_cg_map.c \
                             src/cg/cst_spamf0.c \
                             src/hrg/cst_utterance.c \
                             src/hrg/cst_relation.c \
                             src/hrg/cst_item.c \
                             src/hrg/cst_ffeature.c \
                             src/stats/cst_cart.c \
                             src/speech/cst_wave.c \
                             src/speech/cst_wave_io.c \
                             src/lexicon/cst_lexicon.c \
                             src/lexicon/cst_lts.c \
                             src/synth/cst_synth.c \
                             src/synth/cst_utt_utils.c \
                             src/synth/cst_voice.c \
                             src/synth/cst_phoneset.c \
                             src/synth/cst_ffeatures.c \
                             src/synth/cst_ssml.c \
                             src/synth/flite.c

libbellbird_usenglish_la_LDFLAGS = -version-info @LIBBELLBIRD_USENGLISH_VERSION@

libbellbird_usenglish_la_SOURCES = lang/usenglish/us_int_accent_cart.c \
                             lang/usenglish/us_int_tone_cart.c \
                             lang/usenglish/us_phoneset.c \
                             lang/usenglish/us_ffeatures.c \
                             lang/usenglish/us_ffeatures_hts.c \
                             lang/usenglish/us_phrasing_cart.c \
                             lang/usenglish/us_gpos.c \
                             lang/usenglish/us_text.c \
                             lang/usenglish/us_expand.c \
                             lang/usenglish/us_nums_cart.c \
                             lang/usenglish/us_aswd.c \
                             lang/usenglish/usenglish.c \
                             lang/usenglish/us_pos_cart.c

libbellbird_usenglish_la_LIBADD = $(top_builddir)/libbellbird.la

langusenglishdir = $(pkgincludedir)/lang/usenglish
langusenglish_HEADERS = lang/usenglish/usenglish.h \
                        lang/usenglish/us_ffeatures.h \
                        lang/usenglish/us_ffeatures_hts.h \
                        lang/usenglish/us_int_accent_cart.h \
                        lang/usenglish/us_int_tone_cart.h \
                        lang/usenglish/us_nums_cart.h \
                        lang/usenglish/us_phrasing_cart.h \
                        lang/usenglish/us_pos_cart.h \
                        lang/usenglish/us_regexes.h \
                        lang/usenglish/us_text.h

myincludedir = $(pkgincludedir)/include

myinclude_HEADERS = include/cst_alloc.h \
	include/cst_cart.h include/cst_cg.h include/cst_endian.h \
	include/cst_error.h include/cst_features.h include/cst_ffeatures.h \
	include/cst_file.h include/cst_item.h \
	include/cst_lexicon.h include/cst_lts.h \
	include/cst_phoneset.h include/cst_regex.h \
	include/cst_relation.h include/cst_string.h include/cst_synth.h \
	include/cst_tokenstream.h \
	include/cst_utt_utils.h include/cst_utterance.h \
	include/cst_val.h include/cst_val_const.h include/cst_val_defs.h \
	include/cst_voice.h include/cst_wave.h \
        include/flite.h include/HTS_misc.h include/pstream.h \
        include/HTS_engine.h \
        include/bell_audio.h include/bell_driver.h \
        include/bell_file.h include/bell_track.h include/bell_ff_sym.h \
        include/bell_relation_sym.h include/native_audio.h
