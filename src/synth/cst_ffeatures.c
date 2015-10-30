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
/*                         Copyright (c) 2007                            */
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
/*               Date:  November 2007                                    */
/*************************************************************************/
/*                                                                       */
/*  Some language independent features                                   */
/*                                                                       */

#include "cst_phoneset.h"
#include "cst_regex.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"

static const cst_val *word_break(const cst_item *word);
static const cst_val *word_punc(const cst_item *word);
static const cst_val *word_numsyls(const cst_item *word);
static const cst_val *ssyl_in(const cst_item *syl);
static const cst_val *syl_in(const cst_item *syl);
static const cst_val *syl_out(const cst_item *syl);
static const cst_val *syl_break(const cst_item *syl);
static const cst_val *syl_codasize(const cst_item *syl);
static const cst_val *syl_onsetsize(const cst_item *syl);

const cst_val *accented(const cst_item *p);

DEF_STATIC_CONST_VAL_STRING(val_string_onset,"onset");
DEF_STATIC_CONST_VAL_STRING(val_string_coda,"coda");
DEF_STATIC_CONST_VAL_STRING(val_string_initial,"initial");
DEF_STATIC_CONST_VAL_STRING(val_string_single,"single");
DEF_STATIC_CONST_VAL_STRING(val_string_final,"final");
DEF_STATIC_CONST_VAL_STRING(val_string_mid,"mid");
DEF_STATIC_CONST_VAL_STRING(val_string_empty,"");

static const cst_val *ph_vc(const cst_item *p)
{
    return phone_feature(item_phoneset(p),ITEM_NAME(p),"vc");
}
static const cst_val *ph_vlng(const cst_item *p)
{
    return phone_feature(item_phoneset(p),ITEM_NAME(p),"vlng");
}
static const cst_val *ph_vheight(const cst_item *p)
{
   return phone_feature(item_phoneset(p),ITEM_NAME(p),"vheight");
}
static const cst_val *ph_vrnd(const cst_item *p)
{
    return phone_feature(item_phoneset(p),ITEM_NAME(p),"vrnd");
}
static const cst_val *ph_vfront(const cst_item *p)
{
    return phone_feature(item_phoneset(p),ITEM_NAME(p),"vfront");
}
static const cst_val *ph_ctype(const cst_item *p)
{
    return phone_feature(item_phoneset(p),ITEM_NAME(p),"ctype");
}
static const cst_val *ph_cplace(const cst_item *p)
{
    return phone_feature(item_phoneset(p),ITEM_NAME(p),"cplace");
}
static const cst_val *ph_cvox(const cst_item *p)
{
    return phone_feature(item_phoneset(p),ITEM_NAME(p),"cvox");
}

static const cst_val *cg_duration(const cst_item *p)
{
    /* Note this constructs float vals, these will be freed when the */
    /* cart cache is freed, so this should only be used in carts     */
    if (!p)
        return float_val(0.0);
    else if (!item_prev(p))
        return item_feat(p,"end");
    else
        return float_val(item_feat_float(p,"end")
			 - item_feat_float(item_prev(p),"end"));
}

DEF_STATIC_CONST_VAL_STRING(val_string_pos_b,"b");
DEF_STATIC_CONST_VAL_STRING(val_string_pos_m,"m");
DEF_STATIC_CONST_VAL_STRING(val_string_pos_e,"e");

static const cst_val *cg_state_pos(const cst_item *p)
{
    const char *name;
    name = item_feat_string(p,"name");
    if (!cst_streq(name,ffeature_string(p,"p.name")))
        return &val_string_pos_b;
    if (cst_streq(name,ffeature_string(p,"n.name")))
        return &val_string_pos_m;
    else
        return &val_string_pos_e;
}

