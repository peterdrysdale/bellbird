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
/*  Copyright (c) 2005-2012  Nagoya Institute of Technology          */
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
#include "cst_alloc.h"
#include "cst_cg.h"
#include "cst_error.h"
#include "cst_features.h"
#include "cst_file.h"
#include "bell_file.h"
#include "cst_item.h"
#include "cst_regex.h"
#include "cst_relation.h"
#include "cst_string.h"
#include "cst_synth.h"
#include "cst_tokenstream.h"
#include "cst_val.h"
#include "cst_utterance.h"
#include "cst_utt_utils.h"
#include "cst_wave.h"
#include "flite.h"
#include "bell_driver.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"
#include "../hts/HTS_hidden.h"
#include "../lang/cmulex/cmu_lex.h"

static void bell_hts_get_wave(HTS_Engine * engine,cst_utterance * utt)
{
    /* Extract synthesized wave data from hts engine for use with bellbird's */
    /* existing output routines.                                             */
    cst_wave* wave;

    wave = new_wave();
    wave->num_channels=1;
    CST_WAVE_SET_NUM_SAMPLES(wave,HTS_GStreamSet_get_total_nsamples(&engine->gss));
    CST_WAVE_SAMPLES(wave) = HTS_GStreamSet_get_speech_array(&engine->gss);
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
                                           ("R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn.endtone")));

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
      tmp1 = ffeature_int(item, "R:"SYLSTRUCTURE"."POS_IN_SYL);
      tmp2 = ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_NUMPHONES);
      tmp3 = ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."POS_IN_WORD);
      tmp4 = ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."WORD_NUMSYLS);

      /* This code is to prevent memory leaks when calling syl_vowel             */
      /* via ffeature_string which accidently leaks the cst_val.                 */
      /* The cst_val created by syl_vowel is dynamically allocated hence casting */
      /* away const will let us delete is when we are finished with it.          */
      tmpsyl_vowel = (cst_val *) ffeature(item,"R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_VOWEL);

      sub_phrases = ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SUB_PHRASES);
      lisp_total_phrases = ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_PHRASES);
      labelretlen = bell_snprintf(label, labellen,
              "%s^%s-%s+%s=%s@%d_%d/A:%d_%d_%d/B:%d-%d-%d@%d-%d&%d-%d#%d-%d$%d-%d!%d-%d;%d-%d|%s/C:%d+%d+%d/D:%s_%d/E:%s+%d@%d+%d&%d+%d#%d+%d/F:%s_%d/G:%d_%d/H:%d=%d^%d=%d|%s/I:%d=%d/J:%d+%d-%d",
              strcmp(seg_pp, "0") == 0 ? "x" : seg_pp,
              strcmp(seg_p, "0") == 0 ? "x" : seg_p, seg_c,
              strcmp(seg_n, "0") == 0 ? "x" : seg_n,
              strcmp(seg_nn, "0") == 0 ? "x" : seg_nn,
              tmp1 + 1,
              tmp2 - tmp1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE".p.stress"),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE".p."ACCENTED),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE".p."SYL_NUMPHONES),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE".stress"),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."ACCENTED),
              tmp2,
              tmp3 + 1,
              tmp4 - tmp3,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_IN) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SYL_OUT) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SSYL_IN) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."SSYL_OUT) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."ASYL_IN) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."ASYL_OUT) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."LISP_DISTANCE_TO_P_STRESS),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."LISP_DISTANCE_TO_N_STRESS),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."LISP_DISTANCE_TO_P_ACCENT),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE"."LISP_DISTANCE_TO_N_ACCENT),
              val_string(tmpsyl_vowel),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE".n.stress"),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE".n."ACCENTED),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.R:"SYLLABLE".n."SYL_NUMPHONES),
              ffeature_string(item, "R:"SYLSTRUCTURE".P.P.R:"WORD".p."GPOS),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD".p."WORD_NUMSYLS),
              ffeature_string(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."GPOS),
              tmp4,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."POS_IN_PHRASE) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."WORDS_OUT),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."HTS_CONTENT_WORDS_IN) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."HTS_CONTENT_WORDS_OUT) + 1,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."LISP_DISTANCE_TO_P_CONTENT),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD"."LISP_DISTANCE_TO_N_CONTENT),
              ffeature_string(item, "R:"SYLSTRUCTURE".P.P.R:"WORD".n."GPOS),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"WORD".n."WORD_NUMSYLS),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.p."LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.p."LISP_NUM_WORDS_IN_PHRASE),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_NUM_WORDS_IN_PHRASE),
              sub_phrases + 1,
              lisp_total_phrases - sub_phrases,
              strcmp(endtone, "0") == 0 ? "NONE" : endtone,
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.n."LISP_NUM_SYLS_IN_PHRASE),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.n."LISP_NUM_WORDS_IN_PHRASE),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_SYLS),
              ffeature_int(item, "R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_TOTAL_WORDS),
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

float bell_file_to_speech(HTS_Engine * engine,
                           const char *filename, cst_voice *voice,
                           const char *outtype, const int voice_type)
{
    cst_tokenstream *ts;

    if ((ts = ts_open(filename,
         get_param_string(voice->features,
                          "text_whitespace",
                          cst_ts_default_whitespacesymbols),
         get_param_string(voice->features,"text_singlecharsymbols",""),
         get_param_string(voice->features,"text_prepunctuation",""),
         get_param_string(voice->features,"text_postpunctuation","")))
        == NULL)
    {
        cst_errmsg("Failed to open file \"%s\" for reading\n",filename);
        return -1.0;
    }
    if (voice_type == HTSMODE){
       return bell_hts_ts_to_speech(engine,ts,voice,outtype);
    }
    else       // must be CLUSTERGENMODE
    {
       return flite_ts_to_speech(ts,voice,outtype);
    }
}

float bell_hts_ts_to_speech(HTS_Engine * engine,
                         cst_tokenstream *ts, cst_voice *voice, const char *outtype)
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
    cst_breakfunc breakfunc = default_utt_break;
    cst_uttfunc utt_user_callback = 0;
    int fp;

    fp = get_param_int(voice->features,"file_start_position",0);
    if (fp > 0)
        ts_set_stream_pos(ts,fp);
    if (feat_present(voice->features,"utt_break"))
        breakfunc = val_breakfunc(feat_val(voice->features,"utt_break"));

    if (feat_present(voice->features,"utt_user_callback"))
        utt_user_callback = val_uttfunc(feat_val(voice->features,"utt_user_callback"));

    /* If its a file we write to, create and save an empty wave file */
    /* as we are going to incrementally append to it                 */
    if (!cst_streq(outtype,"play") &&
        !cst_streq(outtype,"bufferedplay") &&
        !cst_streq(outtype,"none"))
    {
        w = new_wave();
        cst_wave_resize(w,0,1);
        if (HTS_Engine_get_sampling_frequency(engine) > INT_MAX)
        {
            cst_errmsg("HTS sample rate appears unusually high");
            cst_error();
        }
        CST_WAVE_SET_SAMPLE_RATE(w,(int)HTS_Engine_get_sampling_frequency(engine));
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
            if (utt_user_callback)
                utt = (utt_user_callback)(utt);

            if (utt)
            {
                utt = flite_do_synth(utt,voice,utt_synth_tokens);
                if (feat_present(utt->features,"Interrupted"))
                {
                    delete_utterance(utt); utt = NULL;
                    break;
                }
   /* hts specific code follows */
                if (utt == NULL){
                    delete_utterance(utt); utt = NULL;
                    break;
                }
                for (s = relation_head(utt_relation(utt, SEGMENT)); s; s = item_next(s))
                    label_size++;
                if (label_size <= 0) return FALSE;
                label_data = cst_alloc(char *,label_size);
                for (i = 0, s = relation_head(utt_relation(utt, SEGMENT)); s; s = item_next(s), i++) {
                    label_data[i] = cst_alloc(char,HTS_MAXBUFLEN);
                    Flite_HTS_Engine_create_label(s, label_data[i], HTS_MAXBUFLEN);
                }
                HTS_Engine_synthesize_from_strings(engine, label_data, label_size);
                bell_hts_get_wave(engine,utt);
                HTS_Engine_refresh(engine);
                for (i = 0; i < label_size; i++){
                    cst_free(label_data[i]);
                }
                cst_free(label_data);
                label_size=0;
   /* hts specific code ends here */
                durs += flite_process_output(utt,outtype,TRUE);
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
        item_set_string(t,"whitespace",ts->whitespace);
        item_set_string(t,"prepunctuation",ts->prepunctuation);
        item_set_string(t,"punc",ts->postpunctuation);
        /* Mark it at the beginning of the token */
        item_set_int(t,"file_pos",
                     ts->file_pos-(1+ /* as we are already on the next char */
                                   cst_strlen(token)+
                                   cst_strlen(ts->prepunctuation)+
                                   cst_strlen(ts->postpunctuation)));
        item_set_int(t,"line_number",ts->line_number);
    }

    delete_utterance(utt);
    ts_close(ts);
    return durs;
}

static cst_voice *register_hts_voice(const cst_lang *lang_list)
{
    /* Voice initialization common to hts voices */
    int i;
    cst_voice *voice=NULL;
    cst_lexicon *lex=NULL;
    const char *language;

    voice  = new_voice();
    voice->name = "hts";

    /* Use the language feature to initialize the correct voice      */
    /* language = get_param_string(voice->features, "language", ""); */

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
    feat_set(voice->features,"lexicon",lexicon_val(lex));

    /* Add hts specific post lexical rules */
    feat_set(voice->features,"postlex_func",uttfunc_val(&cmu_postlex));

    return voice;
}

cst_voice * bell_voice_load(char *fn_voice, const int voice_type,
                             HTS_Engine * engine)
{
    /* Load clustergen or hts voice */
    cst_voice *voice = NULL;
    if (fn_voice == NULL)
    {
       cst_errmsg("A voice must be specified.\n");
       return NULL;
    }
    if (voice_type==CLUSTERGENMODE)
    {
       voice = cst_cg_load_voice(fn_voice,flite_lang_list);
    }
    if (voice_type==HTSMODE)
    {
       HTS_Engine_initialize(engine);
       if (HTS_Engine_load(engine, &fn_voice) != TRUE)
       {
          HTS_Engine_clear(engine);
       }
       else
       {
          voice = register_hts_voice(flite_lang_list);
       }
    }
    if (voice == NULL)
    {
       cst_errmsg("Failed to load voice\n");
    }
    return voice;
}

void bell_voice_unload(cst_voice *voice, const int voice_type, HTS_Engine * engine)
{
   /* Unload clustergen or hts voice types */
    if (voice_type==HTSMODE) HTS_Engine_clear(engine);
    if (voice != NULL) delete_voice(voice); /* Clean up dynamically loaded voice */
}
