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
/*                         Copyright (c) 2000                            */
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
/*  General synthesis control                                            */
/*                                                                       */
/*************************************************************************/

#include "cst_cart.h"
#include "cst_error.h"
#include "cst_lexicon.h"
#include "cst_phoneset.h"
#include "cst_synth.h"
#include "cst_tokenstream.h"
#include "cst_utt_utils.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"

CST_VAL_REGISTER_FUNCPTR(breakfunc,cst_breakfunc)

#ifndef SYNTH_MODULES_DEBUG
#define SYNTH_MODULES_DEBUG 0
#endif

#if SYNTH_MODULES_DEBUG > 0
#define DPRINTF(l,x) if (SYNTH_MODULES_DEBUG > l) cst_errmsg x
#else
#define DPRINTF(l,x)
#endif

typedef struct cst_synth_module_struct {
    const char *hookname;
    cst_uttfunc defhook;
} cst_synth_module;

static const cst_synth_module synth_method_tokens[];
static const cst_synth_module synth_module_tokenizer;

cst_utterance *utt_synth_wave(cst_wave *w,cst_voice *v)
{
    /* Create an utterance with a wave in it as if we've synthesized it */
    cst_utterance *u;

    u = new_utterance();
    utt_init(u,v);
    utt_set_wave(u,w);

    return u;
}

static cst_utterance *apply_synth_module(cst_utterance *u,
				  const cst_synth_module *mod)
{
    const cst_val *v;

    v = feat_val(u->features, mod->hookname);
    if (v)
	return (*val_uttfunc(v))(u);
    if (mod->defhook)
	return (*mod->defhook)(u);
    return u;
}

static cst_utterance *apply_synth_method(cst_utterance *u,
				  const cst_synth_module meth[])
{
    while (meth->hookname)
    {
	if ((u = apply_synth_module(u, meth)) == NULL)
	    return NULL;
	++meth;
    }

    return u;
}

cst_utterance *utt_init(cst_utterance *u, cst_voice *vox)
{
    feat_copy_into(vox->features,u->features);
    u->ffunctions = vox->ffunctions;
    if (vox->utt_init)
	vox->utt_init(u, vox);

    return u;
}

cst_utterance *utt_synth(cst_utterance *u)
{
// utt_synth is a tokenizer module followed by the usual utt_synth_tokens
// synthesis method
    if ((u=apply_synth_module(u,&synth_module_tokenizer)) == NULL)
       return NULL;
    return apply_synth_method(u, synth_method_tokens);
}

cst_utterance *utt_synth_tokens(cst_utterance *u)
{
    return apply_synth_method(u, synth_method_tokens);
}

static cst_utterance *default_tokenization(cst_utterance *u)
{
    const char *text,*token;
    cst_tokenstream *fd;
    cst_item *t;
    cst_relation *r;

    text = utt_input_text(u);
    r = utt_relation_create(u,TOKEN);
    fd = ts_open_string(text,
	get_param_string(u->features,"text_whitespace",NULL),
	get_param_string(u->features,"text_singlecharsymbols",NULL),
	get_param_string(u->features,"text_prepunctuation",NULL),
        get_param_string(u->features,"text_postpunctuation",NULL));
    
    while (!ts_eof(fd))
    {
	token = ts_get(fd);
	if (cst_strlen(token) > 0)
	{
	    t = relation_append(r,NULL);
	    item_set_string(t,"name",token);
	    item_set_string(t,"whitespace",fd->whitespace);
	    item_set_string(t,"prepunctuation",fd->prepunctuation);
	    item_set_string(t,"punc",fd->postpunctuation);
	    item_set_int(t,"file_pos",fd->file_pos);
	    item_set_int(t,"line_number",fd->line_number);
	}
    }

    ts_close(fd);
    
    return u;
}

static cst_val *default_tokentowords(cst_item *i)
{
    return cons_val(string_val(item_feat_string(i,"name")), NULL);
}

static cst_utterance *default_textanalysis(cst_utterance *u)
{
    cst_item *t,*word;
    cst_relation *word_rel;
    cst_val *words;
    const cst_val *w;
    const cst_val *ttwv;

    word_rel = utt_relation_create(u,WORD);
    ttwv = feat_val(u->features, "tokentowords_func");

    for (t=relation_head(utt_relation(u,TOKEN)); t; t=item_next(t))
    {
	if (ttwv)
	    words = (cst_val *)(*val_itemfunc(ttwv))(t);
	else
	    words = default_tokentowords(t);

	for (w=words; w; w=val_cdr(w))
	{
	    word = item_add_daughter(t,NULL);
	    if (cst_val_consp(val_car(w)))
	    {   /* Has extra features */
		item_set_string(word,"name",val_string(val_car(val_car(w))));
		feat_copy_into(val_features(val_cdr(val_car(w))),
			       item_feats(word));
	    }
	    else
		item_set_string(word,"name",val_string(val_car(w)));
	    relation_append(word_rel,word);
	}
	delete_val(words);
    }

    return u;
}