static const cst_val *cg_state_place(const cst_item *p)
{
    float start, end;
    int this;
    start = (float)ffeature_int(p,"R:"MCEP_LINK".P.d1.frame_number");
    end = (float)ffeature_int(p,"R:"MCEP_LINK".P.dn.frame_number");
    this = item_feat_int(p,"frame_number");
    if ((end-start) == 0.0)
        return float_val(0.0);
    else
        return float_val((this-start)/(end-start));
}

static const cst_val *cg_state_index(const cst_item *p)
{
    float start;
    int this;
    start = (float)ffeature_int(p,"R:"MCEP_LINK".P.d1.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(this-start);
}

static const cst_val *cg_state_rindex(const cst_item *p)
{
    float end;
    int this;
    end = (float)ffeature_int(p,"R:"MCEP_LINK".P.dn.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(end-this);
}

static const cst_val *cg_phone_place(const cst_item *p)
{
    float start, end;
    int this;
    start = (float)ffeature_int(p,"R:"MCEP_LINK".P.R:"SEGSTATE".P.d1.R:"MCEP_LINK".d1.frame_number");
    end = (float)ffeature_int(p,"R:"MCEP_LINK".P.R:"SEGSTATE".P.dn.R:"MCEP_LINK".dn.frame_number");
    this = item_feat_int(p,"frame_number");
    if ((end-start) == 0.0)
        return float_val(0.0);
    else
        return float_val((this-start)/(end-start));
}

static const cst_val *cg_phone_index(const cst_item *p)
{
    float start;
    int this;
    start = (float)ffeature_int(p,"R:"MCEP_LINK".P.R:"SEGSTATE".P.d1.R:"MCEP_LINK".d1.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(this-start);
}

static const cst_val *cg_phone_rindex(const cst_item *p)
{
    float end;
    int this;
    end = (float)ffeature_int(p,"R:"MCEP_LINK".P.R:"SEGSTATE".P.dn.R:"MCEP_LINK".dn.frame_number");
    this = item_feat_int(p,"frame_number");
    return float_val(end-this);
}

static const cst_val *cg_is_pau(const cst_item *p)
{
    if (p && cst_streq("pau",item_feat_string(p,"name")))
        return &val_int_1;
    else
        return &val_int_0;
}

static const cst_val *cg_find_phrase_number(const cst_item *p)
{
    const cst_item *v;
    int x = 0;

    for (v=item_prev(p); v; v=item_prev(v))
        x++;

    return val_int_n(x);
}

static const cst_val *cg_position_in_phrasep(const cst_item *p)
{
    float pstart, pend, phrasenumber;
    float x;
    #define CG_FRAME_SHIFT 0.005 

    pstart = ffeature_float(p,"R:"MCEP_LINK".P.R:"SEGSTATE".P.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.d1.R:"SYLSTRUCTURE".d1.d1.R:"SEGMENT".p.end");
    pend = ffeature_float(p,"R:"MCEP_LINK".P.R:"SEGSTATE".P.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn.dn.R:"SEGMENT".end");
    phrasenumber = ffeature_float(p,"R:"MCEP_LINK".P.R:"SEGSTATE".P.R:"SYLSTRUCTURE".P.P.R:"PHRASE".P."LISP_CG_FIND_PHRASE_NUMBER);
    if ((pend - pstart) == 0.0)
        return float_val(-1.0);
    else
    {
        x = phrasenumber +
            ((CG_FRAME_SHIFT*item_feat_float(p,"frame_number"))-pstart) /
            (pend - pstart);
        return float_val(x);
    }
}

/* Spam specific features, but may be useful for others */
static const cst_val *pos_in_word(const cst_item *p)
{
	const cst_item *s;
	int i=0;
	p=item_as(p,SYLLABLE);
	s=item_as(path_to_item(p,"R:"SYLSTRUCTURE".P.d1"),SYLLABLE);
	for (;s && !item_equal(p,s);s=item_next(s),i++){}
	return val_string_n(i);
}

static const cst_val *syllable_duration(const cst_item *p)
{
	return float_val(ffeature_float(p,"R:"SYLSTRUCTURE".dn.R:"SEGMENT".end") - ffeature_float(p,"R:"SYLSTRUCTURE".d1.R:"SEGMENT".p.end"));
}

static const cst_val *syl_vowel(const cst_item *p)
{
    /* Do not call this ffeature with ffeature_string as it will leak a   */
    /* temporary cst_val return value. Call ffeature explicitly instead   */
	const cst_item *s,*ls;
	s=item_as(path_to_item(p,"R:"SYLSTRUCTURE".d1"),SEGMENT);
	ls=item_as(path_to_item(p,"R:"SYLSTRUCTURE".dn"),SEGMENT);
	for (;s && !item_equal(s,ls);s=item_next(s))
	{
		if (cst_streq("+",val_string(ph_vc(s)))){ return string_val(ITEM_NAME(s));}
	}
	if (cst_streq("+",val_string(ph_vc(s)))){ return string_val(ITEM_NAME(s));}
	return (cst_val *) NULL;
}

static const cst_val *syl_numphones(const cst_item *p)
{
	int i;
	const cst_item *s,*ls;
	s=item_as(path_to_item(p,"R:"SYLSTRUCTURE".d1"),SEGMENT);
	ls=item_as(path_to_item(p,"R:"SYLSTRUCTURE".dn"),SEGMENT);
	for (i=1;s && !item_equal(s,ls);s=item_next(s)){i++;}
	return val_string_n(i);
}


static const cst_val *pos_in_phrase(const cst_item *p)
{
	const cst_item *s;
	int i=0;
	p=item_as(p,WORD);
	s=item_as(path_to_item(p,"R:"SYLSTRUCTURE".R:"PHRASE".P.d1"),WORD);
	for (;s && !item_equal(p,s);s=item_next(s),i++){}
	return val_string_n(i);
}

static const cst_val *cg_syl_ratio(const cst_item *p)
{
	return float_val (( 1 + ffeature_float(p,SYL_IN))/(1 + ffeature_float(p,SYL_IN) + ffeature_float(p,SYL_OUT)));
}


static const cst_val *cg_phrase_ratio(const cst_item *p)
{
	const cst_item *lp=p;
	while (item_next(lp)){lp=item_next(lp);}
	return float_val ((1 + ffeature_float(p,LISP_CG_FIND_PHRASE_NUMBER))/(1 + ffeature_float(lp,LISP_CG_FIND_PHRASE_NUMBER)));
}

static const cst_val *cg_syls_in_phrase(const cst_item *p)
{
	cst_item *s=item_as(item_daughter(p),WORD);
	return float_val(1 + ffeature_float(s,"R:"SYLSTRUCTURE".d1.R:"SYLLABLE"."SYL_OUT));
}


const cst_val *accented(const cst_item *syl)
{
    if ((item_feat_present(syl,"accent")) ||
	(item_feat_present(syl,"endtone")))
	return &val_string_1;
    else
	return &val_string_0;
}

static const cst_val *seg_coda_ctype(const cst_item *seg, const char *ctype)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);
    
    for (s=item_last_daughter(item_parent(item_as(seg,SYLSTRUCTURE)));
	 s;
	 s=item_prev(s))
    {
	if (cst_streq("+",phone_feature_string(ps,item_feat_string(s,"name"),
					       "vc")))
	    return &val_string_0;
	if (cst_streq(ctype,phone_feature_string(ps,item_feat_string(s,"name"),
					       "ctype")))
	    return &val_string_1;
    }

    return &val_string_0;
}

static const cst_val *seg_onset_ctype(const cst_item *seg, const char *ctype)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);
    
    for (s=item_daughter(item_parent(item_as(seg,SYLSTRUCTURE)));
	 s;
	 s=item_next(s))
    {
	if (cst_streq("+",phone_feature_string(ps,item_feat_string(s,"name"),
					       "vc")))
	    return &val_string_0;
	if (cst_streq(ctype,phone_feature_string(ps,item_feat_string(s,"name"),
						 "ctype")))
	    return &val_string_1;
    }

    return &val_string_0;
}

