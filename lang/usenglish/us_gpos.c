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
/*  Poor mans part of speech tagger                                      */
/*************************************************************************/

#include "cst_val.h"

static const char * const gpos_in_list[] = {
    "of",
    "for",
    "in",
    "on",
    "that",
    "with",
    "by",
    "at",
    "from",
    "as",
    "if",
    "that",
    "against",
    "about",
    "before",
    "because",
    "under",
    "after",
    "over",
    "into",
    "while",
    "without",
    "through",
    "new",
    "between",
    "among",
    "until",
    "per",
    "up",
    "down",
    0 };

static const char * const gpos_to_list[] = {
    "to",
    0 };

static const char * const gpos_det_list[] = {
    "the",
    "a",
    "an",
    "no",
    "some",
    "this",
    "each",
    "another",
    "those",
    "every",
    "all",
    "any",
    "these",
    "both",
    "neither",
    "no",
    "many",
    0 };

static const char * const gpos_md_list[] = {
    "will",
    "may",
    "would",
    "can",
    "could",
    "should",
    "must",
    "ought",
    "might",
    0 };

static const char * const gpos_cc_list[] = {
    "and",
    "but",
    "or",
    "plus",
    "yet",
    "nor",
    0 };

static const char * const gpos_wp_list[] = {
    "who",
    "what",
    "where",
    "how",
    "when",
    0 };

static const char * const gpos_pps_list[] = {
    "her",
    "his",
    "their",
    "its",
    "our",
    "mine",
    0 };

static const char * const gpos_aux_list[] = {
    "is",
    "am",
    "are",
    "was",
    "were",
    "has",
    "have",
    "had",
    "be",
    0 };

static const char * const gpos_punc_list[] = {
    ".",
    ",",
    ":",
    ";",
    "\"",
    "'",
    "(",
    "?",
    ")",
    "!",
    0 };

const char * const * const us_gpos_words[] = {
    gpos_in_list,
    gpos_to_list,
    gpos_det_list,
    gpos_md_list,
    gpos_cc_list,
    gpos_wp_list,
    gpos_pps_list,
    gpos_aux_list,
    gpos_punc_list,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_in,"in");
DEF_STATIC_CONST_VAL_STRING(gpos_to,"to");
DEF_STATIC_CONST_VAL_STRING(gpos_det,"det");
DEF_STATIC_CONST_VAL_STRING(gpos_md,"md");
DEF_STATIC_CONST_VAL_STRING(gpos_cc,"cc");
DEF_STATIC_CONST_VAL_STRING(gpos_wp,"wp");
DEF_STATIC_CONST_VAL_STRING(gpos_pps,"pps");
DEF_STATIC_CONST_VAL_STRING(gpos_aux,"aux");
DEF_STATIC_CONST_VAL_STRING(gpos_punc,"punc");

const cst_val * const us_gpos[] = {
    &gpos_in,
    &gpos_to,
    &gpos_det,
    &gpos_md,
    &gpos_cc,
    &gpos_wp,
    &gpos_pps,
    &gpos_aux,
    &gpos_punc,
    0 };
