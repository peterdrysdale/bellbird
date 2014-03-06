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

#include <math.h>
#include "cst_alloc.h"
#include "cst_cg.h"
#include "cst_error.h"
#include "cst_string.h"
#include "cst_track.h"
#include "cst_wave.h"
#include "cst_vc.h"
#include "pstreamchol.h"

#define	WLEFT 0
#define	WRIGHT 1
#define PI2             6.283185307179586477
#define	INFTY2 ((double) 1.0e+19)

typedef struct MLPGPARA_STRUCT {
    dvector ov;
    dvector flkv;
    dmatrix stm;
    dmatrix dltm;
    dmatrix pdf;
    dvector detvec;
    dvector wght;
    dmatrix mean;
    dmatrix cov;
    double  clsdet;
    dvector clscov;
} *MLPGPARA;

// NOTE NOTE NOTE we are including static functions here not header files
#include "../commonsynth/cholesky.c"

static MLPGPARA xmlpgpara_init(int dim, int dim_st, int nframes)
{
    MLPGPARA param;
    
    // memory allocation
    param = cst_alloc(struct MLPGPARA_STRUCT,1);
    param->ov = xdvalloc(dim);
    param->flkv = xdvalloc(nframes);
    param->stm = NULL;
    param->dltm = xdmalloc(nframes, dim_st);
    param->pdf = NULL;
    param->detvec = NULL;
    param->wght = xdvalloc(nframes);
    param->mean = xdmalloc(nframes, dim);
    param->cov = NULL;
    param->clscov = xdvalloc(dim);

    return param;
}

static void xmlpgparafree(MLPGPARA param)
{
    if (param != NULL)
    {
        if (param->ov != NULL) xdvfree(param->ov);
        if (param->flkv != NULL) xdvfree(param->flkv);
        if (param->stm != NULL) xdmfree(param->stm);
        if (param->dltm != NULL) xdmfree(param->dltm);
        if (param->pdf != NULL) xdmfree(param->pdf);
        if (param->detvec != NULL) xdvfree(param->detvec);
        if (param->wght != NULL) xdvfree(param->wght);
        if (param->mean != NULL) xdmfree(param->mean);
        if (param->cov != NULL) xdmfree(param->cov);
        if (param->clscov != NULL) xdvfree(param->clscov);
        cst_free(param);
    }

    return;
}

static double get_gauss_dia(dvector ov,   // [dim]
                     double clsdet,
                     dvector wght,        // [nframes]
                     dmatrix mean,        // [nframes][dim]
                     dvector clscov)      // [dim]
{
    double gauss, sb;
    long k;

    if (clsdet <= 0.0)
    {
        cst_errmsg("#error: det <= 0.0\n");
        cst_error();
    }

    for (k = 0, gauss = 0.0; k < ov->length; k++)
    {
        sb = ov->data[k] - mean->data[0][k];
        gauss += sb * clscov->data[k] * sb;
    }

    gauss = wght->data[0]
        / sqrt(pow(PI2, (double)ov->length) * clsdet)
        * exp(-gauss / 2.0);

    return gauss;
}

static double get_like_pdfseq_vit(int dim, int dim_st, int nframes,
                                  MLPGPARA param, float **model)
{
    long d, c, k, j;
    double sumgauss;
    double like = 0.0;

    for (d = 0, like = 0.0; d < nframes; d++)
    {
        // read weight and mean sequences
        param->wght->data[0] = 0.9; /* FIXME weights */
        for (j=0; j<dim; j++)
        {
            param->mean->data[0][j] = model[d][(j+1)*2];
        }

        // observation vector
        for (k = 0; k < dim_st; k++)
        {
            param->ov->data[k] = param->stm->data[d][k];
            param->ov->data[k + dim_st] = param->dltm->data[d][k];
	}

        // mixture index
        c = d;
        param->clsdet = param->detvec->data[c];

        // calculating likelihood
        for (k = 0; k < param->clscov->length; k++)
        {
            param->clscov->data[k] = param->cov->data[c][k];
        }
        sumgauss = get_gauss_dia(param->ov, param->clsdet,
                                  param->wght, param->mean, param->clscov);

        if (sumgauss <= 0.0)
        {
            param->flkv->data[d] = -1.0 * INFTY2;
        } else
        {
            param->flkv->data[d] = log(sumgauss);
        }
        like += param->flkv->data[d];

        // estimating U', U'*M
        // PDF [U'*M U']
        for (k = 0; k < dim; k++)
        {
            param->pdf->data[d][k] = param->clscov->data[k] * param->mean->data[0][k];
            param->pdf->data[d][k + dim] = param->clscov->data[k];
        }
    }

    like /= (double)nframes;

    return like;
}

static void get_dltmat(dmatrix mat, DWin *dw, int dno, dmatrix dmat)
{
    int i, j, k, tmpnum;

    tmpnum = (int)mat->row - dw->width[dno][WRIGHT];
    for (k = dw->width[dno][WRIGHT]; k < tmpnum; k++)  // time index
    {
        for (i = 0; i < (int)mat->col; i++)	// dimension index
        {
            for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
                   j <= dw->width[dno][WRIGHT]; j++)
            {
                dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];
            }
        }
    }
    for (i = 0; i < (int)mat->col; i++)                  // dimension index
    {
        for (k = 0; k < dw->width[dno][WRIGHT]; k++)     // time index
        {
            for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
                  j <= dw->width[dno][WRIGHT]; j++)
            {
                if (k + j >= 0)
                {
                    dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];
                }
                else
                {
                    dmat->data[k][i] += (2.0 * mat->data[0][i] - mat->data[-k - j][i])
                                           * dw->coef[dno][j];
                }
            }
        }
        for (k = tmpnum; k < (int)mat->row; k++)	// time index
        {
            for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
                   j <= dw->width[dno][WRIGHT]; j++)
            {
                if (k + j < (int)mat->row)
                {
                    dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];
                }
                else
                {
                    dmat->data[k][i] += (2.0 * mat->data[mat->row-1][i]
                                             - mat->data[mat->row-k-j + mat->row-2][i])
                                             * dw->coef[dno][j];
                }
            }
        }
    }

    return;
}

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
    register int i, j, k, l, n;
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
   register int m;

   // generating parameter in each dimension
   for (m = 0; m<=pst->order; m++)
   {
       calc_R_and_r(pst, m);
       Cholesky(pst);
       Cholesky_forward(pst);
       Cholesky_backward(pst, m);
   }
   
   return;
}

