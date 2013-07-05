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

#include "cst_audio.h"
#include "cst_error.h"
#include "cst_features.h"
#include "cst_file.h"
#include "cst_regex.h"
#include "cst_string.h"
#include "cst_utt_utils.h"
#include "flite.h"
#include "bell_driver.h"
#include "../lang/cmulex/cmu_lex.h"

/* Its not very appropriate that these are declared here */
void usenglish_init(cst_voice *v);
cst_lexicon *cmu_lex_init(void);

static void bellbird_version()
{
    printf("  Copyright (c) 1999-2012. Details of copyright may be seen in COPYING file\n");
    printf("  version: bellbird-0.1.7-unstable July 2013 \n");
}

static void bellbird_usage()
{
    printf("bellbird: a speech synthesizer\n");
    bellbird_version();
    printf("usage: bellbird [options ] [-f infile] [ -o outfile]\n"
          "  Converts text in infile to a synthesized speech in wav format in outfile.\n"
          "  Other options must appear before these options\n"
          "  --version      Output bellbird version number\n"
          "  --help         Output usage string\n"
          "  -o outfile     Explicitly set output wav audio filename\n"
          "  -f infile      Explicitly set input filename\n"
          "  --seti F=V     Set int feature\n"
          "  --setf F=V     Set float feature\n"
          "  --sets F=V     Set string feature\n"
          "  --startpos n   Read input file from byte n (int), skipping n-1 bytes\n"
          "  --voice VOICEFILE      Use voice clustergen voice at VOICEFILE \n"
          "  --nitechvoice VOICEDIR Use voice nitech voice at VOICEDIR \n"
          "  --htsvoice VOICEFILE   Use voice hts voice at VOICEFILE \n"
          " Clustergen specific options:"
          "  --add_lex FILENAME add lex addenda from FILENAME\n"
          "  -pw         Print words\n"
          "  -ps         Print segments\n"
          "  -pr RelName  Print relation RelName\n"
          "  -ssml          Read input text/file in ssml mode\n"
          "  -t TEXT        Explicitly set input textstring\n"
          "  -p PHONES      Explicitly set input textstring and synthesize as phones\n"
          " HTS specific options:                                         [  def][ min--max]\n"
          "  -s  i           sampling frequency                           [16000][   1--48000]\n"
          "  -p  i           frame period (point)                         [   80][   1--]\n"
          "  -a  f           all-pass constant                            [ 0.42][ 0.0--1.0]\n"
          "  -b  f           postfiltering coefficient                    [  0.0][-0.8--0.8]\n"
          "  -r  f           speech speed rate                            [  1.0][ 0.0--    ]\n"
          "  -fm             additional half-tone                         [  0.0][    --    ]\n"
          "  -u  f           voiced/unvoiced threshold                    [  0.5][ 0.0--1.0]\n"
          "  -jm f           weight of GV for spectrum                    [  1.0][ 0.0--2.0]\n"
          "  -jf f           weight of GV for Log F0                      [  1.0][ 0.0--2.0]\n"
          "\n"
          " infile:\n"
          "   text file  dash(-) reads from stdin\n"
          " outfile:\n"
          "   wav file   dash(-) sends to stdout\n"
          "              none  discards for benchmarking\n"
          "              play  sends to audio device\n");
    exit(0);
}

static cst_utterance *print_info(cst_utterance *u)
{
    cst_item *item;
    const char *relname;

    relname = utt_feat_string(u,"print_info_relation");
    for (item=relation_head(utt_relation(u,relname)); 
	 item; 
	 item=item_next(item))
    {
	printf("%s ",item_feat_string(item,"name"));
#if 0
        if (cst_streq("+",ffeature_string(item,"ph_vc")))
            printf("%s",ffeature_string(item,"R:SylStructure.parent.stress"));
        printf(" ");
#endif
    }
    printf("\n");

    return u;
}

