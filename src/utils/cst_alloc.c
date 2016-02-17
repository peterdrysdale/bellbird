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
/*               Date:  July 1999                                        */
/*************************************************************************/
/*                                                                       */
/*  Basic wraparounds for calloc and free                                */
/*                                                                       */
/*************************************************************************/
#include "cst_file.h"
#include "cst_alloc.h"
#include "cst_error.h"

void *cst_safe_alloc(int size)
{
    /* returns pointer to memory all set 0 */
    void *p = NULL;
    if (size < 0)
    {
	cst_errmsg("alloc: asked for negative size %d\n", size);
	cst_error();
    }
    else if (size == 0)  /* some mallocs return NULL for this */
	size++;

    p = (void *)calloc(size,1);

    if (p == NULL)
    {
	cst_errmsg("alloc: can't alloc %d bytes\n", size);
	cst_error();
    }

    return p;
}

void cst_free(void *p)
{
    free(p);
}

double **bell_alloc_dmatrix(size_t row, size_t col)
/* Allocate a simple double matrix */
{
   size_t i;
   double **p = cst_alloc(double *,row);
   p[0] = cst_alloc(double,(col*row));

   for (i = 1; i < row; i++)
      p[i] = p[i-1] + col;
   return p;
}

void bell_free_dmatrix(double **p)
/* Free the simple double matrix */
{
   cst_free(p[0]);
   cst_free(p);
}