static const cst_val *seg_coda_fric(const cst_item *seg)
{
    return seg_coda_ctype(seg,"f");
}

static const cst_val *seg_onset_fric(const cst_item *seg)
{
    return seg_onset_ctype(seg,"f");
}

static const cst_val *seg_coda_stop(const cst_item *seg)
{
    return seg_coda_ctype(seg,"s");
}

static const cst_val *seg_onset_stop(const cst_item *seg)
{
    return seg_onset_ctype(seg,"s");
}

static const cst_val *seg_coda_nasal(const cst_item *seg)
{
    return seg_coda_ctype(seg,"n");
}

static const cst_val *seg_onset_nasal(const cst_item *seg)
{
    return seg_onset_ctype(seg,"n");
}

static const cst_val *seg_coda_glide(const cst_item *seg)
{
    if (seg_coda_ctype(seg,"r") == &val_string_0)
	    return seg_coda_ctype(seg,"l");
    return &val_string_1;
}

static const cst_val *seg_onset_glide(const cst_item *seg)
{
    if (seg_onset_ctype(seg,"r") == &val_string_0)
	    return seg_onset_ctype(seg,"l");
    return &val_string_1;
}

static const cst_val *seg_onsetcoda(const cst_item *seg)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);

    if (!seg) return &val_string_0;
    for (s=item_next(item_as(seg,SYLSTRUCTURE));
	 s;
	 s=item_next(s))
    {
	if (cst_streq("+",phone_feature_string(ps,item_feat_string(s,"name"),
					       "vc")))
	    return &val_string_onset;
    }
    return &val_string_coda;
}

