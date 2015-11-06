/*************************************************************************/
/*                This file contains code from flite and flite+hts.      */
/*                It has been modified for Bellbird.                     */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notices           */
/*                are included below.                                    */
/*************************************************************************/
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  January 2001                                     */
/*************************************************************************/
/* ----------------------------------------------------------------- */
/*           The English TTS System "Flite+hts_engine"               */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2005-2011  Nagoya Institute of Technology          */
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
#include <stdio.h>

#include "cst_error.h"
#include "cst_features.h"
#include "cst_file.h"
#include "cst_regex.h"
#include "cst_string.h"
#include "cst_utt_utils.h"
#include "flite.h"
#include "bell_audio.h"
#include "bell_file.h"
#include "bell_driver.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"
#include "../lang/cmulex/cmu_lex.h"

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

/* Its not very appropriate that these are declared here */
void usenglish_init(cst_voice *v);
cst_lexicon *cmu_lex_init(void);

static void bellbird_version()
{
    printf("  Copyright (c) 1999-2015. Details of copyright may be seen in COPYING file\n");
    printf("  version: %s \n", PACKAGE_STRING);
}

static void bellbird_usage()
{
    printf("bellbird: a speech synthesizer\n");
    bellbird_version();
    printf("usage: bellbird [options ] [-f infile] [ -o outfile]\n"
          "  Converts text in infile to a synthesized speech in wav format in outfile.\n"
          "  Other options must appear before these options\n"
          "  --version              Output bellbird version number\n"
          "  --help                 Output usage string\n"
          "  -o outfile             Explicitly set output wav audio filename\n"
          "  -f infile              Explicitly set input filename\n"
          "  --voice VOICEFILE      Use voice clustergen voice at VOICEFILE \n"
          "  --htsvoice VOICEFILE   Use voice hts voice at VOICEFILE \n"
          "  --add_dict FILENAME    Add dictionary addenda from FILENAME\n"
          "  --startpos n   Read input file from byte n (int), skipping n-1 bytes\n"
          "  --printphones  Print phones while generating speech\n"
          "  --printtext    Print text while generating speech\n"
          "  --seti F=V     Set int feature\n"
          "  --setf F=V     Set float feature\n"
          "  --sets F=V     Set string feature\n"
          " Clustergen specific options:\n"
          "  -ssml          Read input text/file in ssml mode\n"
          "  -t TEXT        Explicitly set input textstring\n"
          " HTS specific options:                                   [  def][ min--max]\n"
          "  -b  f           postfiltering coefficient              [  0.0][-0.8--0.8]\n"
          "  -r  f           speech speed rate                      [  1.0][ 0.0--   ]\n"
          "  -fm f           additional half-tone                   [  0.0][    --   ]\n"
          "  -u  f           voiced/unvoiced threshold              [  0.5][ 0.0--1.0]\n"
          "  -jm f           weight of GV for spectrum              [  1.0][ 0.0--2.0]\n"
          "  -jf f           weight of GV for Log F0                [  1.0][ 0.0--2.0]\n"
          "\n"
          " infile:\n"
          "   text file  dash(-) reads from stdin\n"
          " outfile:\n"
          "   wav file   dash(-)       sends to stdout\n"
          "              none          discards for benchmarking\n"
          "              play          sends to audio device\n"
          "              bufferedplay  sends to audio device via a buffer to reduce\n"
          "                            pauses between utterances\n");
    exit(0);
}

static cst_utterance *bell_print_text(cst_utterance *u)
{
//  print text (tokens without punctuation) from utterance
    cst_item *token;

    for (token=relation_head(utt_relation(u,TOKEN));
	 token;
	 token=item_next(token))
    {
	bell_fprintf(stdout,"%s ",item_feat_string(token,"name"));
    }
    bell_fprintf(stdout,"\n");

    return u;
}

static cst_utterance *bell_print_phones(cst_utterance *u)
{
//  print phones with stress from utterance
    cst_item *phone;

    for (phone=relation_head(utt_relation(u,SEGMENT));
         phone;
         phone=item_next(phone))
    {
        bell_fprintf(stdout,"%s",item_feat_string(phone,"name"));
	if (cst_streq("+",ffeature_string(phone,PH_VC)))
        {
            if (cst_streq("1",ffeature_string(phone,"R:"SYLSTRUCTURE".P.stress")))
            {
                bell_fprintf(stdout,"1");
            }
            else
            {
                bell_fprintf(stdout,"0");
            }
        }
        bell_fprintf(stdout," ");

    }
    bell_fprintf(stdout,"\n");

    return u;
}

