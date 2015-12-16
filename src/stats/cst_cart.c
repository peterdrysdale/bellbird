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
/*                        Copyright (c) 2000                             */
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
/*               Date:  January 2000                                     */
/*************************************************************************/
/*                                                                       */
/*  Classification and Regression Tree (CART) support                    */
/*                                                                       */
/*************************************************************************/
#include "cst_error.h"
#include "cst_regex.h"
#include "cst_cart.h"

CST_VAL_REGISTER_TYPE_NODEL(cart,cst_cart)

/* Make this 1 if you want to debug some cart calls */
#define CART_DEBUG 0

#define CST_CART_NODE_VAL(n,tree) (((tree)->rule_table[n]).val)
#define CST_CART_NODE_OP(n,tree) (((tree)->rule_table[n]).op)
#define CST_CART_NODE_FEAT(n,tree) (tree->feat_table[((tree)->rule_table[n]).feat])

void delete_cart(cst_cart *cart)
{
    /* have to compensate for previous over-zealous consting */
    /* It is assume that given carts, can be freed (i.e. they aren't in */
    /* in the data segment */
    int i;

    if (cart == NULL)
        return;

    for (i=0; cart->rule_table[i].val; i++)
        delete_val((cst_val *)(void *)cart->rule_table[i].val);
    cst_free((void *)cart->rule_table);

    for (i=0; cart->feat_table[i]; i++)
        cst_free((void *)cart->feat_table[i]);
    cst_free((void *)cart->feat_table);

    cst_free(cart);

    return;
}

#if CART_DEBUG
static void cart_print_node(int n, const cst_cart *tree)
{
    cst_errmsg("%s ",CST_CART_NODE_FEAT(n,tree));
    if (CST_CART_NODE_OP(n,tree) == CST_CART_OP_IS)
	cst_errmsg("IS ");
    else if (CST_CART_NODE_OP(n,tree) == CST_CART_OP_LESS)
	cst_errmsg("< ");
    else
	cst_errmsg("*%d* ",CST_CART_NODE_OP(n,tree));
    val_print(CST_CART_NODE_VAL(n,tree));
    cst_errmsg("  ");
}
#endif

const cst_val *cart_interpret(cst_item *item, const cst_cart *tree)
{
    /* Tree interpretation */
    const cst_val *v=0;
    const cst_val *tree_val;
    const char *tree_feat = "";
    cst_features *fcache;
    int r=0;
    int node=0;

    fcache = new_features();

    while (CST_CART_NODE_OP(node,tree) != CST_CART_OP_NONE)
    { //while not a leaf node
#if CART_DEBUG
 	cart_print_node(node,tree);
#endif
	tree_feat = CST_CART_NODE_FEAT(node,tree);

	v = get_param_val(fcache,tree_feat,0);
	if (v == 0)
	{
	    v = ffeature(item,tree_feat);
	    feat_set(fcache,tree_feat,v);
	}

#if CART_DEBUG
	val_print(v);
#endif
	tree_val = CST_CART_NODE_VAL(node,tree);
	if (CST_CART_NODE_OP(node,tree) == CST_CART_OP_IS)
        {
	    r = val_equal(v,tree_val);
        }
	else if (CST_CART_NODE_OP(node,tree) == CST_CART_OP_LESS)
	    r = (val_float(v) < val_float(tree_val));
	else
	{
	    cst_errmsg("cart_interpret: unknown op type %d\n",
		       CST_CART_NODE_OP(node,tree));
	    cst_error();
	}

	if (r)
	{   // Move to yes node
#if CART_DEBUG
            cst_errmsg("   YES\n");
#endif
	    node++;
	}
	else
	{   // Move to no node
#if CART_DEBUG
            cst_errmsg("   NO\n");
#endif
	    node = (tree->rule_table[node]).no_node;
	}
    }

    delete_features(fcache);

    return CST_CART_NODE_VAL(node,tree);
}
