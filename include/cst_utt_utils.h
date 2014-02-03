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
/*               Date:  September 2000                                   */
/*************************************************************************/
/*                                                                       */
/*  Various utterance access/setting functions                           */
/*                                                                       */
/*************************************************************************/
#ifndef _UTT_UTILS_H__
#define _UTT_UTILS_H__

#include "cst_utterance.h"
#include "cst_wave.h"

int utt_set_wave(cst_utterance *u, cst_wave *w);
cst_wave *utt_wave(cst_utterance *u);

const char *utt_input_text(cst_utterance *u);
int utt_set_input_text(cst_utterance *u,const char *text);

#define UTT_FEAT_STRING(U,F) (feat_string((U)->features,F))
#define UTT_FEAT_INT(U,F) (feat_int((U)->features,F))
#define UTT_FEAT_VAL(U,F) (feat_val((U)->features,F))

#define UTT_SET_FEAT_STRING(U,F,V) (feat_set_string((U)->features,F,V))
#define UTT_SET_FEAT_INT(U,F,V) (feat_set_int((U)->features,F,V))
#define UTT_SET_FEAT(U,F,V) (feat_set((U)->features,F,V))

#define UTT_REL_HEAD(U,R) (relation_head(utt_relation((U),R)))

#endif
