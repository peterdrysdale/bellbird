/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/* ----------------------------------------------------------------- */
/*           The English TTS System "Flite+hts_engine"               */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2005-2016  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/*                2005-2008  Tokyo Institute of Technology           */
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

#include <limits.h>
#include <string.h> // for strstr()
#include "cst_alloc.h"
#include "cst_cg.h"
#include "cst_error.h"
#include "cst_features.h"
#include "bell_file.h"
#include "cst_item.h"
#include "cst_regex.h"
#include "cst_relation.h"
#include "cst_string.h"
#include "cst_synth.h"
#include "cst_tokenstream.h"
#include "cst_val.h"
#include "cst_utterance.h"
#include "cst_wave.h"
#include "flite.h"
#include "bell_driver.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"
#include "../hts/HTS_hidden.h"

static void bell_hts_get_wave(HTS_Engine * engine,cst_utterance * utt)
{
    /* Extract synthesized wave data from hts engine for use with bellbird's */
    /* existing output routines.                                             */
    cst_wave* wave;

    wave = new_wave();
    wave->num_channels=1;
    CST_WAVE_SET_NUM_SAMPLES(wave,HTS_GStreamSet_get_total_nsamples(&engine->gss));
    CST_WAVE_SAMPLES(wave) = HTS_GStreamSet_abandon_speech_array(&engine->gss);
    CST_WAVE_SET_SAMPLE_RATE(wave,HTS_Engine_get_sampling_frequency(engine));
    utt_set_wave(utt,wave);
    return;
}