static void ef_set(cst_features *f,const char *fv,const char *type)
{
    /* set feature from fv (F=V) */
    const char *val;
    char *feat;
    float tempfloat;
    int tempint;

    if ((val = strchr(fv,'=')) == 0)
    {
	fprintf(stderr,
		"bellbird: can't find '=' in featval \"%s\", ignoring it\n",
		fv);
    }
    else
    {
	feat = cst_strdup(fv);
	feat[cst_strlen(fv)-cst_strlen(val)] = '\0'; /* replace equals sign with null */
	val++;     /* increment past the former equals sign */
	if ( cst_streq("int",type) )
            if (bell_validate_atoi(val,&tempint))
            {
	        feat_set_int(f,cst_strdup(feat),tempint);
            }
            else
            {
                printf("Failed to set int val '%s'.\n",fv);
            }
	else if ( cst_streq("float",type) )
            if (bell_validate_atof(val,&tempfloat))
            {
	        feat_set_float(f,cst_strdup(feat),tempfloat);
            }
            else
            {
                printf("Failed to set float value '%s'.\n",fv);
            }
	else
	    feat_set_string(f,cst_strdup(feat),val);
        cst_free(feat);
    }
}

int main(int argc, char **argv)
{
    cst_voice *voice = NULL;
    const char *texttoread=NULL;    /* Text to be read either filename or string */
    const char *outtype = "play";   /* default is to play */
    char *fn_voice = NULL;
    int i;
    float durs;
    int debug_durs = 0;
    int voice_type=CLUSTERGENMODE; /* default is clustergen voice */
    int explicit_filename, explicit_text;
    int ssml_mode = FALSE;         /* default to non-SSML reading */
    cst_features *extra_feats = NULL;
    const char *lex_addenda_file = NULL;
    float tempfloat;
    int tempint;
    cst_lexicon *lex;
#ifdef CST_AUDIO_ALSA
    int fd = -1;     // file descriptor for pipe to audio scheduler if used
#endif //CST_AUDIO_ALSA

    HTS_Engine engine;

    explicit_text = explicit_filename = FALSE;

#ifdef CST_AUDIO_ALSA
//  fork an audio scheduler if needed before any calloc() for efficiency reasons
    for (i=0; i< argc; i++)
    {
        if (cst_streq(argv[i],"-o") && cst_streq(argv[i+1],"bufferedplay"))
        {
            fd = audio_scheduler(); //start audio scheduler
            extra_feats = new_features();
            feat_set_int(extra_feats,"au_buffer_fd",fd);
        }
    }
#endif //CST_AUDIO_ALSA

    if (NULL == extra_feats) extra_feats = new_features();

    for (i = 0; i < argc; i++)
    {
        if (cst_streq(argv[i],"--version"))
	{
	    bellbird_version();
	    return 1;
	}
        else if (cst_streq(argv[i],"--voice") && (i+1 < argc))
        {
           fn_voice = argv[++i];
        }
        else if (cst_streq(argv[i],"--htsvoice") && (i+1 < argc))
        {
           voice_type=HTSMODE;
           fn_voice = argv[++i];
        }
        else if (cst_streq(argv[i],"-h") || cst_streq(argv[i],"--help") ||
                cst_streq(argv[i],"-?"))
        {
	    bellbird_usage();
        }
    }

    flite_add_lang("eng",usenglish_init,cmu_lex_init); /* removed set_lang_list */
    voice = bell_voice_load(fn_voice,voice_type,&engine);
    if (NULL == voice) exit(1);

    for (i=1; i < argc; i++)
    {
	if ( cst_streq(argv[i],"-o") && (i+1 < argc) )
	{
	    outtype = argv[++i];
	}
	else if ( cst_streq(argv[i],"--voice") && (i+1 < argc) )
	{
          /* already loaded above */
	    i++;
	}
        else if ( cst_streq(argv[i],"--htsvoice") && (i+1 < argc) )
        {
           /* Already loaded above */
           i++;
        }
	else if ( cst_streq(argv[i],"--add_dict") && (i+1 < argc) )
	{
            lex_addenda_file = argv[++i];
	}
	else if ( cst_streq(argv[i],"-f") && (i+1 < argc) )
	{
	   texttoread = argv[++i];
	    explicit_filename = TRUE;
	}
        else if ( cst_streq(argv[i],"--startpos") && (i+1 < argc) )
        {
            if (bell_validate_atoi(argv[++i],&tempint))
            {
                feat_set_int(extra_feats,"file_start_position",tempint);
            }
            else
            {
                printf("Failed to set '--startpos' command option\n");
            }
        }
        else if (cst_streq(argv[i],"--printphones"))
        {
            feat_set(extra_feats,"post_synth_hook_func",
                     uttfunc_val(&bell_print_phones));
        }
	else if (cst_streq(argv[i],"--printtext"))
	{
	    feat_set(extra_feats,"post_synth_hook_func",
		     uttfunc_val(&bell_print_text));
	}
        else if (cst_streq(argv[i],"-ssml"))
        {
            ssml_mode = TRUE;
        }
	else if ( cst_streq(argv[i],"--seti") && (i+1 < argc) )
	{
	    ef_set(extra_feats,argv[++i],"int");
	}
	else if ( cst_streq(argv[i],"--setf") && (i+1 < argc) )
	{
	    ef_set(extra_feats,argv[++i],"float");
	}
	else if ( cst_streq(argv[i],"--sets") && (i+1 < argc) )
	{
	    ef_set(extra_feats,argv[++i],"string");
	}
 	else if ( cst_streq(argv[i],"-t") && (i+1 < argc) )
	{
	    texttoread = argv[++i];
	    explicit_text = TRUE;
	}          
        else if (voice_type==HTSMODE) /* hts specific options */
        {
           if ( cst_streq(argv[i],"-b") && (i+1 < argc) )
           {
              if (bell_validate_atof(argv[++i],&tempfloat))
              {
                 HTS_Engine_set_beta(&engine, tempfloat);
              }
              else
              {
                 printf("Failed to set '-b' command option\n");
              }
           }
           else if ( cst_streq(argv[i],"-r") && (i+1 < argc) )
           {
              if (bell_validate_atof(argv[++i],&tempfloat))
              {
                 HTS_Engine_set_speed(&engine, tempfloat);
              }
              else
              {
                  printf("Failed to set '-r' command option\n");
              }
           }
           else if ( cst_streq(argv[i],"-fm") && (i+1 < argc) )
           {
              if (bell_validate_atof(argv[++i],&tempfloat))
              {
                 HTS_Engine_add_half_tone(&engine, tempfloat);
              }
              else
              {
                 printf("Failed to set '-fm' command option\n");
              }
           }
           else if ( cst_streq(argv[i],"-u") && (i+1 < argc) )
           {
              if (bell_validate_atof(argv[++i],&tempfloat))
              {
                 HTS_Engine_set_msd_threshold(&engine, 1, tempfloat);
              }
              else
              {
                 printf("Failed to set '-u' command option\n");
              }

           }
           else if ( cst_streq(argv[i],"-jm") && (i+1 < argc) )
           {
              if (bell_validate_atof(argv[++i],&tempfloat))
              {

               HTS_Engine_set_gv_weight(&engine, 0, tempfloat);
              }
              else
              {
                 printf("Failed to set '-jm' command option\n");
              }

           }
	   else if ( cst_streq(argv[i],"-jf") && (i+1 < argc) )
           {
              if (bell_validate_atof(argv[++i],&tempfloat))
              {

               HTS_Engine_set_gv_weight(&engine, 1, tempfloat);
              }
              else
              {
                 printf("Failed to set '-jf' command option\n");
              }

           }
        } /* end of hts specific options */
    }

    if (NULL == texttoread) texttoread = "-";  /* default to stdin  */

   /* Add extra command line features to already loaded voice and clean up */
    feat_copy_into(extra_feats,voice->features);
    delete_features(extra_feats);

    if (lex_addenda_file)
    {
        lex = val_lexicon(feat_val(voice->features,"lexicon"));
        lex->lex_addenda = cst_lex_load_addenda(lex,lex_addenda_file);
    }

    if (voice_type==HTSMODE)
    {
       bell_file_to_speech(&engine, texttoread, voice, outtype, voice_type);
    }
    else if (voice_type==CLUSTERGENMODE)
    {
       durs = 0.0;

       if ((strchr(texttoread,' ') && !explicit_filename) || explicit_text)
       {
           if (ssml_mode)
               durs = flite_ssml_text_to_speech(texttoread,voice,outtype);
           else
               durs = flite_text_to_speech(texttoread,voice,outtype);
       }
       else
       {
           if (ssml_mode)
               durs = flite_ssml_file_to_speech(texttoread,voice,outtype);
           else
               durs = bell_file_to_speech(&engine,texttoread,voice,outtype,voice_type);
       }

       if (debug_durs && (durs != 0.0)) printf("Durs was %f at end of run",durs);

    } /* end of voice_type==CLUSTERGENMODE */

#ifdef CST_AUDIO_ALSA
//  Close the audio scheduler if it has been started
    fd = get_param_int(voice->features,"au_buffer_fd",-1);
    if (fd != -1)
        audio_scheduler_close(fd);
#endif // CST_AUDIO_ALSA
    bell_voice_unload(voice,voice_type,&engine);
    voice=NULL;
    return 0;
}
