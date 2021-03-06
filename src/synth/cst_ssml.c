/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 2001-2011                          */
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
/*               Date:  June 2008                                        */
/*************************************************************************/
/*                                                                       */
/*  SSML support for flite ( http://www.w3.org/TR/speech-synthesis/ )    */
/*                                                                       */
/*  We don't use a full XML parser here for space and availability       */
/*  reasons, but this is adequate for SSML                               */
/*  This is based on some old SABLE support in flite that never got      */
/*  completed                                                            */
/*                                                                       */
/*  <ssml> </ssml>                                                       */
/*  <voice ...> </voice>                                                 */
/*     name or urls for voices                                           */
/*  <audio ...> </audio>                                                 */
/*  <!-- ... -->                                                         */
/*  <break .../>                                                         */
/*  <prosody ...> </prosody>  pitch rate duration volume                 */
/*  <emphasis ...> </emphasis>                                           */
/*  <phoneme ph="x x x"> </phoneme>                                      */
/*                                                                       */
/*  <...> ignore all others                                              */
/*                                                                       */
/*  Voice call backs (e.g. -pw and -ps) are not transfered when new      */
/*  voices are selected                                                  */
/*                                                                       */
/*************************************************************************/
#include "cst_error.h"
#include "cst_synth.h"
#include "cst_tokenstream.h"
#include "flite.h"
#include "bell_file.h"
#include "bell_relation_sym.h"
#include "bell_ff_sym.h"

static const char *ssml_singlecharsymbols_general = "<>&/\";";
static const char *ssml_singlecharsymbols_inattr = "=>;/\"";

#define SSML_DEBUG 0

static const char *ts_get_quoted_remainder(cst_tokenstream *ts)
{
    const char *q;

    q = ts_get_quoted_token(ts,'"','\\');

    return q;
}

static cst_features *ssml_get_attributes(cst_tokenstream *ts)
{
    cst_features *a = new_features();
    const char* name, *val;

    set_singlecharsymbols(ts, ssml_singlecharsymbols_inattr);

    name = ts_get(ts);
    while (!cst_streq(">",name))
    {
	if (cst_streq(name,"/"))
	    feat_set_string(a,"_type","startend");
	else
	{
	    feat_set_string(a,"_type","start");
	    feat_set_string(a,"_name0",name);
	    if (cst_streq("=",ts_get(ts)))
	    {
                val = ts_get_quoted_remainder(ts);
                feat_set_string(a,"_val0",val);
            }
	}
	if (ts_eof(ts))
	{
	    fprintf(stderr,"ssml: unexpected EOF\n");
	    delete_features(a);
	    return 0;
	}
        name = ts_get(ts);
    }
	
    set_singlecharsymbols(ts, ssml_singlecharsymbols_general);

    return a;
}

