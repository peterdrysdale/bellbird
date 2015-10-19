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
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*    Utterances                                                         */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_utterance.h"

/* utterance functions are the modules that do the meat in synthesis */
CST_VAL_REGISTER_FUNCPTR(uttfunc,cst_uttfunc)

cst_utterance *new_utterance()
{
    cst_utterance *u;

    u = cst_alloc(struct cst_utterance_struct,1);

    u->features = new_features();
    u->relations = new_features();

    return u;
}

void delete_utterance(cst_utterance *u)
{
    cst_featvalpair *fp;
    if (u)
    {
	delete_features(u->features);
	/* Relation vals don't delete their contents */
	for (fp=u->relations->head; fp; fp=fp->next)
	    delete_relation(val_relation(fp->val)); 
	delete_features(u->relations);
	cst_free(u);
    }
}

static int utt_relation_delete(cst_utterance *u,const char *name)
{
    /* Relation vals don't delete their contents */
    if (feat_present(u->relations, name))
	delete_relation(val_relation(feat_val(u->relations,name)));
    return feat_remove(u->relations,name);
}

cst_relation *utt_relation_create(cst_utterance *u,const char *name)
{
    cst_relation *r;
    
    utt_relation_delete(u,name); /* remove if already there */
    r = new_relation(name,u);
    feat_set(u->relations,name,relation_val(r));
    return r;
}

cst_relation *utt_relation(const cst_utterance *u,const char *name)
{
    const cst_val *v = feat_val(u->relations,name);
    if (v != NULL)
	return val_relation(v);
    else
    {
	cst_errmsg("Relation: %s not present in utterance\n",
		   name);
	cst_error();
    }
    return NULL;
}
