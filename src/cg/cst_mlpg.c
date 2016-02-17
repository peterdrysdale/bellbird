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
#include "pstreamchol.h"

#define	WLEFT 0
#define	WRIGHT 1

// NOTE NOTE NOTE we are including static functions here not header files
#include "../commonsynth/cholesky.c"

/////////////////////////////////////
// ML using Choleski decomposition //
/////////////////////////////////////
static void InitDWin(PStreamChol *pst, const float *dynwin, int fsize)
{
    int i,j;
    int leng;

    pst->dw.num = 1;      // only static
    if (dynwin) pst->dw.num = 2;  // static + dyn

    // memory allocation
    pst->dw.width = cst_alloc(int *,pst->dw.num);
    for (i = 0; i < pst->dw.num; i++)
    {
        pst->dw.width[i] = cst_alloc(int,2);
    }

    pst->dw.coef = cst_alloc(double *,pst->dw.num);
    pst->dw.coef_ptrs = cst_alloc(double *,pst->dw.num);
    // window for static parameter	WLEFT = 0, WRIGHT = 1
    pst->dw.width[0][WLEFT] = pst->dw.width[0][WRIGHT] = 0;
    pst->dw.coef_ptrs[0] = cst_alloc(double,1);
    pst->dw.coef[0] = pst->dw.coef_ptrs[0];
    pst->dw.coef[0][0] = 1.0;

    // set delta coefficients
    for (i = 1; i < pst->dw.num; i++)
    {
        pst->dw.coef_ptrs[i] = cst_alloc(double,fsize);
        pst->dw.coef[i] = pst->dw.coef_ptrs[i];
        for (j=0; j<fsize; j++) /* FIXME make dynwin doubles for memmove */
        {
            pst->dw.coef[i][j] = (double)dynwin[j];
        }
        // set pointer
        leng = fsize / 2;			// L (fsize = 2 * L + 1)
        pst->dw.coef[i] += leng;		// [L] -> [0]	center
        pst->dw.width[i][WLEFT] = -leng;	// -L		left
        pst->dw.width[i][WRIGHT] = leng;	//  L		right
        if (fsize % 2 == 0) pst->dw.width[i][WRIGHT]--;
    }

    pst->dw.maxw[WLEFT] = pst->dw.maxw[WRIGHT] = 0;
    for (i = 0; i < pst->dw.num; i++)
    {
        if (pst->dw.maxw[WLEFT] > pst->dw.width[i][WLEFT])
        {
	    pst->dw.maxw[WLEFT] = pst->dw.width[i][WLEFT];
        }
        if (pst->dw.maxw[WRIGHT] < pst->dw.width[i][WRIGHT])
        {
	    pst->dw.maxw[WRIGHT] = pst->dw.width[i][WRIGHT];
        }
    }

    pst->dw.max_L = pst->dw.maxw[WRIGHT]; // Set for compatibility with nitech module

    return;
}

static void InitPStreamChol(PStreamChol *pst, const float *dynwin, int fsize,
                            int order, int T)
{
    // order of cepstrum
    pst->order = order;

    // windows for dynamic feature
    InitDWin(pst, dynwin, fsize);

    // dimension of observed vector
    pst->vSize = (pst->order + 1) * pst->dw.num;  // odim = dim * (1--3)

    // memory allocation
    pst->T = T;                                   // number of frames
    pst->width = pst->dw.maxw[WRIGHT] * 2 + 1;	  // width of R
    pst->mseq = bell_alloc_dmatrix(T,pst->vSize); // [T][odim]
    pst->ivseq = bell_alloc_dmatrix(T,pst->vSize);// [T][odim]
    pst->R = bell_alloc_dmatrix(T,pst->width);    // [T][width]
    pst->r = cst_alloc(double,T);                 // [T]
    pst->g = cst_alloc(double,T);                 // [T]
    pst->c = bell_alloc_dmatrix(T,pst->order + 1);// [T][dim]

    return;
}

