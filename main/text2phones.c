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
/*                                                                       */
/*  For text to phones                                                   */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>

#include "cst_voice.h"
#include "cst_utterance.h"
#include "cst_utt_utils.h"
#include "flite.h"
#include "cst_features.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"

#include "../lang/usenglish/usenglish.h"
#include "../lang/cmulex/cmu_lex.h"

static cst_utterance *no_wave_synth(cst_utterance *u)
{
    return u;
}

static cst_voice *register_cmu_us_no_wave(void)
{
    cst_voice *v = new_voice();
    cst_lexicon *lex;

    v->name = "no_wave_voice";

    /* Set up basic values for synthesizing with this voice */
    usenglish_init(v);
    feat_set_string(v->features,"name","cmu_us_no_wave");

    /* Lexicon */
    lex = cmu_lex_init();
    feat_set(v->features,"lexicon",lexicon_val(lex));

    /* Post lexical rules */
    feat_set(v->features,"postlex_func",uttfunc_val(lex->postlex));

    /* Waveform synthesis: no_wave_synth */
    feat_set(v->features,"wave_synth_func",uttfunc_val(&no_wave_synth));

    return v;
}

int main(int argc, char **argv)
{
    cst_voice *v;
    cst_utterance *u;
    cst_item *s;
    const char *name;

    if (argc != 2 || cst_streq(argv[1],"-h") || cst_streq(argv[1],"--help") ||
        cst_streq(argv[1],"-?"))
    {
        printf("text2phones: text to phones test program\n");
        printf("usage: text2phones \"word word word\"\n");
        return 0;
    }

    v = register_cmu_us_no_wave();

    u = new_utterance();
    utt_set_input_text(u,argv[1]);
    u = flite_do_synth(u, v, utt_synth);

    for (s=relation_head(utt_relation(u,SEGMENT));
	 s;
	 s = item_next(s))
    {
	name = item_feat_string(s,"name");
	printf("%s",name);
//   If its a vowel, output stress value
	if (cst_streq("+",ffeature_string(s,PH_VC)))
        {
	    if (cst_streq("1",ffeature_string(s,"R:"SYLSTRUCTURE".P.stress")))
            {
	        printf("1");
            }
            else
            {
                printf("0");
            }
        }
	printf(" ");
    }

    printf("\n");

    delete_utterance(u);
    delete_voice(v);
    
    return 0;
}
