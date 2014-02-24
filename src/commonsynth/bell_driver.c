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

#include <math.h>
#include "cst_alloc.h"
#include "cst_audio.h"
#include "cst_cg.h"
#include "cst_error.h"
#include "cst_features.h"
#include "cst_file.h"
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

static void Flite_HTS_Engine_create_label(cst_item * item, char *label)
{
   char * seg_pp;
   char * seg_p;
   char * seg_c;
   char * seg_n;
   char * seg_nn;
   char * endtone;
   cst_val *tmpsyl_vowel;

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
                                                                                              ("R:SylStructure.parent.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern.endtone")));

   if (cst_streq(seg_c, "pau")) {
      /* for pause */
      if (item_next(item) != NULL) {
         sub_phrases = ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.sub_phrases");
         tmp1 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls");
         tmp2 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words");
         lisp_total_phrases = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      } else {
         sub_phrases = ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.sub_phrases");
         tmp1 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls");
         tmp2 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words");
         lisp_total_phrases = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      }
      sprintf(label, "%s^%s-%s+%s=%s@x_x/A:%d_%d_%d/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:%d+%d+%d/D:%s_%d/E:x+x@x+x&x+x#x+x/F:%s_%d/G:%d_%d/H:x=x^%d=%d|%s/I:%d=%d/J:%d+%d-%d",
              strcmp(seg_pp, "0") == 0 ? "x" : seg_pp,
              strcmp(seg_p, "0") == 0 ? "x" : seg_p,
              seg_c,
              strcmp(seg_n, "0") == 0 ? "x" : seg_n,
              strcmp(seg_nn, "0") == 0 ? "x" : seg_nn,
              ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.stress"),
              ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.accented"),
              ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.syl_numphones"),
              ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.stress"),
              ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.accented"),
              ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.syl_numphones"),
              ffeature_string(item, "p.R:SylStructure.parent.parent.R:Word.gpos"),
              ffeature_int(item, "p.R:SylStructure.parent.parent.R:Word.word_numsyls"),
              ffeature_string(item, "n.R:SylStructure.parent.parent.R:Word.gpos"),
              ffeature_int(item, "n.R:SylStructure.parent.parent.R:Word.word_numsyls"),
              ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase"),
              ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase"),
              sub_phrases + 1,
              lisp_total_phrases - sub_phrases,
              endtone,
              ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase"),
              ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase"),
              tmp1,
              tmp2,
              lisp_total_phrases);
   } else {
      /* for no pause */
      tmp1 = ffeature_int(item, "R:SylStructure.pos_in_syl");
      tmp2 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_numphones");
      tmp3 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.pos_in_word");
      tmp4 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.word_numsyls");

      /* This code is to prevent memory leaks when calling syl_vowel             */
      /* via ffeature_string which accidently leaks the cst_val.                 */
      /* The cst_val created by syl_vowel is dynamically allocated hence casting */
      /* away const will let us delete is when we are finished with it.          */
      tmpsyl_vowel = (cst_val *) ffeature(item,"R:SylStructure.parent.R:Syllable.syl_vowel");

      sub_phrases = ffeature_int(item, "R:SylStructure.parent.R:Syllable.sub_phrases");
      lisp_total_phrases = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      sprintf(label, "%s^%s-%s+%s=%s@%d_%d/A:%d_%d_%d/B:%d-%d-%d@%d-%d&%d-%d#%d-%d$%d-%d!%d-%d;%d-%d|%s/C:%d+%d+%d/D:%s_%d/E:%s+%d@%d+%d&%d+%d#%d+%d/F:%s_%d/G:%d_%d/H:%d=%d^%d=%d|%s/I:%d=%d/J:%d+%d-%d",
              strcmp(seg_pp, "0") == 0 ? "x" : seg_pp,
              strcmp(seg_p, "0") == 0 ? "x" : seg_p, seg_c,
              strcmp(seg_n, "0") == 0 ? "x" : seg_n,
              strcmp(seg_nn, "0") == 0 ? "x" : seg_nn,
              tmp1 + 1,
              tmp2 - tmp1,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.p.stress"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.p.accented"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.p.syl_numphones"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.stress"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.accented"),
              tmp2,
              tmp3 + 1,
              tmp4 - tmp3,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_in") + 1,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_out") + 1,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.ssyl_in") + 1,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.ssyl_out") + 1,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.asyl_in") + 1,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.asyl_out") + 1,
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_p_stress"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_n_stress"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_p_accent"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_n_accent"),
              val_string(tmpsyl_vowel),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.n.stress"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.n.accented"),
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.n.syl_numphones"),
              ffeature_string(item, "R:SylStructure.parent.parent.R:Word.p.gpos"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.p.word_numsyls"),
              ffeature_string(item, "R:SylStructure.parent.parent.R:Word.gpos"),
              tmp4,
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.pos_in_phrase") + 1,
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.words_out"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.hts_content_words_in") + 1,
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.hts_content_words_out") + 1,
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.lisp_distance_to_p_content"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.lisp_distance_to_n_content"),
              ffeature_string(item, "R:SylStructure.parent.parent.R:Word.n.gpos"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.n.word_numsyls"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.p.lisp_num_syls_in_phrase"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.p.lisp_num_words_in_phrase"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase"),
              sub_phrases + 1,
              lisp_total_phrases - sub_phrases,
              strcmp(endtone, "0") == 0 ? "NONE" : endtone,
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.n.lisp_num_syls_in_phrase"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.n.lisp_num_words_in_phrase"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls"),
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words"),
              lisp_total_phrases);
      delete_val(tmpsyl_vowel);
   }
   cst_free(seg_pp);
   cst_free(seg_p);
   cst_free(seg_c);
   cst_free(seg_n);
   cst_free(seg_nn);
   cst_free(endtone);
}