static cst_utterance *ssml_apply_tag(const char *tag,
                                     cst_features *attributes,
                                     cst_utterance *u,
                                     cst_features *word_feats)
{
    const char *wavefilename;
    cst_wave *wave;
    cst_item *t;
    cst_relation *r;
    float break_size;

#if SSML_DEBUG
    printf("SSML TAG %s\n",tag);
    bell_feat_print(attributes);
    printf("...\n");
#endif

    if (cst_streq("AUDIO",tag))
    {
        if ((cst_streq("start",feat_string(attributes,"_type"))) ||
            (cst_streq("startend",feat_string(attributes,"_type"))))
        {
            wavefilename = feat_string(attributes,"_val0");
            wave = new_wave();
            if (cst_wave_load_riff(wave,wavefilename) == BELL_IO_SUCCESS)
            {
                if (cst_streq("start",feat_string(attributes,"_type")))
                {
                    feat_set_string(word_feats,"ssml_comment","1");
                }
                feat_set(word_feats,"ssml_play_audio",userdata_val(wave));
                return NULL; /* Cause eou */
            }
        }
        else if (cst_streq("end",feat_string(attributes,"_type")))
        {
            feat_remove(word_feats,"ssml_comment");
            return NULL; /* Cause eou */
        }
    }
    else if (cst_streq("BREAK",tag))
    {
        if (u && 
            ((r = utt_relation(u,TOKEN)) != NULL) &&
            ((t = relation_tail(r)) != NULL))
        {
            item_set_string(t,"break","1");
            /* bell_feat_print(attributes); */
            if (cst_streq("size",get_param_string(attributes,"_name0","")))
            {
                break_size=feat_float(attributes,"_val0");
                item_set_float(t,"break_size",break_size);
            }
        }
    }
    else if (cst_streq("PROSODY",tag))
    {
        if (cst_streq("start",feat_string(attributes,"_type")))
        {
            /* Note SSML doesn't do stretch it does reciprical of stretch */
            if (cst_streq("rate",get_param_string(attributes,"_name0","")))
                feat_set_float(word_feats,LOCAL_DUR_STRETCH,
                               1.0/feat_float(attributes,"_val0"));
            if (cst_streq("rate",get_param_string(attributes,"_name1","")))
                feat_set_float(word_feats,LOCAL_DUR_STRETCH,
                               1.0/feat_float(attributes,"_val1"));
        }
        else if (cst_streq("end",feat_string(attributes,"_type")))
        {
            feat_remove(word_feats,LOCAL_DUR_STRETCH);
        }

    }
    else if (cst_streq("PHONEME",tag))
    {
        if (cst_streq("start",feat_string(attributes,"_type")))
        {
            if (cst_streq("ph",get_param_string(attributes,"_name0","")))
            {
                const char *ph;
                ph = feat_string(attributes,"_val0");
                feat_set_string(word_feats,PHONES,ph);
            }
        }
        else if (cst_streq("end",feat_string(attributes,"_type")))
        {
            feat_remove(word_feats,PHONES);
        }

    }

    /* do stuff */
    /* flag what to do mark or end */
    /*
      ph set attributes silence all contained tokens
      break add to previous token a break marker
      audio silence all following tokens (utt break)
        insert waveform 

    */

    return u;
}
			       
