#ifndef __NITECH_ENGINE_H__
#define __NITECH_ENGINE_H__

#include "cst_utterance.h"
#include "../src/nitechvoice/nitech_hidden.h"
typedef struct _nitech_engine {
   cst_utterance *utt;
   ModelSet   ms;
   TreeSet    ts;
   PStream    mceppst, lf0pst;
   VocoderSetup vs;
} nitech_engine;

void nitech_engine_initialize(nitech_engine *ntengine, const char * fn_voice);
bell_boolean nitech_engine_synthesize_from_strings(
                      nitech_engine * ntengine, char **lines, size_t num_lines);
void nitech_engine_clear(nitech_engine * ntengine);

#endif