static const cst_val *pos_in_syl(const cst_item *seg)
{
    const cst_item *s;
    int c;
    
    for (c=-1,s=item_as(seg,SYLSTRUCTURE);
	 s;
	 s=item_prev(s),c++);

    return val_string_n(c);
}

static const cst_val *position_type(const cst_item *syl)
{
    const cst_item *s = item_as(syl,SYLSTRUCTURE);

    if (s == 0)
	return &val_string_single;
    else if (item_next(s) == 0)
    {
	if (item_prev(s) == 0)
	    return &val_string_single;
	else
	    return &val_string_final;
    }
    else if (item_prev(s) == 0)
	return &val_string_initial;
    else
	return &val_string_mid;
}

static const cst_val *sub_phrases(const cst_item *syl)
{
    const cst_item *s;
    int c;
    
    for (c=0,s=path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.p");
	 s && (c < CST_CONST_INT_MAX); 
	 s=item_prev(s),c++);

    return val_string_n(c);
}

static const cst_val *last_accent(const cst_item *syl)
{
    const cst_item *s;
    int c;
    
    for (c=0,s=item_as(syl,SYLLABLE);
	 s && (c < CST_CONST_INT_MAX); 
	 s=item_prev(s),c++)
    {
	if (val_int(accented(s)))
	    return val_string_n(c);
    }

    return val_string_n(c);
}

static const cst_val *next_accent(const cst_item *syl)
{
    const cst_item *s;
    int c;

    s=item_as(syl,SYLLABLE);
    if (s)
        s = item_next(s);
    else
        return val_string_n(0);
    for (c=0;
	 s && (c < CST_CONST_INT_MAX); 
	 s=item_next(s),c++)
    {
	if (val_int(accented(s)))
	    return val_string_n(c);
    }

    return val_string_n(c);
}

static const cst_val *syl_final(const cst_item *seg)
{   /* last segment in a syllable */
    const cst_item *s = item_as(seg,SYLSTRUCTURE);

    if (!s || (item_next(s) == NULL))
	return &val_string_1;
    else
	return &val_string_0;
}

