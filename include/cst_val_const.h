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
/*  Const Vals, and macros to define them                                */
/*                                                                       */
/*  Before you give up in disgust bear with me on this.  Every single    */
/*  line in this file has been *very* carefully decided on after much    */
/*  thought, experimentation and reading more specs of the C language    */
/*  than most people even thought existed.  However inspite of that, the */
/*  result is still unsatisfying from an elegance point of view but the  */
/*  given all the constraints this is probably the best compromise.      */
/*                                                                       */
/*  This file offers macros for defining const cst_vals.  I know many    */
/*  are already laughing at me for wanting runtime types on objects and  */
/*  will use this code as exemplars of why this should be done in C++, I */
/*  say good luck to them with their 4M footprint while I go for my      */
/*  50K footprint.  But I *will* do cst_vals in 8 bytes and I *will*     */
/*  have them defined const so they are in the text segment              */
/*                                                                       */
/*  The problem here is that there is not yet a standard way to do       */
/*  initialization of unions.  There is one in the C99 standard and GCC  */
/*  already supports it, but other compilers do not so I can't use that  */
/*                                                                       */
/*  BELLBIRD NOTE:- As of Dec 2013 even MS-VS 2013 supports designated   */
/*         initializers so the legacy option has now been removed        */
/*                                                                       */
/*  For legacy reasons val_consts are more unpleasant than they should   */
/*  be and force changes elsewhere in the code even when the compiler    */
/*  does support initialization of unions. This may be changed in the    */
/*  future.                                                              */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_VAL_CONST_H__
#define _CST_VAL_CONST_H__

/* There is built-in int to string conversions here for numbers   */
/* up to 20, note if you make this bigger you have to hand change */
/* other things too                                               */
#define CST_CONST_INT_MAX 19

/* This is the simple way when initialization of unions is supported */

#define DEF_CONST_VAL_INT(N,V) const cst_val N = {{.a={.type=CST_VAL_TYPE_INT,.ref_count=-1,.v={.ival=V}}}}
#define DEF_CONST_VAL_STRING(N,S) const cst_val N = {{.a={.type=CST_VAL_TYPE_STRING,.ref_count=-1,.v={.vval= (void *)S}}}}
#define DEF_CONST_VAL_FLOAT(N,F) const cst_val N = {{.a={.type=CST_VAL_TYPE_FLOAT,.ref_count=-1,.v={.fval=F}}}}

#define DEF_STATIC_CONST_VAL_INT(N,V) static DEF_CONST_VAL_INT(N,V)
#define DEF_STATIC_CONST_VAL_STRING(N,S) static DEF_CONST_VAL_STRING(N,S)
#define DEF_STATIC_CONST_VAL_FLOAT(N,F) static DEF_CONST_VAL_FLOAT(N,F)

extern const cst_val val_int_0; 
extern const cst_val val_int_1; 
extern const cst_val val_int_2;
extern const cst_val val_int_3;
extern const cst_val val_int_4;
extern const cst_val val_int_5;
extern const cst_val val_int_6;
extern const cst_val val_int_7;
extern const cst_val val_int_8;
extern const cst_val val_int_9;
extern const cst_val val_int_10; 
extern const cst_val val_int_11; 
extern const cst_val val_int_12;
extern const cst_val val_int_13;
extern const cst_val val_int_14;
extern const cst_val val_int_15;
extern const cst_val val_int_16;
extern const cst_val val_int_17;
extern const cst_val val_int_18;
extern const cst_val val_int_19;

extern const cst_val val_string_0; 
extern const cst_val val_string_1; 
extern const cst_val val_string_2;
extern const cst_val val_string_3;
extern const cst_val val_string_4;
extern const cst_val val_string_5;
extern const cst_val val_string_6;
extern const cst_val val_string_7;
extern const cst_val val_string_8;
extern const cst_val val_string_9;
extern const cst_val val_string_10; 
extern const cst_val val_string_11; 
extern const cst_val val_string_12;
extern const cst_val val_string_13;
extern const cst_val val_string_14;
extern const cst_val val_string_15;
extern const cst_val val_string_16;
extern const cst_val val_string_17;
extern const cst_val val_string_18;
extern const cst_val val_string_19;

const cst_val *val_int_n(int n);

const cst_val *val_string_n(int n);

#endif