static void ef_set(cst_features *f,const char *fv,const char *type)
{
    /* set feature from fv (F=V) */
    const char *val;
    char *feat;

    if ((val = cst_strchr(fv,'=')) == 0)
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
	    feat_set_int(f,cst_strdup(feat),atoi(val));
	else if ( cst_streq("float",type) )
	    feat_set_float(f,cst_strdup(feat),atof(val));
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
    int explicit_filename, explicit_text, explicit_phones;
    int ssml_mode = FALSE;         /* default to non-SSML reading */
    cst_features *extra_feats = NULL;
    const char *lex_addenda_file = NULL;
    cst_audio_streaming_info *asi;

    HTS_Engine engine;
    nitech_engine ntengine;

    explicit_text = explicit_filename = explicit_phones = FALSE;
    extra_feats = new_features();

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
        else if (cst_streq(argv[i],"--nitechvoice") && (i+1 < argc))
        {
           voice_type=NITECHMODE;
           fn_voice = argv[++i];
        }
        else if (cst_streq(argv[i],"-h") || cst_streq(argv[i],"--help") ||
                cst_streq(argv[i],"-?"))
        {
	    bellbird_usage();
        }
    }

    flite_add_lang("eng",usenglish_init,cmu_lex_init); /* removed set_lang_list */
    voice = bell_voice_load(fn_voice,voice_type,&engine,&ntengine);

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
	else if ( cst_streq(argv[i],"--nitechvoice") && (i+1 < argc) )
	{
           /* Already loaded above */
           i++;
        }
        else if ( cst_streq(argv[i],"--htsvoice") && (i+1 < argc) )
        {
           /* Already loaded above */
           i++;
        }
	else if ( cst_streq(argv[i],"--add_lex") && (i+1 < argc) )
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
            feat_set_int(extra_feats,"file_start_position",atoi(argv[++i]));
        }
	else if (cst_streq(argv[i],"-pw"))
	{
	    feat_set_string(extra_feats,"print_info_relation","Word");
	    feat_set(extra_feats,"post_synth_hook_func",
		     uttfunc_val(&print_info));
	}
	else if (cst_streq(argv[i],"-ps"))
	{
	    feat_set_string(extra_feats,"print_info_relation","Segment");
	    feat_set(extra_feats,"post_synth_hook_func",
		     uttfunc_val(&print_info));
	}
        else if (cst_streq(argv[i],"-ssml"))
        {
            ssml_mode = TRUE;
        }
	else if ( cst_streq(argv[i],"-pr") && (i+1 < argc) )
	{
	    feat_set_string(extra_feats,"print_info_relation",argv[++i]);
	    feat_set(extra_feats,"post_synth_hook_func",
		     uttfunc_val(&print_info));
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
	else if ( cst_streq(argv[i],"-p") && (i+1 < argc) )
	{
	    texttoread = argv[++i];
	    explicit_phones = TRUE;
	}
 	else if ( cst_streq(argv[i],"-t") && (i+1 < argc) )
	{
	    texttoread = argv[++i];
	    explicit_text = TRUE;
	}          
        else if (voice_type==HTSMODE) /* hts specific options */
        {
           if ( cst_streq(argv[i],"-s") && (i+1 < argc) )
           {
               HTS_Engine_set_sampling_frequency(&engine, (size_t) atoi(argv[++i]));
           }
           else if ( cst_streq(argv[i],"-p") && (i+1 < argc) )
           {
               HTS_Engine_set_fperiod(&engine, (size_t) atoi(argv[++i]));
           }
           else if ( cst_streq(argv[i],"-a") && (i+1 < argc) )
           {
               HTS_Engine_set_alpha(&engine, atof(argv[++i]));
           }
           else if ( cst_streq(argv[i],"-b") && (i+1 < argc) )
           {
               HTS_Engine_set_beta(&engine, atof(argv[++i]));
           }
           else if ( cst_streq(argv[i],"-r") && (i+1 < argc) )
           {
               HTS_Engine_set_speed(&engine, atof(argv[++i]));
           }
           else if ( cst_streq(argv[i],"-fm") && (i+1 < argc) )
           {
               HTS_Engine_add_half_tone(&engine, atof(argv[++i]));
           }
           else if ( cst_streq(argv[i],"-u") && (i+1 < argc) )
           {
               HTS_Engine_set_msd_threshold(&engine, 1, atof(argv[++i]));
           }
           else if ( cst_streq(argv[i],"-jm") && (i+1 < argc) )
           {
               HTS_Engine_set_gv_weight(&engine, 0, atof(argv[++i]));
           }
	   else if ( cst_streq(argv[i],"-jf") && (i+1 < argc) )
           {
               HTS_Engine_set_gv_weight(&engine, 1, atof(argv[++i]));
           }
        } /* end of hts specific options */
    }

    if (texttoread == NULL) texttoread = "-";  /* default to stdin  */

   /* Add extra command line features to already loaded voice and clean up */
    feat_copy_into(extra_feats,voice->features);
    delete_features(extra_feats);

    if (voice_type==NITECHMODE || voice_type==HTSMODE)
    {
       bell_hts_file_to_speech(&engine, &ntengine, texttoread, voice, outtype, voice_type);
    }
    else if (voice_type==CLUSTERGENMODE)
    {
       durs = 0.0;

       if (lex_addenda_file) flite_voice_add_lex_addenda(voice,lex_addenda_file);

       if (cst_streq("stream",outtype))
       {
           asi = new_audio_streaming_info();
           asi->asc = audio_stream_chunk;
           feat_set(voice->features,"streaming_info",audio_streaming_info_val(asi));
       }

       if (explicit_phones)
       {
	   durs = flite_phones_to_speech(texttoread,voice,outtype);
       }
       else if ((strchr(texttoread,' ') && !explicit_filename) || explicit_text)
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
               durs = flite_file_to_speech(texttoread,voice,outtype);
       }

       if (debug_durs) printf("Durs was %f at end of run",durs);

    } /* end of voice_type==CLUSTERGENMODE */

    bell_voice_unload(voice,voice_type,&engine,&ntengine);
    return 0;
}
