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
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  Without a real garbage collector we need some basic constant val     */
/*  to avoid having to create them on the fly                            */
/*                                                                       */
/*************************************************************************/
#include "cst_val.h"
#include "cst_features.h"
#include "cst_string.h"

DEF_CONST_VAL_STRING(val_string_0,"0");
DEF_CONST_VAL_STRING(val_string_1,"1");
DEF_CONST_VAL_STRING(val_string_2,"2");
DEF_CONST_VAL_STRING(val_string_3,"3");
DEF_CONST_VAL_STRING(val_string_4,"4");
DEF_CONST_VAL_STRING(val_string_5,"5");
DEF_CONST_VAL_STRING(val_string_6,"6");
DEF_CONST_VAL_STRING(val_string_7,"7");
DEF_CONST_VAL_STRING(val_string_8,"8");
DEF_CONST_VAL_STRING(val_string_9,"9");
DEF_CONST_VAL_STRING(val_string_10,"10");
DEF_CONST_VAL_STRING(val_string_11,"11");
DEF_CONST_VAL_STRING(val_string_12,"12");
DEF_CONST_VAL_STRING(val_string_13,"13");
DEF_CONST_VAL_STRING(val_string_14,"14");
DEF_CONST_VAL_STRING(val_string_15,"15");
DEF_CONST_VAL_STRING(val_string_16,"16");
DEF_CONST_VAL_STRING(val_string_17,"17");
DEF_CONST_VAL_STRING(val_string_18,"18");
DEF_CONST_VAL_STRING(val_string_19,"19");

DEF_CONST_VAL_INT(val_int_0,0);
DEF_CONST_VAL_INT(val_int_1,1);
DEF_CONST_VAL_INT(val_int_2,2);
DEF_CONST_VAL_INT(val_int_3,3);
DEF_CONST_VAL_INT(val_int_4,4);
DEF_CONST_VAL_INT(val_int_5,5);
DEF_CONST_VAL_INT(val_int_6,6);
DEF_CONST_VAL_INT(val_int_7,7);
DEF_CONST_VAL_INT(val_int_8,8);
DEF_CONST_VAL_INT(val_int_9,9);
DEF_CONST_VAL_INT(val_int_10,10);
DEF_CONST_VAL_INT(val_int_11,11);
DEF_CONST_VAL_INT(val_int_12,12);
DEF_CONST_VAL_INT(val_int_13,13);
DEF_CONST_VAL_INT(val_int_14,14);
DEF_CONST_VAL_INT(val_int_15,15);
DEF_CONST_VAL_INT(val_int_16,16);
DEF_CONST_VAL_INT(val_int_17,17);
DEF_CONST_VAL_INT(val_int_18,18);
DEF_CONST_VAL_INT(val_int_19,19);

static const int val_int_const_max = 20;
static const cst_val * const val_int_const [] = {
    &val_int_0,
    &val_int_1,
    &val_int_2,
    &val_int_3,
    &val_int_4,
    &val_int_5,
    &val_int_6,
    &val_int_7,
    &val_int_8,
    &val_int_9,
    &val_int_10,
    &val_int_11,
    &val_int_12,
    &val_int_13,
    &val_int_14,
    &val_int_15,
    &val_int_16,
    &val_int_17,
    &val_int_18,
    &val_int_19 };

static const cst_val * const val_string_const [] = {
    &val_string_0,
    &val_string_1,
    &val_string_2,
    &val_string_3,
    &val_string_4,
    &val_string_5,
    &val_string_6,
    &val_string_7,
    &val_string_8,
    &val_string_9,
    &val_string_10,
    &val_string_11,
    &val_string_12,
    &val_string_13,
    &val_string_14,
    &val_string_15,
    &val_string_16,
    &val_string_17,
    &val_string_18,
    &val_string_19 };
  
const cst_val *val_int_n(int n)
{
    if (n < val_int_const_max)
	return val_int_const[n];
    else
	return val_int_const[val_int_const_max-1];
}

/* carts are pretty confused about strings/ints, and some features */
/* are actually used as floats and as int/strings                  */
const cst_val *val_string_n(int n)
{
    if (n < 0)
	return val_string_const[0];
    else if (n < val_int_const_max) /* yes I mean *int*, its the table size */
	return val_string_const[n];
    else
	return val_string_const[val_int_const_max-1];
}
