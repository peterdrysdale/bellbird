#ifndef __BELL_HTS_ENGINE_H__
#define __BELL_HTS_ENGINE_H__

#include "HTS_engine.h"
#include "cst_voice.h"
#include "cst_tokenstream.h"

float bell_file_to_speech(HTS_Engine * engine, const char *filename,
                          bell_voice *voice, const char *outtype,
                          cst_audiodev *ad);
float bell_text_to_speech(HTS_Engine * engine, const char *text,
                          bell_voice *voice, const char *outtype,
                          cst_audiodev *ad);
bell_voice * bell_voice_load(char *fn_name, HTS_Engine * engine);
void bell_voice_unload(bell_voice *voice, HTS_Engine * engine);

#endif