static void Flite_HTS_Engine_create_label(cst_item * item, char *label, size_t labellen)
{
   char * seg_pp;
   char * seg_p;
   char * seg_c;
   char * seg_n;
   char * seg_nn;
   char * endtone;
   cst_val *tmpsyl_vowel;
   size_t labelretlen = 0;
   cst_item *syl_item;
   cst_item *word_item;
   cst_item *phrase_item;

   int sub_phrases = 0;
   int lisp_total_phrases = 0;
   int tmp1 = 0;
   int tmp2 = 0;
   int tmp3 = 0;
   int tmp4 = 0;

   /* load segments */
   seg_pp = cst_strdup(ffeature_string(item, "p.p.name"));
   seg_p = cst_strdup(ffeature_string(item, "p.name"));
   seg_c = cst_strdup(ffeature_string(item, "name"));
   seg_n = cst_strdup(ffeature_string(item, "n.name"));
   seg_nn = cst_strdup(ffeature_string(item, "n.n.name"));

   /* load endtone */
   endtone = cst_strdup(ffeature_string(item,
                                           ("R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn."ENDTONE)));

   if (cst_streq(seg_c, "pau")) {
      /* for pause */
      if (item_next(item) != NULL) {
         sub_phrases = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SUB_PHRASES);
         tmp1 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_SYLS);
         tmp2 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_WORDS);
         lisp_total_phrases = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_PHRASES);
      } else {
         sub_phrases = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SUB_PHRASES);
         tmp1 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_SYLS);
         tmp2 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_WORDS);
         lisp_total_phrases = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_PHRASES);
      }
      labelretlen = bell_snprintf(label, labellen,
              "%s^%s-%s+%s=%s@x_x/A:%d_%d_%d/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:%d+%d+%d/D:%s_%d/E:x+x@x+x&x+x#x+x/F:%s_%d/G:%d_%d/H:x=x^%d=%d|%s/I:%d=%d/J:%d+%d-%d",
              strcmp(seg_pp, "0") == 0 ? "x" : seg_pp,
              strcmp(seg_p, "0") == 0 ? "x" : seg_p,
              seg_c,
              strcmp(seg_n, "0") == 0 ? "x" : seg_n,
              strcmp(seg_nn, "0") == 0 ? "x" : seg_nn,
              ffeature_int(item, "p.R:"SYLSTRUCTURE".P.R:"SYLLABLE".stress"),
              ffeature_int(item, "p.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."ACCENTED),
              ffeature_int(item, "p.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_NUMPHONES),
              ffeature_int(item, "n.R:"SYLSTRUCTURE".P.R:"SYLLABLE".stress"),
              ffeature_int(item, "n.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."ACCENTED),
              ffeature_int(item, "n.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_NUMPHONES),
              ffeature_string(item, "p.R:"SYLSTRUCTURE".P.P.R:"WORD"."GPOS),
              ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"WORD"."WORD_NUMSYLS),
              ffeature_string(item, "n.R:"SYLSTRUCTURE".P.P.R:"WORD"."GPOS),
              ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"WORD"."WORD_NUMSYLS),
              ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_WORDS_IN_PHRASE),
              sub_phrases + 1,
              lisp_total_phrases - sub_phrases,
              endtone,
              ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_WORDS_IN_PHRASE),
              tmp1,
              tmp2,
              lisp_total_phrases);
   } else {
      /* for no pause */
      syl_item = path_to_item(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE);
      word_item = path_to_item(item, "R:"SYLSTRUCTURE".P.P.R:"WORD);
      phrase_item = path_to_item(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P");
      tmp1 = ffeature_int(item, "R:"SYLSTRUCTURE"."POS_IN_SYL);
      tmp2 = ffeature_int(syl_item, SYL_NUMPHONES);
      tmp3 = ffeature_int(syl_item, POS_IN_WORD);
      tmp4 = ffeature_int(word_item, WORD_NUMSYLS);

      /* This code is to prevent memory leaks when calling syl_vowel             */
      /* via ffeature_string which accidently leaks the cst_val.                 */
      /* The cst_val created by syl_vowel is dynamically allocated hence casting */
      /* away const will let us delete is when we are finished with it.          */
      tmpsyl_vowel = (cst_val *) ffeature(syl_item,SYL_VOWEL);

      sub_phrases = ffeature_int(syl_item, SUB_PHRASES);
      lisp_total_phrases = ffeature_int(phrase_item, LISP_TOTAL_PHRASES);
      labelretlen = bell_snprintf(label, labellen,
              "%s^%s-%s+%s=%s@%d_%d/A:%d_%d_%d/B:%d-%d-%d@%d-%d&%d-%d#%d-%d$%d-%d!%d-%d;%d-%d|%s/C:%d+%d+%d/D:%s_%d/E:%s+%d@%d+%d&%d+%d#%d+%d/F:%s_%d/G:%d_%d/H:%d=%d^%d=%d|%s/I:%d=%d/J:%d+%d-%d",
              strcmp(seg_pp, "0") == 0 ? "x" : seg_pp,
              strcmp(seg_p, "0") == 0 ? "x" : seg_p,
              seg_c,
              strcmp(seg_n, "0") == 0 ? "x" : seg_n,
              strcmp(seg_nn, "0") == 0 ? "x" : seg_nn,
              tmp1 + 1,
              tmp2 - tmp1,
              ffeature_int(syl_item, "p.stress"),
              ffeature_int(syl_item, "p."ACCENTED),
              ffeature_int(syl_item, "p."SYL_NUMPHONES),
              ffeature_int(syl_item, "stress"),
              ffeature_int(syl_item, ACCENTED),
              tmp2,
              tmp3 + 1,
              tmp4 - tmp3,
              ffeature_int(syl_item, SYL_IN) + 1,
              ffeature_int(syl_item, SYL_OUT) + 1,
              ffeature_int(syl_item, SSYL_IN) + 1,
              ffeature_int(syl_item, SSYL_OUT) + 1,
              ffeature_int(syl_item, ASYL_IN) + 1,
              ffeature_int(syl_item, ASYL_OUT) + 1,
              ffeature_int(syl_item, LISP_DISTANCE_TO_P_STRESS),
              ffeature_int(syl_item, LISP_DISTANCE_TO_N_STRESS),
              ffeature_int(syl_item, LISP_DISTANCE_TO_P_ACCENT),
              ffeature_int(syl_item, LISP_DISTANCE_TO_N_ACCENT),
              val_string(tmpsyl_vowel),
              ffeature_int(syl_item, "n.stress"),
              ffeature_int(syl_item, "n."ACCENTED),
              ffeature_int(syl_item, "n."SYL_NUMPHONES),
              ffeature_string(word_item, "p."GPOS),
              ffeature_int(word_item, "p."WORD_NUMSYLS),
              ffeature_string(word_item, GPOS),
              tmp4,
              ffeature_int(word_item, POS_IN_PHRASE) + 1,
              ffeature_int(word_item, WORDS_OUT),
              ffeature_int(word_item, HTS_CONTENT_WORDS_IN) + 1,
              ffeature_int(word_item, HTS_CONTENT_WORDS_OUT) + 1,
              ffeature_int(word_item, LISP_DISTANCE_TO_P_CONTENT),
              ffeature_int(word_item, LISP_DISTANCE_TO_N_CONTENT),
              ffeature_string(word_item, "n."GPOS),
              ffeature_int(word_item, "n."WORD_NUMSYLS),
              ffeature_int(phrase_item, "p."LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(phrase_item, "p."LISP_NUM_WORDS_IN_PHRASE),
              ffeature_int(phrase_item, LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(phrase_item, LISP_NUM_WORDS_IN_PHRASE),
              sub_phrases + 1,
              lisp_total_phrases - sub_phrases,
              strcmp(endtone, "0") == 0 ? "NONE" : endtone,
              ffeature_int(phrase_item, "n."LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(phrase_item, "n."LISP_NUM_WORDS_IN_PHRASE),
              ffeature_int(phrase_item, LISP_TOTAL_SYLS),
              ffeature_int(phrase_item, LISP_TOTAL_WORDS),
              lisp_total_phrases);
      delete_val(tmpsyl_vowel);
   }
   if (labelretlen > labellen -1) {
      cst_errmsg("Flite_HTS_Engine_create_label: Buffer overflow");
   }
   cst_free(seg_pp);
   cst_free(seg_p);
   cst_free(seg_c);
   cst_free(seg_n);
   cst_free(seg_nn);
   cst_free(endtone);
}

static void Flite_HTS_Engine_create_label_new(cst_item * item, char *label, size_t labellen)
{
// Create label per phoneme for new API HTS voices
   cst_item *syl_item;
   cst_item *word_item;
   cst_item *phrase_item;
   size_t labelretlen = 0;
   cst_val *tmpsyl_vowel;
   const char *p1 = ffeature_string(item, "p.p.name");
   const char *p2 = ffeature_string(item, "p.name");
   const char *p3 = ffeature_string(item, "name");
   const char *p4 = ffeature_string(item, "n.name");
   const char *p5 = ffeature_string(item, "n.n.name");

   if (strcmp(p3, "pau") == 0) {
      /* for pause */
      int a3 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_NUMPHONES);
      int c3 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_NUMPHONES);
      int d2 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"WORD"."WORD_NUMSYLS);
      int f2 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"WORD"."WORD_NUMSYLS);
      int g1 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_SYLS_IN_PHRASE);
      int g2 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_WORDS_IN_PHRASE);
      int i1 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_SYLS_IN_PHRASE);
      int i2 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_WORDS_IN_PHRASE);
      int j1, j2, j3;
      if (item_next(item) != NULL) {
         j1 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_SYLS);
         j2 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_WORDS);
         j3 = ffeature_int(item, "n.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_PHRASES);
      } else {
         j1 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_SYLS);
         j2 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_WORDS);
         j3 = ffeature_int(item, "p.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_PHRASES);
      }
      labelretlen = bell_snprintf(label, labellen,
                                  "%s^%s-%s+%s=%s@xx_xx/A:%s_%s_%s/B:xx-xx-xx@xx-xx&xx-xx#xx-xx$xx-xx!xx-xx;xx-xx|xx/C:%s+%s+%s/D:%s_%s/E:xx+xx@xx+xx&xx+xx#xx+xx/F:%s_%s/G:%s_%s/H:xx=xx^xx=xx|xx/I:%s=%s/J:%d+%d-%d",      /* */
              strcmp(p1, "0") == 0 ? "xx" : p1, /* p1 */
              strcmp(p2, "0") == 0 ? "xx" : p2, /* p2 */
              p3,               /* p3 */
              strcmp(p4, "0") == 0 ? "xx" : p4, /* p4 */
              strcmp(p5, "0") == 0 ? "xx" : p5, /* p5 */
              a3 == 0 ? "xx" : ffeature_string(item, "p.R:"SYLSTRUCTURE".P.R:"SYLLABLE".stress"),      /* a1 */
              a3 == 0 ? "xx" : ffeature_string(item, "p.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."ACCENTED),    /* a2 */
              a3 == 0 ? "xx" : val_string(val_string_n(a3)),    /* a3 */
              c3 == 0 ? "xx" : ffeature_string(item, "n.R:"SYLSTRUCTURE".P.R:"SYLLABLE".stress"),      /* c1 */
              c3 == 0 ? "xx" : ffeature_string(item, "n.R:"SYLSTRUCTURE".P.R:"SYLLABLE"."ACCENTED),    /* c2 */
              c3 == 0 ? "xx" : val_string(val_string_n(c3)),    /* c3 */
              d2 == 0 ? "xx" : ffeature_string(item, "p.R:"SYLSTRUCTURE".P.P.R:"WORD"."GPOS),     /* d1 */
              d2 == 0 ? "xx" : val_string(val_string_n(d2)),    /* d2 */
              f2 == 0 ? "xx" : ffeature_string(item, "n.R:"SYLSTRUCTURE".P.P.R:"WORD"."GPOS),     /* f1 */
              f2 == 0 ? "xx" : val_string(val_string_n(f2)),    /* f2 */
              g1 == 0 ? "xx" : val_string(val_string_n(g1)),    /* g1 */
              g2 == 0 ? "xx" : val_string(val_string_n(g2)),    /* g2 */
              i1 == 0 ? "xx" : val_string(val_string_n(i1)),    /* i1 */
              i2 == 0 ? "xx" : val_string(val_string_n(i2)),    /* i2 */
              j1,               /* j1 */
              j2,               /* j2 */
              j3);              /* j3 */
   } else {
      /* for no pause */
      syl_item = path_to_item(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE);
      word_item = path_to_item(item, "R:"SYLSTRUCTURE".P.P.R:"WORD);
      phrase_item = path_to_item(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P");
      int p6 = ffeature_int(item, "R:"SYLSTRUCTURE"."POS_IN_SYL) + 1;
      int a3 = ffeature_int(syl_item, "p."SYL_NUMPHONES);
      int b3 = ffeature_int(syl_item, SYL_NUMPHONES);
      int b4 = ffeature_int(syl_item, POS_IN_WORD) + 1;
      int b12 = ffeature_int(syl_item, LISP_DISTANCE_TO_P_STRESS);
      int b13 = ffeature_int(syl_item, LISP_DISTANCE_TO_N_STRESS);
      int b14 = ffeature_int(syl_item, LISP_DISTANCE_TO_P_ACCENT);
      int b15 = ffeature_int(syl_item, LISP_DISTANCE_TO_N_ACCENT);
      int c3 = ffeature_int(syl_item, "n."SYL_NUMPHONES);
      int d2 = ffeature_int(word_item, "p."WORD_NUMSYLS);
      int e2 = ffeature_int(word_item, WORD_NUMSYLS);
      int e3 = ffeature_int(word_item, POS_IN_PHRASE) + 1;
      int e7 = ffeature_int(word_item, NEW_LISP_DISTANCE_TO_P_CONTENT);
      int e8 = ffeature_int(word_item, NEW_LISP_DISTANCE_TO_N_CONTENT);
      int f2 = ffeature_int(word_item, "n."WORD_NUMSYLS);
      int g1 = ffeature_int(phrase_item, "p."LISP_NUM_SYLS_IN_PHRASE);
      int g2 = ffeature_int(phrase_item, "p."LISP_NUM_WORDS_IN_PHRASE);
      int h2 = ffeature_int(phrase_item, LISP_NUM_WORDS_IN_PHRASE);
      int h3 = ffeature_int(syl_item, SUB_PHRASES) + 1;
      const char *h5 = ffeature_string(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn."ENDTONE);
      int i1 = ffeature_int(phrase_item, "n."LISP_NUM_SYLS_IN_PHRASE);
      int i2 = ffeature_int(phrase_item, "n."LISP_NUM_WORDS_IN_PHRASE);
      int j1 = ffeature_int(phrase_item, LISP_TOTAL_SYLS);
      int j2 = ffeature_int(phrase_item, LISP_TOTAL_WORDS);
      int j3 = ffeature_int(phrase_item, LISP_TOTAL_PHRASES);

      /* This code is to prevent memory leaks when calling syl_vowel             */
      /* via ffeature_string which accidently leaks the cst_val.                 */
      /* The cst_val created by syl_vowel is dynamically allocated hence casting */
      /* away const will let us delete is when we are finished with it.          */
      tmpsyl_vowel = (cst_val *) ffeature(syl_item, SYL_VOWEL);  /* b16 */

      labelretlen = bell_snprintf(label, labellen,
                                  "%s^%s-%s+%s=%s@%d_%d/A:%s_%s_%s/B:%d-%d-%d@%d-%d&%d-%d#%d-%d$%d-%d!%s-%s;%s-%s|%s/C:%s+%s+%s/D:%s_%s/E:%s+%d@%d+%d&%d+%d#%s+%s/F:%s_%s/G:%s_%s/H:%d=%d^%d=%d|%s/I:%s=%s/J:%d+%d-%d",      /* */
              strcmp(p1, "0") == 0 ? "xx" : p1, /* p1 */
              strcmp(p2, "0") == 0 ? "xx" : p2, /* p2 */
              p3,               /* p3 */
              strcmp(p4, "0") == 0 ? "xx" : p4, /* p4 */
              strcmp(p5, "0") == 0 ? "xx" : p5, /* p5 */
              p6,               /* p6 */
              b3 - p6 + 1,      /* p7 */
              a3 == 0 ? "xx" : ffeature_string(syl_item, "p.stress"),      /* a1 */
              a3 == 0 ? "xx" : ffeature_string(syl_item, "p."ACCENTED),    /* a2 */
              a3 == 0 ? "xx" : val_string(val_string_n(a3)),    /* a3 */
              ffeature_int(syl_item, "stress"),    /* b1 */
              ffeature_int(syl_item, ACCENTED),    /* b2 */
              b3,               /* b3 */
              b4,               /* b4 */
              e2 - b4 + 1,      /* b5 */
              ffeature_int(syl_item, SYL_IN) + 1,        /* b6 */
              ffeature_int(syl_item, SYL_OUT) + 1,       /* b7 */
              ffeature_int(syl_item, HTS_SSYL_IN),       /* b8 */
              ffeature_int(syl_item, SSYL_OUT),          /* b9 */
              ffeature_int(syl_item, ASYL_IN),           /* b10 */
              ffeature_int(syl_item, ASYL_OUT),          /* b11 */
              val_string(val_string_n(b12)),  /* b12 */
              val_string(val_string_n(b13)),  /* b13 */
              val_string(val_string_n(b14)),  /* b14 */
              val_string(val_string_n(b15)),  /* b15 */
              val_string(tmpsyl_vowel),
              c3 == 0 ? "xx" : ffeature_string(syl_item, "n.stress"),      /* c1 */
              c3 == 0 ? "xx" : ffeature_string(syl_item, "n."ACCENTED),    /* c2 */
              c3 == 0 ? "xx" : val_string(val_string_n(c3)),               /* c3 */
              d2 == 0 ? "xx" : ffeature_string(word_item, "p."GPOS),       /* d1 */
              d2 == 0 ? "xx" : val_string(val_string_n(d2)),               /* d2 */
              ffeature_string(word_item, GPOS),                            /* e1 */
              e2,               /* e2 */
              e3,               /* e3 */
              h2 - e3 + 1,      /* e4 */
              ffeature_int(word_item, CONTENT_WORDS_IN),                   /* e5 */
              ffeature_int(word_item, CONTENT_WORDS_OUT),                  /* e6 */
              val_string(val_string_n(e7)),               /* e7 */
              val_string(val_string_n(e8)),               /* e8 */
              f2 == 0 ? "xx" : ffeature_string(word_item, "n."GPOS),       /* f1 */
              f2 == 0 ? "xx" : val_string(val_string_n(f2)),               /* f2 */
              g1 == 0 ? "xx" : val_string(val_string_n(g1)),               /* g1 */
              g2 == 0 ? "xx" : val_string(val_string_n(g2)),               /* g2 */
              ffeature_int(phrase_item, LISP_NUM_SYLS_IN_PHRASE),          /* h1 */
              h2,               /* h2 */
              h3,               /* h3 */
              j3 - h3 + 1,      /* h4 */
              strcmp(h5, "0") == 0 ? "NONE" : h5,                          /* h5 */
              i1 == 0 ? "xx" : val_string(val_string_n(i1)),    /* i1 */
              i2 == 0 ? "xx" : val_string(val_string_n(i2)),    /* i2 */
              j1,               /* j1 */
              j2,               /* j2 */
              j3);              /* j3 */
      delete_val(tmpsyl_vowel);
   }
   if (labelretlen > labellen -1) {
      cst_errmsg("Flite_HTS_Engine_create_label: Buffer overflow");
   }
}

static float bell_ts_to_speech(HTS_Engine * engine, cst_tokenstream *ts,
                               bell_voice *voice, const char *outtype,
                               cst_audiodev *ad)
{
    int i;
    cst_utterance *utt;
    const char *token;
    cst_item *t;
    cst_relation *tokrel;
    float durs = 0;
    cst_item *s = NULL;
    char **label_data = NULL;
    int label_size = 0;
    int num_tokens;
    cst_wave *w;
    cst_breakfunc breakfunc = voice ->utt_break;
    int fp;

    fp = get_param_int(voice->features,"file_start_position",0);
    if (fp > 0)
        ts_set_stream_pos(ts,fp);

    /* If its a file we write to, create and save an empty wave file */
    /* as we are going to incrementally append to it                 */
    if (!cst_streq(outtype,"play") &&
        !cst_streq(outtype,"bufferedplay") &&
        !cst_streq(outtype,"none"))
    {
        w = new_wave();
        cst_wave_resize(w,0,1);
        if (engine != NULL)
        {  // Is HTS voice type
            if (HTS_Engine_get_sampling_frequency(engine) > INT_MAX)
            {
                cst_errmsg("HTS sample rate appears unusually high");
                cst_error();
            }
            CST_WAVE_SET_SAMPLE_RATE(w,(int)HTS_Engine_get_sampling_frequency(engine));
        }
        else
        {  // Is cg voice type
            CST_WAVE_SET_SAMPLE_RATE(w,16000);
        }
        cst_wave_save_riff(w,outtype);  /* an empty wave */
        delete_wave(w);
    }

    num_tokens = 0;
    utt = new_utterance();
    tokrel = utt_relation_create(utt, TOKEN);
    while (!ts_eof(ts) || num_tokens > 0)
    {
        token = ts_get(ts);
        if ((cst_strlen(token) == 0) ||
            (num_tokens > 500) ||  /* need an upper bound */
            (relation_head(tokrel) &&
             breakfunc(ts,token,tokrel)))
        {
            /* An end of utt, so synthesize it */
            if (utt)
            {
                utt = utt_init(utt, voice);
                utt = utt_synth_tokens(utt);
                if (feat_present(utt->features,"Interrupted"))
                {
                    delete_utterance(utt); utt = NULL;
                    break;
                }
                if (engine != NULL)
                { // HTS specific code
                    if (utt == NULL)
                    {
                        delete_utterance(utt); utt = NULL;
                        break;
                    }
                    for (s = UTT_REL_HEAD(utt, SEGMENT); s; s = item_next(s))
                        label_size++;
                    if (label_size <= 0) return FALSE;
                    label_data = cst_alloc(char *,label_size);
                    for (i = 0, s = UTT_REL_HEAD(utt, SEGMENT); s; s = item_next(s), i++)
                    {
                        label_data[i] = cst_alloc(char,HTS_MAXBUFLEN);
                        if (engine->bell_new_label_api)
                        {
                           Flite_HTS_Engine_create_label_new(s, label_data[i], HTS_MAXBUFLEN);
                        }
                        else
                        {
                           Flite_HTS_Engine_create_label(s, label_data[i], HTS_MAXBUFLEN);
                        }
                    }
                    HTS_Engine_synthesize_from_strings(engine, label_data, label_size);
                    bell_hts_get_wave(engine,utt);
                    HTS_Engine_refresh(engine);
                    for (i = 0; i < label_size; i++)
                    {
                        cst_free(label_data[i]);
                    }
                    cst_free(label_data);
                    label_size=0;
                }
                durs += flite_process_output(utt,outtype,TRUE,ad);
                delete_utterance(utt); utt = NULL;
            }
            else 
                break;

            if (ts_eof(ts)) break;

            utt = new_utterance();
            tokrel = utt_relation_create(utt, TOKEN);
            num_tokens = 0;
        }
        num_tokens++;

        t = relation_append(tokrel, NULL);
        item_set_string(t,"name",token);
        item_set_string(t,WHITESPACE,ts->whitespace);
        item_set_string(t,"punc",ts->postpunctuation);
    }

    delete_utterance(utt);
    ts_close(ts);
    return durs;
}

float bell_file_to_speech(HTS_Engine * engine, const char *filename,
                          bell_voice *voice, const char *outtype,
                          cst_audiodev *ad)
{
    cst_tokenstream *ts;

    if ((ts = ts_open(filename,
                      voice->text_whitespace,
                      voice->text_singlecharsymbols,
                      voice->text_prepunctuation,
                      voice->text_postpunctuation))
        == NULL)
    {
        cst_errmsg("Failed to open file \"%s\" for reading\n",filename);
        return -1.0;
    }
    if (BELL_HTS == voice->type){
       return bell_ts_to_speech(engine,ts,voice,outtype,ad);
    }
    else       // must be BELL_CLUSTERGEN
    {
       return bell_ts_to_speech(NULL,ts,voice,outtype,ad);
    }
}

float bell_text_to_speech(HTS_Engine * engine, const char *text,
                          bell_voice *voice, const char *outtype,
                          cst_audiodev *ad)
{
    cst_utterance *utt;
    float dur;
    int i;
    cst_item *s = NULL;
    char **label_data = NULL;
    int label_size = 0;

    utt = new_utterance();
    utt_set_input_text(utt,text);
    utt = utt_init(utt, voice);
    utt = utt_synth(utt);
    if (engine != NULL)
    { // HTS specific code
        if (utt == NULL)
        {
            delete_utterance(utt); utt = NULL;
        }
        else
        {
            for (s = UTT_REL_HEAD(utt, SEGMENT); s; s = item_next(s))
                label_size++;
            if (label_size <= 0) return FALSE;
            label_data = cst_alloc(char *,label_size);
            for (i = 0, s = UTT_REL_HEAD(utt, SEGMENT); s; s = item_next(s), i++)
            {
                label_data[i] = cst_alloc(char,HTS_MAXBUFLEN);
                if (engine->bell_new_label_api)
                {
                   Flite_HTS_Engine_create_label_new(s, label_data[i], HTS_MAXBUFLEN);
                }
                else
                {
                   Flite_HTS_Engine_create_label(s, label_data[i], HTS_MAXBUFLEN);
                }
            }
            HTS_Engine_synthesize_from_strings(engine, label_data, label_size);
            bell_hts_get_wave(engine,utt);
            HTS_Engine_refresh(engine);
            for (i = 0; i < label_size; i++)
            {
                cst_free(label_data[i]);
            }
            cst_free(label_data);
        }
    }
    dur = flite_process_output(utt,outtype,FALSE,ad);
    delete_utterance(utt);

    return dur;
}

static bell_voice *register_hts_voice(const cst_lang *lang_list)
{
    /* Voice initialization common to hts voices */
    int i;
    bell_voice *voice=NULL;
    cst_lexicon *lex=NULL;
    const char *language;

    voice  = new_voice();
    voice->name = "hts";

    /* Use the language feature to initialize the correct voice      */
    /* language = voice->language */

    /* Hack Hack Hack HTS voices don't have language set so set here */
    language = "eng";

    /* Search language list for lang_init() and lex_init() and set features */
    for (i=0; lang_list[i].lang; i++)
    {
        if (cst_streq(language,lang_list[i].lang))
        {
            (lang_list[i].lang_init)(voice);
            lex = (lang_list[i].lex_init)();
            break;
        }
    }
    if (lex == NULL)
    {
        cst_errmsg("Language is not supported. \n");
        return NULL;
    }
    voice->lexicon = lex;

    return voice;
}

static int bell_hts_find_api(HTS_Engine * engine)
// Return TRUE if voice uses new HTS engine label type
// Return FALSE if voice uses old HTS engine label type
{
    size_t i;
    HTS_ModelSet * modelset=&engine->ms;
    HTS_Model * model;
    HTS_Question * question;
    int retval = FALSE;

// Search model questions for characteristic signature of new label api
    model = modelset->duration;
    question = model->question;
    for (; question; question = question->next)
    {
       if (strstr(question->string,"=xx"))
       {
          retval = TRUE;
          break;
       }
    }
    for (i = 0; i < modelset->num_streams; i++)
    {
       model = &(modelset->stream[i]);
       question = model->question;
       for (; question; question = question->next)
       {
          if (strstr(question->string,"=xx"))
          {
             retval = TRUE;
             break;
          }
       }
    }
    model = modelset->gv;
    question = model->question;
    for (; question; question = question->next)
    {
       if (strstr(question->string,"=xx"))
       {
          retval = TRUE;
          break;
       }
    }
    return retval;
}

bell_voice * bell_voice_load(char *fn_voice, HTS_Engine * engine)
{
    int voice_type = BELL_CLUSTERGEN; // Set voice type to default
    FILE *fd;
    int first_char;
    /* Load clustergen or hts voice */
    bell_voice *voice = NULL;
    if (NULL == fn_voice)
    {
       cst_errmsg("A voice must be specified.\n");
       return NULL;
    }
    // Determine voice type
    if ((fd = bell_fopen(fn_voice,"rb")) == NULL)
    {
       cst_errmsg("Unable to open voice file.\n");
       return NULL;
    }
    // Test first character to check for HTS voice type
    first_char = bell_fgetc(fd);
    if ('[' == first_char) voice_type = BELL_HTS;
    bell_fclose(fd);

    if (BELL_CLUSTERGEN == voice_type)
    {
       voice = cst_cg_load_voice(fn_voice,flite_lang_list);
       if (voice != NULL) voice->type = BELL_CLUSTERGEN;
    }
    if (BELL_HTS == voice_type)
    {
       HTS_Engine_initialize(engine);
       if (HTS_Engine_load(engine, &fn_voice) != TRUE)
       {
          HTS_Engine_clear(engine);
       }
       else
       {
          voice = register_hts_voice(flite_lang_list);
          voice->type = BELL_HTS;
          engine->bell_new_label_api = bell_hts_find_api(engine);
       }
    }
    if (voice == NULL)
    {
       cst_errmsg("Failed to load voice\n");
    }
    return voice;
}

void bell_voice_unload(bell_voice *voice, HTS_Engine * engine)
{
   /* Unload clustergen or hts voice types */
    if (BELL_HTS == voice->type) HTS_Engine_clear(engine);
    if (voice != NULL) delete_voice(voice); /* Clean up dynamically loaded voice */
}
