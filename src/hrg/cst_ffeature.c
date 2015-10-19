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
/*                       Copyright (c) 1999-2007                         */
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
/*               Date:  April 2000                                       */
/*************************************************************************/
/*                                                                       */
/*  Item features and paths                                              */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_tokenstream.h"

#define TOKENSTRINGBUF 199 /* current max. length of tokenstring is approx. 142 chars */

DEF_STATIC_CONST_VAL_STRING(ffeature_default_val,"0");

static const void *internal_ff(const cst_item *item,
			       const char *featpath,int type);

const char *ffeature_string(const cst_item *item,const char *featpath)
{
    return val_string(ffeature(item,featpath));
}
int ffeature_int(const cst_item *item,const char *featpath)
{
    return val_int(ffeature(item,featpath));
}
float ffeature_float(const cst_item *item,const char *featpath)
{
    return val_float(ffeature(item,featpath));
}

cst_item* path_to_item(const cst_item *item,const char *featpath)
{
    return (cst_item *)internal_ff(item,featpath,1);
}

const cst_val *ffeature(const cst_item *item,const char *featpath)
{
    return (cst_val *)internal_ff(item,featpath,0);
}

static const void *internal_ff(const cst_item *item,
			       const char *featpath,int type)
{
    const char *tk, *relation;
    cst_utterance *utt;
    const cst_item *pitem;
    void *void_v;
    cst_ffunction ff;
    char tokenstring[TOKENSTRINGBUF+1];
    char *tokens[TOKENSTRINGBUF+1];
    int i,j;

    /* Perform 'split' of featpath into tokenstring using '.' delimiter */
    tokens[0] = tokenstring;
    for (i=0,j=1; i<TOKENSTRINGBUF && featpath[i]; i++)
    {
        if ('.'==featpath[i])
        {
            tokenstring[i] = '\0';
            tokens[j] = &tokenstring[i+1];
            j++;
        }
        else
        {
            tokenstring[i] = featpath[i];
        }
    }
    tokenstring[i]='\0';
    tokens[j] = NULL;

    /* Parse directives */
    j=0;
    for (tk = tokens[j], pitem=item;
	 pitem && 
	     (((type == 0) && tokens[j+1]) ||
	      ((type == 1) && tk));
	 j++, tk = tokens[j])
    {
        if (tk[0]=='R' && tk[0]!='\0' && tk[1]==':')
	{
	    /* A relation move */
	    relation = tk+2; /* the bit past the 'R:' */
	    pitem = item_as(pitem,relation);
	}
	else if (tk[0]=='P' && tk[1]=='\0')
	    pitem = item_parent(pitem);
	else if (tk[0]=='n' && tk[1]=='\0')
	    pitem = item_next(pitem);
	else if (tk[0]=='p' && tk[1]=='\0')
	    pitem = item_prev(pitem);
	else if (tk[0]=='p' && tk[1]=='p' && tk[2]=='\0')
	    pitem = item_prev(item_prev(pitem));
	else if (tk[0]=='n' && tk[1]=='n' && tk[2]=='\0')
	    pitem = item_next(item_next(pitem));
	else if (tk[0]=='d' && tk[1]=='1' && tk[2]=='\0')
	    pitem = item_daughter(pitem);
	else if (tk[0]=='d' && tk[1]=='n' && tk[2]=='\0')
	    pitem = item_last_daughter(pitem);
	else
	{
	    cst_errmsg("ffeature: unknown directive \"%s\" ignored\n",tk);
	    return NULL;
	}
    }

    if (type == 0)
    {
	if (pitem && (utt = item_utt(pitem)) && tk[0]!='\0' && tk[1]=='\0')
// the final two subconditions in the if statement excludes tokens which
// are know not to be ffunctions
	        ff = utt->ffunctions[(int)tk[0]];
	else
	    ff = NULL;
	void_v = NULL;
	if (!ff)
	    void_v = (void *)item_feat(pitem,tk);
	else if (pitem)
	{
	    void_v = (void *)(*ff)(pitem);
	}
	if (void_v == NULL)
        {
	    void_v = (void *)&ffeature_default_val;
        }
    }
    else
	void_v = (void *)pitem;

    return void_v;
}

void ff_register(cst_ffunction *ffunctions, const char *name, cst_ffunction f)
{
//  Register features functions in indexed lookup table.
//  'name' should be a single byte string symbol which will be used as an
//  unique index. This should be recorded in bell_ff_sym.h

    if (name[1]!='\0' && name[1] != '\0')
        cst_errmsg("warning: ffunction identifier %s too long - truncating", name);
    if (ffunctions[(int)name[0]] != NULL)
	cst_errmsg("warning: ffunction %s redefined\n", name);
    ffunctions[(int)name[0]]=f;
}
