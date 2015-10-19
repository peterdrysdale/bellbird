include $(top_srcdir)/Makefile.inc
AM_CFLAGS += -I. -I$(AUDIOINCLUDE) $(AUDIODEFS)

noinst_HEADERS = native_audio.h

noinst_LTLIBRARIES = libbellbird_src_audio.la

libbellbird_src_audio_la_SOURCES = bell_au_buff.c bell_audio.c au_alsa.c au_none.c

libbellbird_src_audio_la_LIBADD = $(AUDIOLIBS)
