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
/*                        Copyright (c) 1999                             */
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
/*               Date:  December 2000                                    */
/*************************************************************************/
/*                                                                       */
/*    Voice definition                                                   */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_voice.h"
#include "flite.h"

bell_voice *new_voice()
{
    bell_voice *v = cst_alloc(bell_voice, 1);

    v->features = new_features();
    v->ffunctions = cst_alloc(cst_ffunction, 256);
    v->phrasing_cart = NULL;
    v->pos_tagger_cart = NULL;
    v->int_cart_accents = NULL;
    v->int_cart_tones = NULL;
    v->utt_break = NULL;
    v->language = NULL;
    v->synth_methods = NULL;
    v->post_synth_func = NULL;
    v->lexicon = NULL;
    v->phoneset = NULL;
    v->cg_db = NULL;

    return v;
}

void delete_voice(bell_voice *v)
{
    if (v != NULL)
    {
        if (v->synth_methods) cst_free(v->synth_methods);
        if (v->language) cst_free(v->language);
        if (v->lexicon) delete_lexicon(v->lexicon);
        if (v->cg_db) delete_cg_db(v->cg_db);
	delete_features(v->features);
	cst_free(v->ffunctions);
	cst_free(v);
    }
}