static const cst_val *word_punc(const cst_item *word)
{
    cst_item *ww;
    const cst_val *v;

    ww = item_as(word,TOKEN);

    if ((ww != NULL) && (item_next(ww) != 0))
	v = &val_string_empty;
    else
	v = ffeature(item_parent(ww),"punc");

/*    printf("word_punc word %s punc %s\n",
	   item_feat_string(ww,"name"),
	   val_string(v)); */

    return v;

}

static const cst_val *word_break(const cst_item *word)
{
    cst_item *ww,*pp;
    const char *pname;

    ww = item_as(word,PHRASE);

    if ((ww == NULL) || (item_next(ww) != 0))
	return &val_string_1;
    else
    {
	pp = item_parent(ww);
	pname = val_string(item_feat(pp,"name"));
	if (cst_streq("BB",pname))
	    return &val_string_4;
	else if (cst_streq("B",pname))
	    return &val_string_3;
	else 
	    return &val_string_1;
    }
}

static const cst_val *word_numsyls(const cst_item *word)
{
    cst_item *d;
    int c;
    
    for (c=0,d=item_daughter(item_as(word,SYLSTRUCTURE));
	 d;
	 d=item_next(d),c++);

    return val_int_n(c);
}

static const cst_val *ssyl_in(const cst_item *syl)
{
    /* Number of stressed syllables since last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,SYLLABLE);

    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.d1.R:"SYLSTRUCTURE".d1");

    /* fix by uratec */
    if (item_equal(ss,fs))
        return val_string_n(0);

    for (c=0, p=item_prev(ss); 
	 p && (!item_equal(p,fs)) && (c < CST_CONST_INT_MAX);
	 p=item_prev(p))
    {
	if (cst_streq("1",ffeature_string(p,"stress")))
	    c++;
    }
    
    return val_string_n(c);  /* its used randomly as int and float */
}

static const cst_val *ssyl_out(const cst_item *syl)
{
    /* Number of stressed syllables until last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,SYLLABLE);

    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn");

    /* fix by uratec */
    if (item_equal(ss,fs))
        return val_string_n(0);

    for (c=0, p=item_next(ss); 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_next(p))
    {
	if (cst_streq("1",ffeature_string(p,"stress")))
	    c++;
	if (item_equal(p,fs))
	    break;
    }
    
    return val_string_n(c);  /* its used randomly as int and float */
}

static const cst_val *syl_in(const cst_item *syl)
{
    /* Number of syllables since last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,SYLLABLE);

    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.d1.R:"SYLSTRUCTURE".d1");

    for (c=0, p=ss; 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_prev(p),c++)
	if (item_equal(p,fs))
	    break;
    return val_string_n(c);
}

static const cst_val *syl_out(const cst_item *syl)
{
    /* Number of syllables until next major break */
    cst_item *ss,*p;
    const cst_item *fs;
    int c;

    ss = item_as(syl,SYLLABLE);

    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn");

    for (c=0, p=ss; 
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_next(p),c++)
	if (item_equal(p,fs))
	    break;
    return val_string_n(c);
}

static const cst_val *syl_break(const cst_item *syl)
{
    /* Break level after this syllable */
    cst_item *ss;

    ss = item_as(syl,SYLSTRUCTURE);

    if (ss == NULL)
	return &val_string_1;  /* hmm, no sylstructure */
    else if (item_next(ss) != NULL)
	return &val_string_0;  /* word internal */
    else if (item_parent(ss) == NULL)  /* no parent */
	return &val_string_1;
    else
	return word_break(item_parent(ss));
}

