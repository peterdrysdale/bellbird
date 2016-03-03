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
/*  Maximum Likelihood Parameter Generation                          */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "cst_alloc.h"
#include "cst_cg.h"
#include "cst_error.h"
#include "cst_track.h"
#include "pstream.h"

#define	WLEFT 0
#define	WRIGHT 1

// NOTE NOTE NOTE we are including static functions here not header files
#include "../commonsynth/ldl.c"

static void InitDWin(PStream *pst, const float *dynwin, int fsize)
{
    int j;
    int leng;

    pst->dw.num = 1;      // only static
    if (dynwin) pst->dw.num = 2;  // static + dyn

    leng = fsize / 2;			// L (fsize = 2 * L + 1)

    // memory allocation
    // set delta coefficients
    if (pst->dw.num ==2)
    {
        pst->dw.coef_ptrs = cst_alloc(double,fsize);
        pst->dw.coef = pst->dw.coef_ptrs;
        for (j=0; j<fsize; j++) /* FIXME make dynwin doubles for memmove */
        {
            pst->dw.coef[j] = (double)dynwin[j];
        }
        // set pointer
        pst->dw.coef += leng;		// [L] -> [0]	center
    }
    else
    {
        pst->dw.coef_ptrs = cst_alloc(double,1); // unused dummy allocation
        pst->dw.coef = pst->dw.coef_ptrs;
    }
    pst->dw.width[WLEFT] = -leng;	// -L		left
    pst->dw.width[WRIGHT] = leng;	//  L		right
    if (fsize % 2 == 0) pst->dw.width[WRIGHT]--;

    return;
}

static void InitPStream(PStream *pst, const float *dynwin, int fsize,
                            int T)
{
    // windows for dynamic feature
    InitDWin(pst, dynwin, fsize);

    // memory allocation
    pst->T = T;                                   // number of frames
    pst->width = pst->dw.width[WRIGHT] * 2 + 1;	  // width of R
    pst->mseq = bell_alloc_dmatrix(T,2);          // [T][2]
    pst->ivseq = bell_alloc_dmatrix(T,2);         // [T][2]
    pst->R = bell_alloc_dmatrix(T,pst->width);    // [T][width]
    pst->r = cst_alloc(double,T);                 // [T]
    pst->g = cst_alloc(double,T);                 // [T]
    pst->c = cst_alloc(double,T);                 // [T]

    return;
}

static void calc_R_and_r(PStream *pst)
{
// calculate R = W'U^{-1}W and r = W'U^{-1}M
    int i, j, k, n;
    double   wu;

    for (i = 0; i < pst->T; i++) {
	pst->r[i] = pst->mseq[i][0];
	pst->R[i][0] = pst->ivseq[i][0];
      
	for (j = 1; j < pst->width; j++) pst->R[i][j] = 0.0;

        if (pst->dw.num == 2){
	    for (j = pst->dw.width[WLEFT]; j <= pst->dw.width[WRIGHT]; j++) {
		n = i+j;
		if (n >= 0 && n < pst->T && pst->dw.coef[-j] != 0.0) {
		    pst->r[i] += pst->dw.coef[-j] * pst->mseq[n][1];
		    wu = pst->dw.coef[-j] * pst->ivseq[n][1];
		    for (k = 0; k < pst->width; k++) {
			n = k-j;
			if (n<=pst->dw.width[WRIGHT] && (i+k) < pst->T &&
			    pst->dw.coef[n] != 0.0)
			    pst->R[i][k] += wu * pst->dw.coef[n];
		    }
		}
	    }
	}
    }

    return;
}

static void pst_free(PStream *pst)
{
    pst->dw.coef = NULL;
    cst_free(pst->dw.coef_ptrs); pst->dw.coef_ptrs = NULL;

    bell_free_dmatrix(pst->mseq);
    bell_free_dmatrix(pst->ivseq);
    bell_free_dmatrix(pst->R);
    cst_free(pst->r);
    cst_free(pst->g);
    cst_free(pst->c);

    return;
}

void cg_mlpg(const cst_track *param_track, cst_cg_db *cg_db)
{
// Generate an (mcep) track using Maximum Likelihood Parameter Generation
// Overwrite param_track with answer.
    const int dim = (param_track->num_channels/2)-1;
    const int dim_st = dim/2;
    int i,k,idx,idx1;
    const int num_frames= param_track->num_frames;
    PStream pst;

    InitPStream(&pst, cg_db->dynwin, cg_db->dynwinsize, num_frames);

// generate parameter sequence using LDL decomposition
// and store results in param_track->frames
    for (i = 0; i < dim_st; i++)
    {
        /* GMM parameters diagonal covariance */
        // Calculate only those terms we need for this loop iteration
        // for memory efficiency
        idx=(i+1)*2+1;
        idx1=idx+dim;
        for (k=0; k < num_frames; k++)
        {
            // estimating U', U'*M
            // PDF [U'*M U']
            pst.ivseq[k][0] = 1.0 /(param_track->frames[k][idx] *
                                param_track->frames[k][idx]);
            pst.mseq[k][0] = pst.ivseq[k][0] * param_track->frames[k][idx-1];
                                          //Latter term in product is mean
            pst.ivseq[k][1] = 1.0 /(param_track->frames[k][idx1] *
                                param_track->frames[k][idx1]);
            pst.mseq[k][1] = pst.ivseq[k][1] * param_track->frames[k][idx1-1];
                                          //Latter term in product is mean
        }

        calc_R_and_r(&pst);
        solvemateqn(&pst, param_track->frames, i+1);
    }

    pst_free(&pst); // free pst

    return;
}