cst_utterance *default_phrasing(cst_utterance *u)
{
    cst_relation *r;
    cst_item *w, *p, *lp=NULL;
    const cst_val *v;
    cst_cart *phrasing_cart;

    r = utt_relation_create(u,PHRASE);
    phrasing_cart = val_cart(feat_val(u->features,"phrasing_cart"));

    for (p=NULL,w=relation_head(utt_relation(u,WORD)); w; w=item_next(w))
    {
	if (p == NULL)
	{
	    p = relation_append(r,NULL);
            lp = p;
            item_set_string(p,"name","B");
	}
	item_add_daughter(p,w);
	v = cart_interpret(w,phrasing_cart);
	if (cst_streq(val_string(v),"BB"))
	    p = NULL;
    }

    if (lp && item_prev(lp)) /* follow festival */
        item_set_string(lp,"name","BB");
    
    return u;
}

cst_utterance *hts_phrasing(cst_utterance *u)
{
    cst_relation *r;
    cst_item *w, *p, *lp=NULL;
    const cst_val *v;
    cst_cart *phrasing_cart;

    r = utt_relation_create(u,PHRASE);
    phrasing_cart = val_cart(feat_val(u->features,"phrasing_cart"));

    for (p=NULL,w=relation_head(utt_relation(u,WORD)); w; w=item_next(w))
    {
	if (p == NULL)
	{
	    p = relation_append(r,NULL);
            lp = p;
            item_set_string(p,"name","BB");
	}
	item_add_daughter(p,w);
	v = cart_interpret(w,phrasing_cart);
	if (cst_streq(val_string(v),"BB"))
	    p = NULL;
    }

    if (lp && item_prev(lp)) /* follow festival */
        item_set_string(lp,"name","BB");
    
    return u;
}

static cst_utterance *default_pause_insertion(cst_utterance *u)
{
    /* Add initial silences and silence at each phrase break */
    const char *silence;
    const cst_item *w;
    cst_item *p, *s;

    silence = val_string(feat_val(u->features,"silence"));

    /* Insert initial silence */
    s = relation_head(utt_relation(u,SEGMENT));
    if (s == NULL)
	s = relation_append(utt_relation(u,SEGMENT),NULL);
    else
	s = item_prepend(s,NULL);
    item_set_string(s,"name",silence);

    for (p=relation_head(utt_relation(u,PHRASE)); p; p=item_next(p))
    {
	for (w = item_last_daughter(p); w; w=item_prev(w))
	{
	    s = path_to_item(w,"R:"SYLSTRUCTURE".dn.dn.R:"SEGMENT);
	    if (s)
	    {
		s = item_append(s,NULL);
		item_set_string(s,"name",silence);
		break;
	    }
	}
    }

    return u;
}

cst_utterance *cart_intonation(cst_utterance *u)
{
    cst_cart *accents, *tones;
    cst_item *s;
    const cst_val *v;

    if (feat_present(u->features,"no_intonation_accent_model"))
        return u;  /* not all languages have intonation models */

    accents = val_cart(feat_val(u->features,"int_cart_accents"));
    tones = val_cart(feat_val(u->features,"int_cart_tones"));
    
    for (s=relation_head(utt_relation(u,SYLLABLE)); s; s=item_next(s))
    {
	v = cart_interpret(s,accents);
	if (!cst_streq("NONE",val_string(v)))
	    item_set_string(s,"accent",val_string(v));
	v = cart_interpret(s,tones);
	if (!cst_streq("NONE",val_string(v)))
	    item_set_string(s,"endtone",val_string(v));
	DPRINTF(0,("word %s gpos %s stress %s ssyl_in %s ssyl_out %s accent %s endtone %s\n",
		   ffeature_string(s,"R:"SYLSTRUCTURE".P.name"),
		   ffeature_string(s,"R:"SYLSTRUCTURE".P."GPOS),
		   ffeature_string(s,"stress"),
		   ffeature_string(s,SSYL_IN),
		   ffeature_string(s,SSYL_OUT),
		   ffeature_string(s,"accent"),
		   ffeature_string(s,"endtone")));
    }

    return u;
}

static cst_utterance *default_pos_tagger(cst_utterance *u)
{
    cst_item *word;
    const cst_val *p;
    const cst_cart *tagger;

    p = get_param_val(u->features,"pos_tagger_cart",NULL);
    if (p == NULL)
        return u;
    tagger = val_cart(p);

    for (word=relation_head(utt_relation(u,WORD));
	 word; word=item_next(word))
    {
        p = cart_interpret(word,tagger);
        item_set_string(word,"pos",val_string(p));
    }

    return u;
}

