#ifndef __BELL_HTS_ENGINE_H__
#define __BELL_HTS_ENGINE_H__

#include "HTS_engine.h"
#include "cst_voice.h"
#include "cst_tokenstream.h"
#include "nitech_engine.h"

#define CLUSTERGENMODE 0
#define HTSMODE 1
#define NITECHMODE 2

float bell_hts_file_to_speech(HTS_Engine * engine, nitech_engine * ntengine, const char *filename, cst_voice *voice, const char *outtype, const int voice_type);

float bell_hts_ts_to_speech(HTS_Engine * engine, nitech_engine * ntengine, cst_tokenstream *ts, cst_voice *voice, const char *outtype, const int voice_type);

cst_voice * bell_voice_load(char *fn_name, const int voice_type, HTS_Engine * engine,
                           nitech_engine * ntengine);
void bell_voice_unload(cst_voice *voice, const int voice_type, HTS_Engine * engine,
                           nitech_engine * ntengine);

#endif