/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/* ----------------------------------------------------------------- */
/*           The HMM-Based Speech Synthesis Engine "hts_engine API"  */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2001-2013  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/*                2001-2008  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the HTS working group nor the names of its  */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */
#include <string.h>             /* for strlen(), strstr() */

#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_string.h"
/* hts_engine libraries */
#include "HTS_hidden.h"

#define HALF_TONE 0.05776226504666210911810267678818    /* log(2.0) / 12.0 */

/* HTS_Engine_initialize: initialize engine */
void HTS_Engine_initialize(HTS_Engine * engine)
{
   /* global */
   engine->condition.sampling_frequency = 0;
   engine->condition.fperiod = 0;
   engine->condition.msd_threshold = NULL;
   engine->condition.gv_weight = NULL;

   /* duration */
   engine->condition.speed = 1.0;

   /* spectrum */
   engine->condition.alpha = 0.0;
   engine->condition.beta = 0.0;

   /* log F0 */
   engine->condition.additional_half_tone = 0.0;

   /* initialize model set */
   HTS_ModelSet_initialize(&engine->ms);
   /* initialize state sequence set */
   HTS_SStreamSet_initialize(&engine->sss);
   /* initialize pstream set */
   HTS_PStreamSet_initialize(&engine->pss);
   /* initialize gstream set */
   HTS_GStreamSet_initialize(&engine->gss);
}

/* HTS_Engine_load: load HTS voices */
HTS_Boolean HTS_Engine_load(HTS_Engine * engine, char **voices)
{
   size_t i;
   size_t nstream;
   const char *option, *find;
   float tempfloat;

   /* reset engine */
   HTS_Engine_clear(engine);

   /* load voices */
   if (HTS_ModelSet_load(&engine->ms, voices) != TRUE) {
      HTS_Engine_clear(engine);
      return FALSE;
   }
   nstream = HTS_ModelSet_get_nstream(&engine->ms);

   /* global */
   engine->condition.sampling_frequency = HTS_ModelSet_get_sampling_frequency(&engine->ms);
   engine->condition.fperiod = HTS_ModelSet_get_fperiod(&engine->ms);
   engine->condition.msd_threshold = cst_alloc(double,nstream);
   for (i = 0; i < nstream; i++)
      engine->condition.msd_threshold[i] = 0.5;
   engine->condition.gv_weight = cst_alloc(double,nstream);
   for (i = 0; i < nstream; i++)
      engine->condition.gv_weight[i] = 1.0;

   /* spectrum */
   option = HTS_ModelSet_get_option(&engine->ms, 0);
   find = strstr(option, "GAMMA=");
   if (find != NULL) {
      cst_errmsg("Non-Zero GAMMA not supported\n");
      HTS_Engine_clear(engine);
      return FALSE;
   }
   find = strstr(option, "LN_GAIN=1");
   if (find != NULL) {
      cst_errmsg("Non-Zero LN_GAIN not supported\n");
      HTS_Engine_clear(engine);
      return FALSE;
   }
   find = strstr(option, "ALPHA=");
   if (find != NULL) {
      if (bell_validate_atof(&find[strlen("ALPHA=")],&tempfloat)) {
         engine->condition.alpha = tempfloat;
      } else {
         cst_errmsg("Voice file option 'ALPHA' is not float, setting to default 0.42\n");
         engine->condition.alpha = 0.42;
      }
   }

   return TRUE;
}

/* HTS_Engine_get_sampling_frequency: get sampling frequency */
size_t HTS_Engine_get_sampling_frequency(HTS_Engine * engine)
{
   return engine->condition.sampling_frequency;
}

/* HTS_Egnine_set_msd_threshold: set MSD threshold */
void HTS_Engine_set_msd_threshold(HTS_Engine * engine, size_t stream_index, double f)
{
   if (f < 0.0)
      f = 0.0;
   if (f > 1.0)
      f = 1.0;
   engine->condition.msd_threshold[stream_index] = f;
}

/* HTS_Engine_set_gv_weight: set GV weight */
void HTS_Engine_set_gv_weight(HTS_Engine * engine, size_t stream_index, double f)
{
   if (f < 0.0)
      f = 0.0;
   engine->condition.gv_weight[stream_index] = f;
}

/* HTS_Engine_set_speed: set speech speed */
void HTS_Engine_set_speed(HTS_Engine * engine, double f)
{
   if (f < 1.0E-06)
      f = 1.0E-06;
   engine->condition.speed = f;
}

/* HTS_Engine_set_beta: set beta */
void HTS_Engine_set_beta(HTS_Engine * engine, double f)
{
   if (f < 0.0)
      f = 0.0;
   if (f > 1.0)
      f = 1.0;
   engine->condition.beta = f;
}

/* HTS_Engine_add_half_tone: add half tone */
void HTS_Engine_add_half_tone(HTS_Engine * engine, double f)
{
   engine->condition.additional_half_tone = f;
}

/* HTS_Engine_synthesize_from_strings: synthesize speech from strings */
HTS_Boolean HTS_Engine_synthesize_from_strings(HTS_Engine * engine, char **lines, size_t num_lines)
{
   size_t i;
   double f;

   HTS_Engine_refresh(engine);
// Generate state sequence
   if (HTS_SStreamSet_create(&engine->sss, &engine->ms, lines, num_lines, engine->condition.speed) != TRUE) {
      HTS_Engine_refresh(engine);
      return FALSE;
   }
   if (engine->condition.additional_half_tone != 0.0) {
      for (i = 0; i < HTS_SStreamSet_get_total_state(&engine->sss); i++) {
         f = HTS_SStreamSet_get_mean(&engine->sss, 1, i, 0);
         f += engine->condition.additional_half_tone * HALF_TONE;
         if (f < MIN_LF0)
            f = MIN_LF0;
         else if (f > MAX_LF0)
            f = MAX_LF0;
         HTS_SStreamSet_set_mean(&engine->sss, 1, i, 0, f);
      }
   }
// Generate parameter sequence
   if (HTS_PStreamSet_create(&engine->pss, &engine->sss, engine->condition.msd_threshold, engine->condition.gv_weight) != TRUE) {
      HTS_Engine_refresh(engine);
      return FALSE;
   }
// Generate sound sample sequence
   if (HTS_GStreamSet_create(&engine->gss, &engine->pss, engine->condition.sampling_frequency,
                             engine->condition.fperiod, engine->condition.alpha,
                             engine->condition.beta) != TRUE) {
      HTS_Engine_refresh(engine);
      return FALSE;
   }
   return TRUE;
}

/* HTS_Engine_refresh: free model per one time synthesis */
void HTS_Engine_refresh(HTS_Engine * engine)
{
   /* free generated parameter stream set */
   HTS_GStreamSet_clear(&engine->gss);
   /* free parameter stream set */
   HTS_PStreamSet_clear(&engine->pss);
   /* free state stream set */
   HTS_SStreamSet_clear(&engine->sss);
}

/* HTS_Engine_clear: free engine */
void HTS_Engine_clear(HTS_Engine * engine)
{
   if (engine->condition.msd_threshold != NULL)
      cst_free(engine->condition.msd_threshold);
   if (engine->condition.gv_weight != NULL)
      cst_free(engine->condition.gv_weight);

   HTS_ModelSet_clear(&engine->ms);
   HTS_Engine_initialize(engine);
}
