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
/*                        Copyright (c) 2001                             */
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
/*               Date:  January 1999                                     */
/*************************************************************************/
/*                                                                       */
/*  User defined type registration                                       */
/*                                                                       */
/*************************************************************************/

#include "cst_val.h"
#include "cst_string.h"

#define VAL_REG(NAME,NUM)                              \
extern const int cst_val_type_##NAME;                  \
const int cst_val_type_##NAME=NUM;                     \

VAL_REG(utterance,7)
VAL_REG(wave,9)
VAL_REG(track,11)
VAL_REG(uttfunc,13)
VAL_REG(relation,15)
VAL_REG(item,17)
VAL_REG(cart,19)
VAL_REG(phoneset,21)
VAL_REG(lexicon,23)
VAL_REG(userdata,25)
CST_VAL_REGISTER_TYPE_NODEL(userdata,cst_userdata)   // This generic type has not been defined elsewhere
VAL_REG(itemfunc,27)
VAL_REG(features,29)
VAL_REG(breakfunc,31)
VAL_REG(cg_db,33)

// The val takes ownership and must provide delete for these types
void val_delete_utterance(void *v);
void val_delete_wave(void *v);
void val_delete_track(void *v);
void val_delete_features(void *v);
void val_delete_cg_db(void *v);
void val_delete_lexicon(void *v);

// Table of names and delete functions (where required) for vals
const cst_val_def cst_val_defs[] = {
/* These ones are never called */
    { "int"  , NULL },                     /* 1 INT */
    { "float", NULL },                     /* 3 FLOAT */
    { "string", NULL },                    /* 5 STRING */
/* These are indexed (type/2) at print and delete time */
    { "utterance", val_delete_utterance }, /*  7 utterance */
    { "wave", val_delete_wave },           /*  9 wave */
    { "track", val_delete_track },         /* 11 track */
    { "uttfunc", NULL },                   /* 13 uttfunc */
    { "relation", NULL },                  /* 15 relation */
    { "item", NULL },                      /* 17 item */
    { "cart", NULL },                      /* 19 cart */
    { "phoneset", NULL },                  /* 21 phoneset */
    { "lexicon", val_delete_lexicon },     /* 23 lexicon */
    { "userdata", NULL },                  /* 25 userdata */
    { "itemfunc", NULL },                  /* 27 itemfunc */
    { "features", val_delete_features },   /* 29 features */
    { "breakfunc", NULL },                 /* 31 breakfunc */
    { "cg_db", val_delete_cg_db },         /* 33 cg_db */
};