static const cst_val *cg_break(const cst_item *syl)
{
    /* phrase prediction is so different between flite and festival */
    /* we go with this more robust technique */
    cst_item *ss;

    ss = item_as(syl,SYLSTRUCTURE);

    if (ss == NULL)
	return &val_string_0;  /* hmm, no sylstructure */
    else if (item_next(ss) != NULL)
	return &val_string_0;  /* word internal */
    else if (path_to_item(ss,"R:"SYLSTRUCTURE".P.R:"WORD".n") == NULL)
        return &val_string_4;  /* utterance final */
    else if (path_to_item(ss,"R:"SYLSTRUCTURE".P.R:"PHRASE".n") == NULL)
        return &val_string_3;  /* phrase final */
    else
	return &val_string_1;  /* word final */
}

static const cst_val *syl_codasize(const cst_item *syl)
{
    cst_item *d;
    int c;

    for (c=1,d=item_last_daughter(item_as(syl,SYLSTRUCTURE));
	 d;
	 d=item_prev(d),c++)
    {
	if (cst_streq("+",val_string(ph_vc(d))))
	    break;
    }

    return val_string_n(c);
}

static const cst_val *syl_onsetsize(const cst_item *syl)
{
    cst_item *d;
    int c;
    
    for (c=0,d=item_daughter(item_as(syl,SYLSTRUCTURE));
	 d;
	 d=item_next(d),c++)
    {
	if (cst_streq("+",val_string(ph_vc(d))))
	    break;
    }

    return val_string_n(c);
}

static const cst_val *asyl_in(const cst_item *syl)
{
    /* Number of accented syllables since last major break */
    const cst_item *ss,*p,*fs;
    int c;

    ss = item_as(syl,SYLLABLE);

    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.d1.R:"SYLSTRUCTURE".d1");

    /* fix by uratec */
    if (item_equal(ss,fs))
        return val_string_n(0);
    for (c=0, p=item_prev(ss);
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_prev(p))
    {
	if (val_int(accented(p)) == 1)
	    c++;
	if (item_equal(p,fs))
	    break;
    }
    
    return val_string_n(c);
}

static const cst_val *asyl_out(const cst_item *syl)
{
    /* Number of accented syllables until next major break */
    cst_item *ss,*p;
    const cst_item *fs;
    int c;

    ss = item_as(syl,SYLLABLE);

    fs = path_to_item(syl,"R:"SYLSTRUCTURE".P.R:"PHRASE".P.dn.R:"SYLSTRUCTURE".dn");

    /* fix by uratec */
    if (item_equal(ss,fs))
        return val_string_n(0);
    for (c=0, p=item_next(ss);
	 p && (c < CST_CONST_INT_MAX); 
	 p=item_next(p))
    {
	if (val_int(accented(p)) == 1)
	    c++;
	if (item_equal(p,fs))
	    break;
    }
    return val_string_n(c);
}

static const cst_val *segment_duration(const cst_item *seg)
{
    const cst_item *s = item_as(seg,SEGMENT);

    if (!s)
	return &val_string_0;
    else if (item_prev(s) == NULL)
	return item_feat(s,"end");
    else
    {
	/* It should be okay to construct this as it will get
           dereferenced when the CART interpreter frees its feature
           cache. */
	return float_val(item_feat_float(s,"end")
			 - item_feat_float(item_prev(s),"end"));
    }
}