static void mlgparaChol(dmatrix pdf, PStreamChol *pst, dmatrix mlgp)
{
    int t, d;

    // error check
    if (pst->vSize * 2 != pdf->col || pst->order + 1 != mlgp->col)
    {
	cst_errmsg("Error mlgparaChol: Different dimension\n");
        cst_error();
    }

    // mseq: U^{-1}*M,	ifvseq: U^{-1}
    for (t = 0; t < pst->T; t++)
    {
        for (d = 0; d < pst->vSize; d++)
        {
            pst->mseq[t][d] = pdf->data[t][d];
            pst->ivseq[t][d] = pdf->data[t][pst->vSize + d];
        }
    } 

    // ML parameter generation
    mlpgChol(pst);

    // extracting parameters
    for (t = 0; t < pst->T; t++)
    {
        for (d = 0; d <= pst->order; d++) mlgp->data[t][d] = pst->c[t][d];
    }

    return;
}

// diagonal covariance
static dvector xget_detvec_diamat2inv(dmatrix covmat)	// [nframes][dim]
{
    long dim, nframes;
    long i, j;
    double det;
    dvector detvec = NULL;

    nframes = covmat->row;
    dim = covmat->col;
    // memory allocation
    detvec = xdvalloc(nframes);
    for (i = 0; i < nframes; i++)
    {
        for (j = 0, det = 1.0; j < dim; j++)
        {
            det *= covmat->data[i][j];
            if (det > 0.0)
            {
		covmat->data[i][j] = 1.0 / covmat->data[i][j];
	    } else {
                cst_errmsg("error:(class %ld) determinant <= 0, det = %f\n", i, det);
                xdvfree(detvec);
                return NULL;
            }
        }
        detvec->data[i] = det;
    }

    return detvec;
}

static void pst_free(PStreamChol *pst)
{
    int i;

    for (i=0; i<pst->dw.num; i++) cst_free(pst->dw.width[i]);
    cst_free(pst->dw.width); pst->dw.width = NULL;
    for (i=0; i<pst->dw.num; i++) cst_free(pst->dw.coef_ptrs[i]);
    cst_free(pst->dw.coef); pst->dw.coef = NULL;
    cst_free(pst->dw.coef_ptrs); pst->dw.coef_ptrs = NULL;

    bell_free_dmatrix(pst->mseq,pst->T);
    bell_free_dmatrix(pst->ivseq,pst->T);
    bell_free_dmatrix(pst->R,pst->T);
    cst_free(pst->r);
    cst_free(pst->g);
    bell_free_dmatrix(pst->c,pst->T);

    return;
}

cst_track *cg_mlpg(const cst_track *param_track, cst_cg_db *cg_db)
{
// Generate an (mcep) track using Maximum Likelihood Parameter Generation
    MLPGPARA param = NULL;
    cst_track *out;
    int dim, dim_st;
    int i,j;
    int nframes;
    PStreamChol pst;

    nframes = param_track->num_frames;
    dim = (param_track->num_channels/2)-1;
    dim_st = dim/2;
    out = new_track();
    cst_track_resize(out,nframes,dim_st+1);

    param = xmlpgpara_init(dim,dim_st,nframes);

    // initial static feature sequence
    param->stm = xdmalloc(nframes,dim_st);
    for (i=0; i<nframes; i++)
    {
        for (j=0; j<dim_st; j++)
        {
            param->stm->data[i][j] = param_track->frames[i][(j+1)*2];
        }
    }

    /* Load cluster means */
    for (i=0; i<nframes; i++)
    {
        for (j=0; j<dim_st; j++)
        {
            param->mean->data[i][j] = param_track->frames[i][(j+1)*2];
        }
    }
    
    /* GMM parameters diagonal covariance */
    InitPStreamChol(&pst, cg_db->dynwin, cg_db->dynwinsize, dim_st-1, nframes);
    param->pdf = xdmalloc(nframes,dim*2);
    param->cov = xdmalloc(nframes,dim);
    for (i=0; i<nframes; i++)
    {
        for (j=0; j<dim; j++)
        {
            param->cov->data[i][j] = param_track->frames[i][(j+1)*2+1] *
                                       param_track->frames[i][(j+1)*2+1];
        }
    }
    param->detvec = xget_detvec_diamat2inv(param->cov);

    get_dltmat(param->stm, &pst.dw, 1, param->dltm);

    get_like_pdfseq_vit(dim, dim_st, nframes, param, param_track->frames);

    mlgparaChol(param->pdf, &pst, param->stm);

    /* Put the answer back into the output track */
    for (i=0; i<nframes; i++)
    {
        out->frames[i][0] = param_track->frames[i][0]; /* F0 */
        for (j=0; j<dim_st; j++)
        {
            out->frames[i][j+1] = param->stm->data[i][j];
        }
    }

    // memory free
    xmlpgparafree(param);
    pst_free(&pst);

    return out;
}
