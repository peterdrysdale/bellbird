/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
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

#include "cst_phoneset.h"
#include "cst_regex.h"
#include "cst_ffeatures.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"
#include "us_ffeatures.h"

static const cst_val *gpos(const cst_item *word);

DEF_STATIC_CONST_VAL_STRING(val_string_content,"content");

const cst_val *accented(const cst_item *p); /* defined in cst_ffeatures.c */

static const cst_val *gpos(const cst_item *word)
{
    /* Guess at part of speech (function/content) */
    const char *w;
    int s,t;

    w = item_feat_string(word,"name");
    for (s=0; us_gpos[s]; s++)
    {
	for (t=0; us_gpos_words[s][t]; t++)
	    if (cst_streq(w,us_gpos_words[s][t]))
		return us_gpos[s];
    }
    return &val_string_content;
}

/* 21 by Toda-san */
static const cst_val *lisp_distance_to_p_stress(const cst_item *syl)
{
    const cst_item *s, *fs;
    int c;

    s=item_as(syl,SYLLABLE);
    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.d1.R:"SYLSTRUCTURE".d1");
    if (item_equal(s,fs)) return val_string_n(0);
    s=item_prev(s);
    for (c=1; s && (!item_equal(s,fs)) && (c < CST_CONST_INT_MAX); s=item_prev(s),c++)
    {
        if (strcmp("1", ffeature_string(s,"stress")) == 0) return val_string_n(c);
    }
    if (strcmp("1", ffeature_string(s,"stress")) == 0)
    {
        return val_string_n(c);
    }
    else
    {
        return val_string_n(0);
    }
}

/* 22 by Toda-san */
static const cst_val *lisp_distance_to_n_stress(const cst_item *syl)
{
    const cst_item *s, *fs;
    int c;

    s=item_as(syl,SYLLABLE);
    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn");
    if (item_equal(s,fs)) return val_string_n(0);
    s=item_next(s);
    for (c=1; s && (!item_equal(s,fs)) && (c < CST_CONST_INT_MAX); s=item_next(s),c++)
    {
        if (strcmp("1", ffeature_string(s,"stress")) == 0) return val_string_n(c);
    }
    if (strcmp("1", ffeature_string(s,"stress")) == 0)
    {
        return val_string_n(c);
    }
    else
    {
        return val_string_n(0);
    }
}

/* 23 by Toda-san */
static const cst_val *lisp_distance_to_p_accent(const cst_item *syl)
{
    const cst_item *s, *fs;
    int c;

    s=item_as(syl,SYLLABLE);
    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.d1.R:"SYLSTRUCTURE".d1");
    if (item_equal(s,fs)) return val_string_n(0);
    s=item_prev(s);
    for (c=1; s && (!item_equal(s,fs)) && (c < CST_CONST_INT_MAX); s=item_prev(s),c++)
    {
        if (val_int(accented(s))) return val_string_n(c);
    }
    if (val_int(accented(s)))
    {
        return val_string_n(c);
    }
    else
    {
        return val_string_n(0);
    }
}

/* 24 by Toda-san */
static const cst_val *lisp_distance_to_n_accent(const cst_item *syl)
{
    const cst_item *s, *fs;
    int c;

    s=item_as(syl,SYLLABLE);
    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn");
    if (item_equal(s,fs)) return val_string_n(0);
    s=item_next(s);
    for (c=1; s && (!item_equal(s,fs)) && (c < CST_CONST_INT_MAX); s=item_next(s),c++)
    {
        if (val_int(accented(s))) return val_string_n(c);
    }
    if (val_int(accented(s)))
    {
        return val_string_n(c);
    }
    else
    {
        return val_string_n(0);
    }
}

/* 33 */
static const cst_val *words_out(const cst_item *syl)
{
    const cst_item *ss,*p;
    int c = 0;

    ss = item_as(syl,PHRASE);
    for (p = ss;p;p=item_next(p),c++);
    return val_string_n(c);
}

/* 34 by Toda-san */
static const cst_val *hts_content_words_in(const cst_item *word)
{
    const cst_item *p,*fs;
    int c;
  
    fs = path_to_item(word,"R:"PHRASE".P.d1");
    for (c=0,p=word; p && (!item_equal(p,fs)) && (c < CST_CONST_INT_MAX); p=item_prev(p))
    {
        if (cst_streq("content", ffeature_string(p,GPOS))) c++;
    }
    return val_string_n(c);  /* its used randomly as int and float */
}

/* 35 by Toda-san */
static const cst_val *hts_content_words_out(const cst_item *word)
{
    const cst_item *p,*fs;
    int c;

    fs = path_to_item(word,"R:"PHRASE".P.dn");
    for (c=0, p=word; p && (!item_equal(p,fs)) && (c < CST_CONST_INT_MAX); p=item_next(p))
    {
        if (cst_streq("content", ffeature_string(p,GPOS))) c++;
    }
    return val_string_n(c);  /* its used randomly as int and float */
}

/* 36 */
static const cst_val *lisp_distance_to_p_content(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p=item_prev(item_as(syl,PHRASE));p;p=item_prev(p))
    {
        c++;
        if (gpos(p)==&val_string_content)
            break;
    }
    return val_string_n(c);
}