static float flite_ssml_to_speech_ts(cst_tokenstream *ts,
                                     bell_voice *voice,
                                     const char *outtype,
                                     cst_audiodev *ad)
{
    /* This is a very ugly function, that might be better written with gotos */
    /* This just doesn't seem to be properly functions -- perhaps a proper */
    /* consumer/producer threaded model might be better here -- but its */
    /* not clear.  There is so much have-to-be-done-now vs note-for-later */
    /* code, that the code is far from clear, and probably not right */
    cst_features *ssml_feats, *ssml_word_feats;
    cst_features *attributes;
    const char *token = "";
    char *tag=NULL;
    cst_utterance *utt;
    cst_relation *tokrel;
    int num_tokens;
    cst_breakfunc breakfunc = voice->utt_break;
    float durs = 0.0;
    cst_item *t;
    int ssml_eou = 0;
    cst_wave *wave, *w;

    ssml_feats = new_features();
    ssml_word_feats = new_features();
    set_singlecharsymbols(ts, ssml_singlecharsymbols_general);

    /* If its a file to write to, create and save an empty wave file */
    /* as we are going to incrementally append to it                 */
    if (!cst_streq(outtype,"play") &&
        !cst_streq(outtype,"bufferedplay") &&
        !cst_streq(outtype,"none"))
    {
	w = new_wave();
	cst_wave_resize(w,0,1);
	CST_WAVE_SET_SAMPLE_RATE(w,16000);
	cst_wave_save_riff(w,outtype);  /* an empty wave */
	delete_wave(w);
    }

    num_tokens = 0;
    utt = new_utterance();

    tokrel = utt_relation_create(utt, TOKEN);
    while (!ts_eof(ts) || num_tokens > 0)
    {
        /* printf("awb_debug prewhile %d %s\n",ssml_eou,token); */
        if (ssml_eou == 0)
            token = ts_get(ts);
        else
        {
            if (!cst_streq("<",token))
                token = ts_get(ts);
            ssml_eou = 0;
        }
	while ((cst_streq("<",token)) && (ssml_eou == 0))
	{   /* A tag -- look ahead and process it to find out how to advance */
	    tag = cst_upcase(ts_get(ts));
            /* printf("awb_debug tag is %s\n",tag); */
            if (cst_streq("/",tag)) /* an end tag */
            {
                cst_free(tag);
                tag = cst_upcase(ts_get(ts));
                attributes = ssml_get_attributes(ts);
                feat_set_string(attributes,"_type","end");
            }
            else
                attributes = ssml_get_attributes(ts);
            token = ts_get(ts);  /* skip ">" */
	    if (ssml_apply_tag(tag,attributes,utt,ssml_word_feats))
                ssml_eou = 0;
            else
                ssml_eou = 1;
            
            delete_features(attributes);
	    cst_free(tag); tag=NULL;
	}

        if ((cst_strlen(token) == 0) ||
            (num_tokens > 500) ||  /* need an upper bound */
            (ssml_eou == 1) ||  /* ssml tag was utterance break */
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

        if (feat_present(ssml_word_feats,"ssml_play_audio"))
        {
            wave = (cst_wave *)val_userdata(feat_val(ssml_word_feats,"ssml_play_audio"));
            /* Should create an utterances with the waveform in it */
            /* Have to stream it if there is streaming */
            if (utt) delete_utterance(utt);
            utt = utt_synth_wave(wave, voice);
            durs += flite_process_output(utt,outtype,TRUE,ad);
            delete_utterance(utt);

            utt = new_utterance();
            tokrel = utt_relation_create(utt, TOKEN);
            num_tokens = 0;

            feat_remove(ssml_word_feats,"ssml_play_audio");
        }
	else if (!cst_streq("<",token))
        {  /* wasn't an ssml tag */
            num_tokens++;

            t = relation_append(tokrel, NULL);
            item_set_string(t,"name",token);
            item_set_string(t,WHITESPACE,ts->whitespace);
            item_set_string(t,"punc",ts->postpunctuation);
            feat_copy_into(ssml_word_feats,item_feats(t));
        }
    }

    delete_utterance(utt);
    delete_features(ssml_feats);
    delete_features(ssml_word_feats);
    return durs;
}

float flite_ssml_file_to_speech(const char *filename,
                                bell_voice *voice,
                                const char *outtype,
                                cst_audiodev *ad)
{
    cst_tokenstream *ts;
    int fp;
    cst_wave *w;
    float d;

    if ((ts = ts_open(filename,
                      cst_ts_default_whitespacesymbols,
                      voice->text_singlecharsymbols,
                      voice->text_prepunctuation,
                      voice->text_postpunctuation))
	== NULL)
    {
	cst_errmsg("failed to open file \"%s\" for ssml reading\n",
		   filename);
	return 1;
    }
    fp = get_param_int(voice->features,"file_start_position",0);
    if (fp > 0)
        ts_set_stream_pos(ts,fp);

    /* If its a file to write to, create and save an empty wave file */
    /* as we are going to incrementally append to it                 */
    if (!cst_streq(outtype,"play") &&
        !cst_streq(outtype,"bufferedplay") &&
        !cst_streq(outtype,"none"))
    {
	w = new_wave();
	cst_wave_resize(w,0,1);
	CST_WAVE_SET_SAMPLE_RATE(w,16000);
	cst_wave_save_riff(w,outtype);  /* an empty wave */
	delete_wave(w);
    }

    d = flite_ssml_to_speech_ts(ts,voice,outtype,ad);

    ts_close(ts);
    
    return d;

}

float flite_ssml_text_to_speech(const char *text,
                                bell_voice *voice,
                                const char *outtype,
                                cst_audiodev *ad)
{
    cst_tokenstream *ts;
    int fp;
    cst_wave *w;
    float d;

    if ((ts = ts_open_string(text,
                             cst_ts_default_whitespacesymbols,
                             voice->text_singlecharsymbols,
                             voice->text_prepunctuation,
                             voice->text_postpunctuation))
	== NULL)
    {
	return 1;
    }
    fp = get_param_int(voice->features,"file_start_position",0);
    if (fp > 0)
        ts_set_stream_pos(ts,fp);

    /* If its a file to write to, create and save an empty wave file */
    /* as we are going to incrementally append to it                 */
    if (!cst_streq(outtype,"play") &&
        !cst_streq(outtype,"bufferedplay") &&
        !cst_streq(outtype,"none"))
    {
	w = new_wave();
	cst_wave_resize(w,0,1);
	CST_WAVE_SET_SAMPLE_RATE(w,16000);
	cst_wave_save_riff(w,outtype);  /* an empty wave */
	delete_wave(w);
    }

    d = flite_ssml_to_speech_ts(ts,voice,outtype,ad);

    ts_close(ts);
    
    return d;

}

