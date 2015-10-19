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
/*                       Copyright (c) 2000-2008                         */
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
/*               Date:  September 2000                                   */
/*************************************************************************/
/*                                                                       */
/*  Basic user level functions                                           */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_cg.h"
#include "cst_error.h"
#include "cst_tokenstream.h"
#include "cst_synth.h"
#include "cst_utt_utils.h"
#include "flite.h"
#include "bell_audio.h"
#include "bell_relation_sym.h"

/* This is a global, which isn't ideal, this may change */
cst_lang flite_lang_list[20];
int flite_lang_list_length = 0;

int flite_add_lang(const char *langname,
                   void (*lang_init)(cst_voice *vox),
                   cst_lexicon *(*lex_init)())
{
    if (flite_lang_list_length < 19)
    {
        flite_lang_list[flite_lang_list_length].lang = langname;
        flite_lang_list[flite_lang_list_length].lang_init = lang_init;
        flite_lang_list[flite_lang_list_length].lex_init = lex_init;
        flite_lang_list_length++;
        flite_lang_list[flite_lang_list_length].lang = NULL;
    }

    return TRUE;
}

cst_utterance *flite_do_synth(cst_utterance *u,
                                     cst_voice *voice,
                                     cst_uttfunc synth)
{		       
    utt_init(u, voice);
    if ((*synth)(u) == NULL)
    {
	delete_utterance(u);
	return NULL;
    }
    else
	return u;
}

float flite_ts_to_speech(cst_tokenstream *ts,
                         cst_voice *voice,
                         const char *outtype)
{
    cst_utterance *utt;
    const char *token;
    cst_item *t;
    cst_relation *tokrel;
    float durs = 0;
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

float flite_text_to_speech(const char *text,
			   cst_voice *voice,
			   const char *outtype)
{
    cst_utterance *u;
    float dur;

    u = new_utterance();
    utt_set_input_text(u,text);
    u = flite_do_synth(u, voice, utt_synth);
    dur = flite_process_output(u,outtype,FALSE);
    delete_utterance(u);

    return dur;
}

float flite_process_output(cst_utterance *u, const char *outtype,
                           int append)
{
    /* Play or save (append) output to output file */
    cst_wave *w;
    float dur;

    if (!u) return 0.0;

    w = utt_wave(u);

    dur = (float)w->num_samples/(float)w->sample_rate;
	     
    if (cst_streq(outtype,"play"))
    {
	play_wave(w);
    }
#ifdef CST_AUDIO_ALSA
    else if (cst_streq(outtype,"bufferedplay"))
    {
        buffer_wave(w,get_param_int(u->features,"au_buffer_fd",-1));
    }
#endif // CST_AUDIO_ALSA
    else if (!cst_streq(outtype,"none"))
    {
        if (append)
            cst_wave_append_riff(w,outtype);
        else
            cst_wave_save_riff(w,outtype);
    }

    return dur;
}