/* 37 */
static const cst_val *lisp_distance_to_n_content(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p=item_next(item_as(syl,PHRASE));p;p=item_next(p))
    {
        c++;
        if (gpos(p)==&val_string_content)
            break;
    }
    return val_string_n(c);
}

/* 38 39 40 59 60 by Toda-san */
static const cst_val *lisp_num_syls_in_phrase(const cst_item *phrase)
{
    const cst_item *sw,*fw;
    int c;
  
    sw = path_to_item(phrase,"d1");
    fw = path_to_item(phrase,"dn");
    for (c=0; sw && (!item_equal(sw,fw)) && (c < CST_CONST_INT_MAX); sw=item_next(sw))
    {
        c += ffeature_int(sw, WORD_NUMSYLS);
    }
    c += ffeature_int(sw, WORD_NUMSYLS);
    return val_string_n(c);
}

/* 41 42 43 61 62 by Toda-san */
static const cst_val *lisp_num_words_in_phrase(const cst_item *phrase)
{
    const cst_item *sw,*fw;
    int c;
  
    sw = path_to_item(phrase,"d1");
    fw = path_to_item(phrase,"dn");
    for (c=1; sw && (!item_equal(sw,fw)) && (c < CST_CONST_INT_MAX); sw=item_next(sw))
    {
        c++;
    }
    return val_string_n(c);
}

/* 46 by Toda-san */
static const cst_val *lisp_total_syls(const cst_item *phrase)
{
    const cst_item *sp, *fp;
    int c;

    sp = phrase;
    while (item_prev(sp) != NULL) sp = item_prev(sp);
    fp = phrase;
    while (item_next(fp) != NULL) fp = item_next(fp);
    for (c = 0; sp && (!item_equal(sp, fp)) && (c < CST_CONST_INT_MAX); sp = item_next(sp))
    {
        c += ffeature_int(sp, LISP_NUM_SYLS_IN_PHRASE);
    }
    c += ffeature_int(sp, LISP_NUM_SYLS_IN_PHRASE);
    return val_string_n(c);
}

/* 47 by Toda-san */
static const cst_val *lisp_total_words(const cst_item *phrase)
{
    const cst_item *sp, *fp;
    int c;
  
    sp = phrase;
    while (item_prev(sp) != NULL) sp = item_prev(sp);
    fp = phrase;
    while (item_next(fp) != NULL) fp = item_next(fp);
    for (c = 0; sp && (!item_equal(sp, fp)) && (c < CST_CONST_INT_MAX); sp = item_next(sp))
    {
        c += ffeature_int(sp, LISP_NUM_WORDS_IN_PHRASE);
    }
    c += ffeature_int(sp, LISP_NUM_WORDS_IN_PHRASE);
    return val_string_n(c);
}

/* 48 by Toda-san */
static const cst_val *lisp_total_phrases(const cst_item *phrase)
{
    const cst_item *sp, *fp;
    int c;

    sp = phrase;
    while (item_prev(sp) != NULL) sp = item_prev(sp);
    fp = phrase;
    while (item_next(fp) != NULL) fp = item_next(fp);
    for (c = 1; sp && (!item_equal(sp, fp)) && (c < CST_CONST_INT_MAX); sp = item_next(sp))
    {
        c++;
    }
    return val_string_n(c);
}

void us_ff_register_hts(cst_ffunction *ffunctions)
{
    us_ff_register(ffunctions);

    ff_register(ffunctions, LISP_DISTANCE_TO_P_STRESS,lisp_distance_to_p_stress); /* 21 */
    ff_register(ffunctions, LISP_DISTANCE_TO_N_STRESS,lisp_distance_to_n_stress); /* 22 */
    ff_register(ffunctions, LISP_DISTANCE_TO_P_ACCENT,lisp_distance_to_p_accent); /* 23 */
    ff_register(ffunctions, LISP_DISTANCE_TO_N_ACCENT,lisp_distance_to_n_accent); /* 24 */
    ff_register(ffunctions, WORDS_OUT,words_out); /* 33 */
    ff_register(ffunctions, HTS_CONTENT_WORDS_IN,hts_content_words_in); /* 34 */
    ff_register(ffunctions, HTS_CONTENT_WORDS_OUT,hts_content_words_out); /* 35 */
    ff_register(ffunctions, LISP_DISTANCE_TO_P_CONTENT,lisp_distance_to_p_content); /* 36 */
    ff_register(ffunctions, LISP_DISTANCE_TO_N_CONTENT,lisp_distance_to_n_content); /* 37 */
    ff_register(ffunctions, LISP_NUM_SYLS_IN_PHRASE,lisp_num_syls_in_phrase); /* 38 39 40 59 60 */
    ff_register(ffunctions, LISP_NUM_WORDS_IN_PHRASE,lisp_num_words_in_phrase); /* 41 42 43 61 62 */
    ff_register(ffunctions, LISP_TOTAL_SYLS,lisp_total_syls); /* 46 */
    ff_register(ffunctions, LISP_TOTAL_WORDS,lisp_total_words); /* 47 */
    ff_register(ffunctions, LISP_TOTAL_PHRASES,lisp_total_phrases); /* 48 */
}
