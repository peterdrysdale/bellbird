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
/*  Vals, typed objects                                                  */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_VAL_H__
#define _CST_VAL_H__

#include "cst_file.h"
#include "cst_string.h"
#include "cst_val_defs.h"

/* Only CONS can be an even number */
#define CST_VAL_TYPE_CONS    0
#define CST_VAL_TYPE_INT     1
#define CST_VAL_TYPE_FLOAT   3
#define CST_VAL_TYPE_STRING  5
#define CST_VAL_TYPE_FIRST_FREE 7

typedef struct  cst_val_cons_struct {
    struct cst_val_struct *car;
    struct cst_val_struct *cdr;
}  cst_val_cons;

typedef struct  cst_val_atom_struct {
#ifdef WORDS_BIGENDIAN
    short ref_count;
    short type;  /* order is here important */
#else
#if _M_X64
    int type;  /* order is here important */
    int ref_count;
#else
    short type;  /* order is here important */
    short ref_count;
#endif
#endif
    union 
    {
#if _M_X64
        double fval;
        long long ival;
        void *vval;
#else
        float fval;
        int ival;
        void *vval;
#endif
    } v;
} cst_val_atom;

typedef struct  cst_val_struct {
    union
    {
	cst_val_cons cc;
	cst_val_atom a;
    } c;
} cst_val;

typedef struct cst_val_def_struct {
    const char *name;
    void (*delete_function)(void *);
} cst_val_def;

/* Constructor functions */
cst_val *int_val(int i);
cst_val *float_val(float f);
cst_val *string_val(const char *s);
cst_val *val_new_typed(int type, void *vv);
cst_val *cons_val(const cst_val *a, const cst_val *b);

/* Derefence and delete val if no other references */
void delete_val(cst_val *val);

/* Accessor functions */
int val_int(const cst_val *v);
float val_float(const cst_val *v);
const char *val_string(const cst_val *v);
void *val_generic(const cst_val *v, int type, const char *stype);
const cst_val *val_car(const cst_val *v);
const cst_val *val_cdr(const cst_val *v);

const cst_val *set_car(cst_val *v1, const cst_val *v2);

int cst_val_consp(const cst_val *v);

/* Some standard function */
int val_equal(const cst_val *a, const cst_val *b);
const cst_val *val_assoc_string(const char *v1,const cst_val *al);

#if defined(CART_DEBUG) || defined(SSML_DEBUG)
void val_print(const cst_val *v);
#endif // defined(CART_DEBUG) || defined(SSML_DEBUG)

cst_val *val_readlist_string(const char *str);

cst_val *val_reverse(cst_val *v);
cst_val *val_append(cst_val *a,cst_val *b);
int val_length(const cst_val *l);

/* make sure you know what you are doing before you call these */
cst_val *val_inc_refcount(const cst_val *b);

#include "cst_val_const.h"
extern const cst_val_def cst_val_defs[];

/* Generic pointer vals */
typedef void cst_userdata;
CST_VAL_USER_TYPE_DCLS(userdata,cst_userdata)

#endif
