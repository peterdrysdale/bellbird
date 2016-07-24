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
#include "cst_synth.h"
#include "flite.h"
#include "bell_audio.h"

/* This is a global, which isn't ideal, this may change */
cst_lang flite_lang_list[20];
int flite_lang_list_length = 0;

int flite_add_lang(const char *langname,
                   void (*lang_init)(bell_voice *vox),
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

float flite_process_output(cst_utterance *u, const char *outtype,
                           int append, cst_audiodev *ad)
{
    /* Play or save (append) output to output file */
    cst_wave *w;
    float dur;

    if (!u) return 0.0;

    w = utt_wave(u);

    dur = (float)w->num_samples/(float)w->sample_rate;
	     
    if (cst_streq(outtype,"play"))
    {
	play_wave(w,ad);
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
