/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*  ---------------------------------------------------------------  */
/*      The HMM-Based Speech Synthesis System (HTS): version 1.1.1   */
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
/*  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS   */
/*  ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR          */
/*  PERFORMANCE OF THIS SOFTWARE.                                    */
/*                                                                   */
/*  ---------------------------------------------------------------  */
/*    mlpg.c : speech parameter generation from pdf sequence         */
/*                                                                   */
/*                                    2003/12/26 by Heiga Zen        */
/*  ---------------------------------------------------------------  */
/*********************************************************************/
/*                                                                   */
/*            Nagoya Institute of Technology, Aichi, Japan,          */
/*                                and                                */
/*             Carnegie Mellon University, Pittsburgh, PA            */
/*                   Copyright (c) 2003-2004,2008                    */
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
/*    2. Any modifications must be clearly marked as such.           */
/*    3. Original authors' names are not deleted.                    */
/*                                                                   */    
/*  NAGOYA INSTITUTE OF TECHNOLOGY, CARNEGIE MELLON UNIVERSITY, AND  */
/*  THE CONTRIBUTORS TO THIS WORK DISCLAIM ALL WARRANTIES WITH       */
/*  REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF     */
/*  MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NAGOYA INSTITUTE  */
/*  OF TECHNOLOGY, CARNEGIE MELLON UNIVERSITY, NOR THE CONTRIBUTORS  */
/*  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR  */
/*  ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR       */
/*  PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER   */
/*  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE    */
/*  OR PERFORMANCE OF THIS SOFTWARE.                                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*          Author :  Tomoki Toda (tomoki@ics.nitech.ac.jp)          */
/*          Date   :  June 2004                                      */
/*                                                                   */
/*  Modified as a single file for inclusion in festival/flite        */
/*  May 2008 awb@cs.cmu.edu                                          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Matrix equation solving functions using Cholesky decomposition   */
/*                                                                   */
/*-------------------------------------------------------------------*/

/* This source file is included in more than one compilation unit.      */
/* Ordinarily we might use normal functions but we use static functions */
/* as experience has showed us this code is highly performance critical */
/* and the compiler likes to inline and improve performance of these    */
/* functions if we declare them static.                                 */

static void Cholesky(PStreamChol *pst)
{
// In place Cholesky factorization of matrix R
// First step to solving matrix equation R c = r
    register int i, j, k;

    pst->R[0][0] = sqrt(pst->R[0][0]);

    for (i = 1; i < pst->width; i++)
       pst->R[0][i] /= pst->R[0][0];

    for (i = 1; i < pst->T; i++) {
	for (j=1; j < pst->width; j++)
	    if (i-j >= 0)
		pst->R[i][0] -= pst->R[i-j][j] * pst->R[i-j][j];
         
	pst->R[i][0] = sqrt(pst->R[i][0]);
         
	for (j=1; j<pst->width; j++) {
	   for (k = 0; k < pst->dw.max_L; k++)
		if (j!=pst->width-1)
		    pst->R[i][j] -= pst->R[i-k-1][j-k]*pst->R[i-k-1][j+1];
            
	    pst->R[i][j] /= pst->R[i][0];
	}
    }
   
    return;
}

static void Cholesky_forward(PStreamChol *pst)
{
// Cholesky forward substitution to solve matrix equation R c = r
// This second step generates intermediate result vector 'g'
    register int i, j;
    double hold;
   
    pst->g[0] = pst->r[0] / pst->R[0][0];

    for (i=1; i<pst->T; i++) {
	hold = 0.0;
	for (j=1; j<pst->width; j++) {
	    if (i-j >= 0 && pst->R[i-j][j] != 0.0)
		hold += pst->R[i-j][j] * pst->g[i-j];
        }
	pst->g[i] = (pst->r[i]-hold)/pst->R[i][0];
    }
   
    return;
}

static void Cholesky_backward(PStreamChol *pst, const int m)
{
// Cholesky backward substitution to solve matrix equation R c = r
// This third step uses intermediate result 'g' from Cholesky_forward
// with factorized matrix R to yield solution vector c
    register int i, j;
    double hold;
   
    pst->c[pst->T-1][m] = pst->g[pst->T-1]/pst->R[pst->T-1][0];

    for (i=pst->T-2; i>=0; i--) {
	hold = 0.0;
	for (j=1; j<pst->width; j++) {
	    if (i + j < pst->T && pst->R[i][j] != 0.0)
               hold += pst->R[i][j]*pst->c[i + j][m];
        }
	pst->c[i][m] = (pst->g[i] - hold) / pst->R[i][0];
   }
   
   return;
}