#ifndef _BELL_AUDIO_H__
#define _BELL_AUDIO_H__

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "cst_wave.h"

bell_boolean play_wave(cst_wave *w);

#ifdef CST_AUDIO_ALSA
int audio_scheduler();
int buffer_wave(cst_wave *w, int fd);
void audio_scheduler_close(int fd);
#endif

#endif