static cst_utterance *default_lexical_insertion(cst_utterance *u)
{
    cst_item *word;
    cst_relation *sylstructure,*seg,*syl;
    cst_lexicon *lex;
    const cst_val *lex_addenda = NULL;
    const cst_val *p, *wp = NULL;
    char *phone_name;
    const char *stress = "0";
    const char *pos;
    cst_val *phones;
    cst_item *ssword, *sssyl, *segitem, *sylitem, *seg_in_syl;
    const cst_val *vpn;
    int dp = 0;

    lex = val_lexicon(feat_val(u->features,"lexicon"));
    if (lex->lex_addenda)
	lex_addenda = lex->lex_addenda;

    syl = utt_relation_create(u,SYLLABLE);
    sylstructure = utt_relation_create(u,SYLSTRUCTURE);
    seg = utt_relation_create(u,SEGMENT);

    for (word=relation_head(utt_relation(u,WORD));
	 word; word=item_next(word))
    {
	ssword = relation_append(sylstructure,word);
        pos = ffeature_string(word,"pos");
	phones = NULL;
        wp = NULL;
        dp = 0;  /* should the phones get deleted or not */
        
        /*        printf("awb_debug word %s pos %s gpos %s\n",
               item_feat_string(word,"name"),
               pos,
               ffeature_string(word,GPOS)); */

	/* FIXME: need to make sure that textanalysis won't split
           tokens with explicit pronunciation (or that it will
           propagate such to words, then we can remove the path here) */
	if (item_feat_present(item_parent(item_as(word, TOKEN)), "phones"))
        {
            vpn = item_feat(item_parent(item_as(word, TOKEN)), "phones");
            dp = 1;
            if (cst_streq(val_string(vpn),
                          ffeature_string(word,"p.R:"TOKEN".P.phones")))
                phones = NULL; /* Already given these phones */
            else
                phones = val_readlist_string(val_string(vpn));
        }
	else
	{
            wp = get_entry_lex_addenda(item_feat_string(word, "name"),
                                       lex_addenda);
            if (wp)
                phones = get_phones_lex_addenda(wp);
            else
            {
                dp = 1;
		phones = lex_lookup(lex,item_feat_string(word,"name"),pos);
            }
	}

	for (sssyl=NULL,sylitem=NULL,p=phones; p; p=val_cdr(p))
	{
	    if (sylitem == NULL)
	    {
		sylitem = relation_append(syl,NULL);
		sssyl = item_add_daughter(ssword,sylitem);
		stress = "0";
	    }
	    segitem = relation_append(seg,NULL);
	    phone_name = cst_strdup(val_string(val_car(p)));
	    if (phone_name[cst_strlen(phone_name)-1] == '1')
	    {
		stress = "1";
		phone_name[cst_strlen(phone_name)-1] = '\0';
	    }
	    else if (phone_name[cst_strlen(phone_name)-1] == '0')
	    {
		stress = "0";
		phone_name[cst_strlen(phone_name)-1] = '\0';
	    }
	    item_set_string(segitem,"name",phone_name);
	    seg_in_syl = item_add_daughter(sssyl,segitem);
#if 0
            printf("awb_debug ph %s\n",phone_name);
#endif
	    if ((lex->syl_boundary)(seg_in_syl,val_cdr(p)))
	    {
#if 0
                printf("awb_debug SYL\n");
#endif
		sylitem = NULL;
		if (sssyl)
		    item_set_string(sssyl,"stress",stress);
	    }
	    cst_free(phone_name);
	}
	if (dp)
        {
	    delete_val(phones);
            phones = NULL;
        }
    }

    return u;
}

int default_utt_break(cst_tokenstream *ts,
		      const char *token,
		      cst_relation *tokens)
{
    /* This is the default utt break functions, languages may override this */
    /* This will be ok for some latin based languages */
    const char *postpunct = item_feat_string(relation_tail(tokens), "punc");
    const char *ltoken = ITEM_NAME(relation_tail(tokens));

    if (strchr(ts->whitespace,'\n') != strrchr(ts->whitespace,'\n'))
	 /* contains two new lines */
	 return TRUE;
    else if (strchr(postpunct,':') ||
	     strchr(postpunct,'?') ||
	     strchr(postpunct,'!'))
	return TRUE;
    else if (strchr(postpunct,'.') &&
	     (cst_strlen(ts->whitespace) > 1) &&
	     strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",token[0]))
	return TRUE;
    else if (strchr(postpunct,'.') &&
	     /* next word starts with a capital */
	     strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",token[0]) &&
	     /* last word isn't an abbreviation */
	     !(strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",ltoken[cst_strlen(ltoken)-1])||
	       ((cst_strlen(ltoken) < 4) &&
		strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",ltoken[0]))))
	return TRUE;
    else
	return FALSE;
}

static const cst_synth_module synth_module_tokenizer =
    { "tokenizer_func", default_tokenization };

static const cst_synth_module synth_method_tokens[] = {
    { "textanalysis_func", default_textanalysis },
    { "pos_tagger_func", default_pos_tagger },
    { "phrasing_func", NULL },
    { "lexical_insertion_func", default_lexical_insertion },
    { "pause_insertion_func", default_pause_insertion },
    { "intonation_func", cart_intonation },
    { "postlex_func", NULL },
    { "wave_synth_func", NULL },
    { "post_synth_hook_func", NULL },
    { NULL, NULL }
};