float bell_hts_file_to_speech(HTS_Engine * engine, nitech_engine * ntengine,
                           const char *filename, cst_voice *voice,
                           const char *outtype, const int voice_type)
{
    cst_tokenstream *ts;

    if ((ts = ts_open(filename,
         get_param_string(voice->features,"text_whitespace",NULL),
         get_param_string(voice->features,"text_singlecharsymbols",NULL),
         get_param_string(voice->features,"text_prepunctuation",NULL),
         get_param_string(voice->features,"text_postpunctuation",NULL)))
        == NULL)
    {
        cst_errmsg("Failed to open file \"%s\" for reading\n",filename);
        cst_error();
    }
    return bell_hts_ts_to_speech(engine,ntengine,ts,voice,outtype,voice_type);
}

float bell_hts_ts_to_speech(HTS_Engine * engine, nitech_engine * ntengine,
                         cst_tokenstream *ts, cst_voice *voice, const char *outtype,
                         const int voice_type)
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
        !cst_streq(outtype,"none") &&
        !cst_streq(outtype,"stream"))
    {
        w = new_wave();
        cst_wave_resize(w,0,1);
        if (voice_type==HTSMODE)
        {
            CST_WAVE_SET_SAMPLE_RATE(w,48000);
        } 
        else if (voice_type==NITECHMODE)
        {
            CST_WAVE_SET_SAMPLE_RATE(w,16000);
        }
        cst_wave_save_riff(w,outtype);  /* an empty wave */
        delete_wave(w);
    }

    num_tokens = 0;
    utt = new_utterance();
    tokrel = utt_relation_create(utt, "Token");
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
                for (s = relation_head(utt_relation(utt, "Segment")); s; s = item_next(s))
                    label_size++;
                if (label_size <= 0) return FALSE;
                label_data = cst_alloc(char *,label_size);
                for (i = 0, s = relation_head(utt_relation(utt, "Segment")); s; s = item_next(s), i++) {
                    label_data[i] = cst_alloc(char,HTS_MAXBUFLEN);
                    Flite_HTS_Engine_create_label(s, label_data[i]);
                }
                if (voice_type==HTSMODE)
                {
                    HTS_Engine_synthesize_from_strings(engine,
                                              label_data, label_size);
                    bell_hts_get_wave(engine,utt);
                    HTS_Engine_refresh(engine);
                }
                else if (voice_type==NITECHMODE)
                {
                    ntengine->utt=utt;
                    nitech_engine_synthesize_from_strings(ntengine,
                                              label_data, label_size);
                }
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
            tokrel = utt_relation_create(utt, "Token");
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
    /* Voice initialization common to hts and nitech voices */
    int i;
    cst_voice *voice=NULL;
    cst_lexicon *lex=NULL;
    const char *language;

    voice  = new_voice();
    voice->name = "hts";

    /* Use the language feature to initialize the correct voice      */
    /* language = get_param_string(voice->features, "language", ""); */

    /* Hack Hack Hack HTS and nitech voices don't have language set so set here */
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
                             HTS_Engine * engine, nitech_engine * ntengine)
{
    /* Load clustergen, hts or nitech voice */
    cst_voice *voice = NULL;
    if (fn_voice == NULL)
    {
       cst_errmsg("A voice must be specified.\n");
       cst_error();
    }
    if (voice_type==CLUSTERGENMODE)
    {
       voice = cst_cg_load_voice(fn_voice,flite_lang_list);
    }
    if (voice_type==HTSMODE)
    {
       HTS_Engine_initialize(engine);
       if ( HTS_Engine_load(engine, &fn_voice, 1) != TRUE)
       {
          cst_errmsg("HTS voice cannot be loaded.\n");
          HTS_Engine_clear(engine);
          cst_error();
       }
       voice = register_hts_voice(flite_lang_list);
    }
    if (voice_type==NITECHMODE)
    {
       nitech_engine_initialize(ntengine, fn_voice);
       voice = register_hts_voice(flite_lang_list);
    }
    if (voice == NULL)
    {
       cst_errmsg("Failed to load voice\n");
       cst_error();
    }
    return voice;
}

void bell_voice_unload(cst_voice *voice, const int voice_type, HTS_Engine * engine,
                                     nitech_engine * ntengine)
{
   /* Unload clustergen, hts or nitech voice types */
    if (voice_type==HTSMODE) HTS_Engine_clear(engine);
    if (voice_type==NITECHMODE) nitech_engine_clear(ntengine);  
    if (voice != NULL) delete_voice(voice); /* Clean up dynamically loaded voice */
}
