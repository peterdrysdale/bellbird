/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*  ---------------------------------------------------------------  */
/*      The HMM-Based Speech Synthesis System (HTS): version 1.1b    */
/*                        HTS Working Group                          */
/*                                                                   */
/*                   Department of Computer Science                  */
/*                   Nagoya Institute of Technology                  */
/*                                and                                */
/*    Interdisciplinary Graduate School of Science and Engineering   */
/*                   Tokyo Institute of Technology                   */
/*                      Copyright (c) 2001-2003                      */
/*                        All Rights Reserved.                       */
/*                                                                   */
/*  Permission is hereby granted, free of charge, to use and         */
/*  distribute this software and its documentation without           */
/*  restriction, including without limitation the rights to use,     */
/*  copy, modify, merge, publish, distribute, sublicense, and/or     */
/*  sell copies of this work, and to permit persons to whom this     */
/*  work is furnished to do so, subject to the following conditions: */
/*                                                                   */
/*    1. The code must retain the above copyright notice, this list  */
/*       of conditions and the following disclaimer.                 */
/*                                                                   */
/*    2. Any modifications must be clearly marked as such.           */
/*                                                                   */    
/*  NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSITITUTE OF TECHNOLOGY,  */
/*  HTS WORKING GROUP, AND THE CONTRIBUTORS TO THIS WORK DISCLAIM    */
/*  ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL       */
/*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL NAGOYA INSTITUTE OF TECHNOLOGY, TOKYO INSITITUTE OF        */
/*  TECHNOLOGY, HTS WORKING GROUP, NOR THE CONTRIBUTORS BE LIABLE    */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY        */
/*  DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  */
/*  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTUOUS   */
/*  ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR          */
/*  PERFORMANCE OF THIS SOFTWARE.                                    */
/*                                                                   */
/*  ---------------------------------------------------------------  */
/*   This is Zen's MLSA filter as ported by Toda to festvox vc        */
/*   and back ported into hts/festival so we can do MLSA filtering   */
/*   If I took more time I could probably make this use the same as  */
/*   as the other code in this directory -- awb@cs.cmu.edu 03JAN06   */
/*  ---------------------------------------------------------------  */
/*   and then ported into Flite (November 2007 awb@cs.cmu.edu)       */

/*********************************************************************/
/*                                                                   */
/*  vector (etc) code common to mlpg and mlsa                        */
/*-------------------------------------------------------------------*/

#include <math.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_vc.h"

/* from vector.cc */

LVECTOR xlvalloc(long length)
{
    LVECTOR x;

    length = MAX(length, 0);
    x = cst_alloc(struct LVECTOR_STRUCT,1);
    x->data = cst_alloc(long,MAX(length, 1));
    x->length = length;

    return x;
}

void xlvfree(LVECTOR x)
{
    if (x != NULL) {
	if (x->data != NULL) {
	    cst_free(x->data);
	}
	cst_free(x);
    }

    return;
}

DVECTOR xdvalloc(long length)
{
    DVECTOR x;

    length = MAX(length, 0);
    x = cst_alloc(struct DVECTOR_STRUCT,1);
    x->data = cst_alloc(double,MAX(length, 1));
    x->length = length;

    return x;
}

void xdvfree(DVECTOR x)
{
    if (x != NULL) {
	if (x->data != NULL) {
	    cst_free(x->data);
	}
	cst_free(x);
    }

    return;
}

DMATRIX xdmalloc(long row, long col)
{
    DMATRIX matrix;
    int i;

    matrix = cst_alloc(struct DMATRIX_STRUCT,1);
    matrix->data = cst_alloc(double *,row);
    for (i=0; i<row; i++)
        matrix->data[i] = cst_alloc(double,col);
    matrix->row = row;
    matrix->col = col;

    return matrix;
}

void xdmfree(DMATRIX matrix)
{
    int i;

    if (matrix != NULL) {
	if (matrix->data != NULL) {
            for (i=0; i<matrix->row; i++)
                cst_free(matrix->data[i]);
            cst_free(matrix->data);
	}
	cst_free(matrix);
    }

    return;
}