void basic_ff_register(cst_ffunction *ffunctions)
{
    ff_register(ffunctions, PH_VC,ph_vc);
    ff_register(ffunctions, PH_VLNG,ph_vlng);
    ff_register(ffunctions, PH_VHEIGHT,ph_vheight);
    ff_register(ffunctions, PH_VFRONT,ph_vfront);
    ff_register(ffunctions, PH_VRND,ph_vrnd);
    ff_register(ffunctions, PH_CTYPE,ph_ctype);
    ff_register(ffunctions, PH_CPLACE,ph_cplace);
    ff_register(ffunctions, PH_CVOX,ph_cvox);

    ff_register(ffunctions, LISP_CG_DURATION, cg_duration);
    ff_register(ffunctions, LISP_CG_STATE_POS, cg_state_pos);
    ff_register(ffunctions, LISP_CG_STATE_PLACE, cg_state_place);
    ff_register(ffunctions, LISP_CG_STATE_INDEX, cg_state_index);
    ff_register(ffunctions, LISP_CG_STATE_RINDEX, cg_state_rindex);
    ff_register(ffunctions, LISP_CG_PHONE_PLACE, cg_phone_place);
    ff_register(ffunctions, LISP_CG_PHONE_INDEX, cg_phone_index);
    ff_register(ffunctions, LISP_CG_PHONE_RINDEX, cg_phone_rindex);

    ff_register(ffunctions, LISP_CG_POSITION_IN_PHRASEP, cg_position_in_phrasep);
    ff_register(ffunctions, LISP_CG_FIND_PHRASE_NUMBER, cg_find_phrase_number);
    ff_register(ffunctions, LISP_IS_PAU, cg_is_pau);

    ff_register(ffunctions, WORD_NUMSYLS,word_numsyls);
    ff_register(ffunctions, WORD_BREAK,word_break);
    ff_register(ffunctions, WORD_PUNC,word_punc);
    ff_register(ffunctions, SSYL_IN,ssyl_in);
    ff_register(ffunctions, SSYL_OUT,ssyl_out);
    ff_register(ffunctions, SYL_IN,syl_in);
    ff_register(ffunctions, SYL_OUT,syl_out);
    ff_register(ffunctions, SYL_BREAK,syl_break);
    ff_register(ffunctions, LISP_CG_BREAK,cg_break);
    ff_register(ffunctions, OLD_SYL_BREAK,syl_break);
    ff_register(ffunctions, SYL_ONSETSIZE,syl_onsetsize);
    ff_register(ffunctions, SYL_CODASIZE,syl_codasize);
    ff_register(ffunctions, ACCENTED,accented);
    ff_register(ffunctions, ASYL_IN,asyl_in);
    ff_register(ffunctions, ASYL_OUT,asyl_out);
    ff_register(ffunctions, LISP_CODA_FRIC,seg_coda_fric);
    ff_register(ffunctions, LISP_ONSET_FRIC,seg_onset_fric);
    ff_register(ffunctions, LISP_CODA_STOP,seg_coda_stop);
    ff_register(ffunctions, LISP_ONSET_STOP,seg_onset_stop);
    ff_register(ffunctions, LISP_CODA_NASAL,seg_coda_nasal);
    ff_register(ffunctions, LISP_ONSET_NASAL,seg_onset_nasal);
    ff_register(ffunctions, LISP_CODA_GLIDE,seg_coda_glide);
    ff_register(ffunctions, LISP_ONSET_GLIDE,seg_onset_glide);
    ff_register(ffunctions, SEG_ONSETCODA,seg_onsetcoda);
    ff_register(ffunctions, POS_IN_SYL,pos_in_syl);
    ff_register(ffunctions, POSITION_TYPE,position_type);
    ff_register(ffunctions, SUB_PHRASES,sub_phrases);
    ff_register(ffunctions, LAST_ACCENT,last_accent);
    ff_register(ffunctions, NEXT_ACCENT,next_accent);
    ff_register(ffunctions, SYL_FINAL,syl_final);
    ff_register(ffunctions, SEGMENT_DURATION,segment_duration);
    ff_register(ffunctions, LISP_CG_SYL_RATIO,cg_syl_ratio);
    ff_register(ffunctions, LISP_CG_PHRASE_RATIO,cg_phrase_ratio);
    ff_register(ffunctions, LISP_CG_SYLS_IN_PHRASE,cg_syls_in_phrase);
    ff_register(ffunctions, POS_IN_PHRASE,pos_in_phrase);
    ff_register(ffunctions, POS_IN_WORD,pos_in_word);
    ff_register(ffunctions, SYLLABLE_DURATION,syllable_duration);
    ff_register(ffunctions, SYL_VOWEL,syl_vowel);
    ff_register(ffunctions, SYL_NUMPHONES,syl_numphones);
}
