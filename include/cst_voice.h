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
/*  Voice functions                                                      */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_VOICE_H__
#define _CST_VOICE_H__

#include "cst_val.h"
#include "cst_cart.h"
#include "cst_features.h"
#include "cst_lexicon.h"
#include "cst_tokenstream.h"
#include "cst_cg_db.h"

#define BELL_CLUSTERGEN 0
#define BELL_HTS        1

typedef int (*cst_breakfunc)(cst_tokenstream *ts,
		             const char *token,
			     cst_relation *tokens);

typedef struct bell_voice_struct {
    const char *name;
    int type; // Voice type: clustergen or hts

    cst_features *features;
    cst_ffunction *ffunctions; // Array for indexed access of features functions
    const cst_cart *phrasing_cart;
    const cst_cart *pos_tagger_cart; // Parts of speech tagger
    const cst_cart *int_cart_accents;
    const cst_cart *int_cart_tones;
    cst_breakfunc utt_break;
    char * language;
    cst_lexicon *lexicon;
    cst_cg_db *cg_db;
} bell_voice;

/* Hold pointers to language and lexicon init function */
struct cst_lang_struct {
    const char *lang;
    void (*lang_init)(bell_voice *vox);
    cst_lexicon *(*lex_init)();
};
typedef struct cst_lang_struct cst_lang;

/* Constructor functions */
bell_voice *new_voice();
void delete_voice(bell_voice *u);

#endif