//------ parameter generation fuctions
// calc_R_and_r: calculate R = W'U^{-1}W and r = W'U^{-1}M
static void calc_R_and_r(PStreamChol *pst, const int m)
{
    int i, j, k, l, n;
    double   wu;
   
    for (i = 0; i < pst->T; i++) {
	pst->r[i] = pst->mseq[i][m];
	pst->R[i][0] = pst->ivseq[i][m];
      
	for (j = 1; j < pst->width; j++) pst->R[i][j] = 0.0;
      
	for (j = 1; j < pst->dw.num; j++) {
	    for (k = pst->dw.width[j][0]; k <= pst->dw.width[j][1]; k++) {
		n = i+k;
		if (n >= 0 && n < pst->T && pst->dw.coef[j][-k] != 0.0) {
		    l = j*(pst->order+1)+m;
		    pst->r[i] += pst->dw.coef[j][-k] * pst->mseq[n][l]; 
		    wu = pst->dw.coef[j][-k] * pst->ivseq[n][l];
            
		    for (l = 0; l<pst->width; l++) {
			n = l-k;
			if (n<=pst->dw.width[j][1] && i+l<pst->T &&
			    pst->dw.coef[j][n] != 0.0)
			    pst->R[i][l] += wu * pst->dw.coef[j][n];
		    }
		}
	    }
	}
    }

    return;
}

// generate parameter sequence from pdf sequence using Cholesky decomposition
static void mlpgChol(PStreamChol *pst)
{
   int m;

   // generating parameter in each dimension
   for (m = 0; m<=pst->order; m++)
   {
       calc_R_and_r(pst, m);
       solvemateqn(pst,m);
   }
   
   return;
}

static void pst_free(PStreamChol *pst)
{
    int i;

    for (i=0; i<pst->dw.num; i++) cst_free(pst->dw.width[i]);
    cst_free(pst->dw.width); pst->dw.width = NULL;
    for (i=0; i<pst->dw.num; i++) cst_free(pst->dw.coef_ptrs[i]);
    cst_free(pst->dw.coef); pst->dw.coef = NULL;
    cst_free(pst->dw.coef_ptrs); pst->dw.coef_ptrs = NULL;

    bell_free_dmatrix(pst->mseq);
    bell_free_dmatrix(pst->ivseq);
    bell_free_dmatrix(pst->R);
    cst_free(pst->r);
    cst_free(pst->g);
    bell_free_dmatrix(pst->c);

    return;
}

cst_track *cg_mlpg(const cst_track *param_track, cst_cg_db *cg_db)
{
// Generate an (mcep) track using Maximum Likelihood Parameter Generation
    cst_track *out;
    int dim, dim_st;
    int i,j;
    int nframes;
    PStreamChol pst;

    nframes = param_track->num_frames;
    dim = (param_track->num_channels/2)-1;
    dim_st = dim/2;
    out = new_track(nframes,dim_st+1);

    /* GMM parameters diagonal covariance */
    InitPStreamChol(&pst, cg_db->dynwin, cg_db->dynwinsize, dim_st-1, nframes);
    for (i=0; i<nframes; i++)
    {
        for (j=0; j<dim; j++)
        {
            // estimating U', U'*M
            // PDF [U'*M U']
            pst.ivseq[i][j] = 1.0 /(param_track->frames[i][(j+1)*2+1] *
                                param_track->frames[i][(j+1)*2+1]);
            pst.mseq[i][j] = pst.ivseq[i][j] * param_track->frames[i][(j+1)*2];
                                          //Latter term in product is mean
        }
    }

    mlpgChol(&pst);

    /* Put the answer back into the output track */
    for (i=0; i<nframes; i++)
    {
        out->frames[i][0] = param_track->frames[i][0]; /* F0 */
        for (j=0; j<dim_st; j++)
        {
            out->frames[i][j+1] = pst.c[i][j];
        }
    }

    // memory free
    pst_free(&pst);

    return out;
}
