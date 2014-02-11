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
/*  ML-Based Parameter Generation                                    */
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

#define	WLEFT 0
#define	WRIGHT 1
#define PI2             6.283185307179586477
#define	INFTY2 ((double) 1.0e+19)

typedef struct _DWin {
    int	num;		/* number of static + deltas */
    int **width;	/* width [0..num-1][0(left) 1(right)] */
    double **coef;	/* coefficient [0..num-1][length[0]..length[1]] */
    double **coef_ptrs;	/* keeps the pointers so we can free them */
    int maxw[2];	/* max width [0(left) 1(right)] */
} DWin;

typedef struct _PStreamChol {
    int vSize;		// size of ovserved vector
    int order;		// order of cepstrum
    int T;		// number of frames
    int width;		// width of WSW
    DWin dw;
    double **mseq;	// sequence of mean vector
    double **ivseq;	// sequence of invarsed covariance vector
    double ***ifvseq;	// sequence of invarsed full covariance vector
    double **R;		// WSW[T][range]
    double *r;		// WSM [T]
    double *g;		// g [T]
    double **c;		// parameter c
} PStreamChol;

typedef struct MLPGPARA_STRUCT {
    dvector ov;
    dvector iuv;
    dvector iumv;
    dvector flkv;
    dmatrix stm;
    dmatrix dltm;
    dmatrix pdf;
    dvector detvec;
    dmatrix wght;
    dmatrix mean;
    dmatrix cov;
    lvector clsidxv;
    dvector clsdetv;
    dmatrix clscov;
    double vdet;
    dvector vm;
    dvector vv;
    dvector var;
} *MLPGPARA;

static MLPGPARA xmlpgpara_init(int dim, int dim2, int dnum, 
                               int clsnum)
{
    MLPGPARA param;
    
    // memory allocation
    param = cst_alloc(struct MLPGPARA_STRUCT,1);
    param->ov = xdvalloc(dim);
    param->iuv = NULL;
    param->iumv = NULL;
    param->flkv = xdvalloc(dnum);
    param->stm = NULL;
    param->dltm = xdmalloc(dnum, dim2);
    param->pdf = NULL;
    param->detvec = NULL;
    param->wght = xdmalloc(clsnum, 1);
    param->mean = xdmalloc(clsnum, dim);
    param->cov = NULL;
    param->clsidxv = NULL;
    /* dia_flag */
	param->clsdetv = xdvalloc(1);
	param->clscov = xdmalloc(1, dim);

    param->vdet = 1.0;
    param->vm = NULL;
    param->vv = NULL;
    param->var = NULL;

    return param;
}

static void xmlpgparafree(MLPGPARA param)
{
    if (param != NULL) {
	if (param->ov != NULL) xdvfree(param->ov);
	if (param->iuv != NULL) xdvfree(param->iuv);
	if (param->iumv != NULL) xdvfree(param->iumv);
	if (param->flkv != NULL) xdvfree(param->flkv);
	if (param->stm != NULL) xdmfree(param->stm);
	if (param->dltm != NULL) xdmfree(param->dltm);
	if (param->pdf != NULL) xdmfree(param->pdf);
	if (param->detvec != NULL) xdvfree(param->detvec);
	if (param->wght != NULL) xdmfree(param->wght);
	if (param->mean != NULL) xdmfree(param->mean);
	if (param->cov != NULL) xdmfree(param->cov);
	if (param->clsidxv != NULL) xlvfree(param->clsidxv);
	if (param->clsdetv != NULL) xdvfree(param->clsdetv);
	if (param->clscov != NULL) xdmfree(param->clscov);
	if (param->vm != NULL) xdvfree(param->vm);
	if (param->vv != NULL) xdvfree(param->vv);
	if (param->var != NULL) xdvfree(param->var);
	cst_free(param);
    }

    return;
}

static double cal_xmcxmc(long clsidx,
		  dvector x,
		  dmatrix mm,	// [num class][dim]
		  dmatrix cm)	// [num class * dim][dim]
{
    long clsnum, k, l, b, dim;
    double *vec = NULL;
    double td, d;

    dim = x->length;
    clsnum = mm->row;
    b = clsidx * dim;
    if (mm->col != dim || cm->col != dim || clsnum * dim != cm->row) {
	cst_errmsg("Error cal_xmcxmc: different dimension\n");
        cst_error();
    }

    // memory allocation
    vec = cst_alloc(double,(int)dim);
    for (k = 0; k < dim; k++) vec[k] = x->data[k] - mm->data[clsidx][k];
    for (k = 0, d = 0.0; k < dim; k++) {
	for (l = 0, td = 0.0; l < dim; l++) td += vec[l] * cm->data[l + b][k];
	d += td * vec[k];
    }
    // memory free
    cst_free(vec); vec = NULL;

    return d;
}

static double get_gauss_full(long clsidx,
		      dvector vec,		// [dim]
		      dvector detvec,		// [clsnum]
		      dmatrix weightmat,	// [clsnum][1]
		      dmatrix meanvec,		// [clsnum][dim]
		      dmatrix invcovmat)	// [clsnum * dim][dim]
{
    double gauss;

    if (detvec->data[clsidx] <= 0.0) {
	cst_errmsg("#error: det <= 0.0\n");
        cst_error();
    }

    gauss = weightmat->data[clsidx][0]
	/ sqrt(pow(PI2, (double)vec->length) * detvec->data[clsidx])
	* exp(-1.0 * cal_xmcxmc(clsidx, vec, meanvec, invcovmat) / 2.0);
    
    return gauss;
}

static double get_gauss_dia(long clsidx,
		     dvector vec,		// [dim]
		     dvector detvec,		// [clsnum]
		     dmatrix weightmat,		// [clsnum][1]
		     dmatrix meanmat,		// [clsnum][dim]
		     dmatrix invcovmat)		// [clsnum][dim]
{
    double gauss, sb;
    long k;

    if (detvec->data[clsidx] <= 0.0) {
	cst_errmsg("#error: det <= 0.0\n");
        cst_error();
    }

    for (k = 0, gauss = 0.0; k < vec->length; k++) {
	sb = vec->data[k] - meanmat->data[clsidx][k];
	gauss += sb * invcovmat->data[clsidx][k] * sb;
    }

    gauss = weightmat->data[clsidx][0]
	/ sqrt(pow(PI2, (double)vec->length) * detvec->data[clsidx])
	* exp(-gauss / 2.0);

    return gauss;
}

static double get_like_pdfseq_vit(int dim, int dim2, int dnum,
                                  MLPGPARA param, 
                                  float **model, 
                                  int dia_flag)
{
    long d, c, k, l, j;
    double sumgauss;
    double like = 0.0;

    for (d = 0, like = 0.0; d < dnum; d++) {
	// read weight and mean sequences
        param->wght->data[0][0] = 0.9; /* FIXME weights */
        for (j=0; j<dim; j++)
            param->mean->data[0][j] = model[d][(j+1)*2];

	// observation vector
	for (k = 0; k < dim2; k++) {
	    param->ov->data[k] = param->stm->data[d][k];
	    param->ov->data[k + dim2] = param->dltm->data[d][k];
	}

	// mixture index
        c = d;
	param->clsdetv->data[0] = param->detvec->data[c];

	// calculating likelihood
	if (dia_flag) {
	    for (k = 0; k < param->clscov->col; k++)
		param->clscov->data[0][k] = param->cov->data[c][k];
	    sumgauss = get_gauss_dia(0, param->ov, param->clsdetv,
				     param->wght, param->mean, param->clscov);
	} else {
	    for (k = 0; k < param->clscov->row; k++)
		for (l = 0; l < param->clscov->col; l++)
		    param->clscov->data[k][l] =
			param->cov->data[k + param->clscov->row * c][l];
	    sumgauss = get_gauss_full(0, param->ov, param->clsdetv,
				      param->wght, param->mean, param->clscov);
	}
	if (sumgauss <= 0.0) param->flkv->data[d] = -1.0 * INFTY2;
	else param->flkv->data[d] = log(sumgauss);
	like += param->flkv->data[d];

	// estimating U', U'*M
	if (dia_flag) {
	    // PDF [U'*M U']
	    for (k = 0; k < dim; k++) {
		param->pdf->data[d][k] =
		    param->clscov->data[0][k] * param->mean->data[0][k];
		param->pdf->data[d][k + dim] = param->clscov->data[0][k];
	    }
	} else {
	    // PDF [U'*M U']
	    for (k = 0; k < dim; k++) {
		param->pdf->data[d][k] = 0.0;
		for (l = 0; l < dim; l++) {
		    param->pdf->data[d][k * dim + dim + l] =
			param->clscov->data[k][l];
		    param->pdf->data[d][k] +=
			param->clscov->data[k][l] * param->mean->data[0][l];
		}
	    }
	}
    }

    like /= (double)dnum;

    return like;
}

static void get_dltmat(dmatrix mat, DWin *dw, int dno, dmatrix dmat)
{
    int i, j, k, tmpnum;

    tmpnum = (int)mat->row - dw->width[dno][WRIGHT];
    for (k = dw->width[dno][WRIGHT]; k < tmpnum; k++)	// time index
	for (i = 0; i < (int)mat->col; i++)	// dimension index
	    for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
		 j <= dw->width[dno][WRIGHT]; j++)
		dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];

    for (i = 0; i < (int)mat->col; i++) {		// dimension index
	for (k = 0; k < dw->width[dno][WRIGHT]; k++)		// time index
	    for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
		 j <= dw->width[dno][WRIGHT]; j++)
		if (k + j >= 0)
		    dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];
		else
		    dmat->data[k][i] += (2.0 * mat->data[0][i] - mat->data[-k - j][i]) * dw->coef[dno][j];
	for (k = tmpnum; k < (int)mat->row; k++)	// time index
	    for (j = dw->width[dno][WLEFT], dmat->data[k][i] = 0.0;
		 j <= dw->width[dno][WRIGHT]; j++)
		if (k + j < (int)mat->row)
		    dmat->data[k][i] += mat->data[k + j][i] * dw->coef[dno][j];
		else
		    dmat->data[k][i] += (2.0 * mat->data[mat->row - 1][i] - mat->data[mat->row - k - j + mat->row - 2][i]) * dw->coef[dno][j];
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

    pst->dw.num = 1;	// only static
    if (dynwin) {
	pst->dw.num = 2;	// static + dyn
    }
    // memory allocation
    pst->dw.width = cst_alloc(int *,pst->dw.num);
    for (i = 0; i < pst->dw.num; i++)
        pst->dw.width[i] = cst_alloc(int,2);

    pst->dw.coef = cst_alloc(double *,pst->dw.num);
    pst->dw.coef_ptrs = cst_alloc(double *,pst->dw.num);
    // window for static parameter	WLEFT = 0, WRIGHT = 1
    pst->dw.width[0][WLEFT] = pst->dw.width[0][WRIGHT] = 0;
    pst->dw.coef_ptrs[0] = cst_alloc(double,1);
    pst->dw.coef[0] = pst->dw.coef_ptrs[0];
    pst->dw.coef[0][0] = 1.0;

    // set delta coefficients
    for (i = 1; i < pst->dw.num; i++) {
        pst->dw.coef_ptrs[i] = cst_alloc(double,fsize);
	pst->dw.coef[i] = pst->dw.coef_ptrs[i];
        for (j=0; j<fsize; j++) /* FIXME make dynwin doubles for memmove */
            pst->dw.coef[i][j] = (double)dynwin[j];
	// set pointer
	leng = fsize / 2;			// L (fsize = 2 * L + 1)
	pst->dw.coef[i] += leng;		// [L] -> [0]	center
	pst->dw.width[i][WLEFT] = -leng;	// -L		left
	pst->dw.width[i][WRIGHT] = leng;	//  L		right
	if (fsize % 2 == 0) pst->dw.width[i][WRIGHT]--;
    }

    pst->dw.maxw[WLEFT] = pst->dw.maxw[WRIGHT] = 0;
    for (i = 0; i < pst->dw.num; i++) {
	if (pst->dw.maxw[WLEFT] > pst->dw.width[i][WLEFT])
	    pst->dw.maxw[WLEFT] = pst->dw.width[i][WLEFT];
	if (pst->dw.maxw[WRIGHT] < pst->dw.width[i][WRIGHT])
	    pst->dw.maxw[WRIGHT] = pst->dw.width[i][WRIGHT];
    }

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
    pst->vSize = (pst->order + 1) * pst->dw.num;	// odim = dim * (1--3)

    // memory allocation
    pst->T = T;					// number of frames
    pst->width = pst->dw.maxw[WRIGHT] * 2 + 1;	// width of R
    pst->mseq = bell_alloc_dmatrix(T,pst->vSize);	// [T][odim] 
    pst->ivseq = bell_alloc_dmatrix(T,pst->vSize);// [T][odim]
    pst->R = bell_alloc_dmatrix(T,pst->width);    // [T][width]
    pst->r = cst_alloc(double,T);		// [T]
    pst->g = cst_alloc(double,T);		// [T]
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
		n = i + k;
		if (n >= 0 && n < pst->T && pst->dw.coef[j][-k] != 0.0) {
		    l = j * (pst->order + 1) + m;
		    pst->r[i] += pst->dw.coef[j][-k] * pst->mseq[n][l]; 
		    wu = pst->dw.coef[j][-k] * pst->ivseq[n][l];
            
		    for (l = 0; l < pst->width; l++) {
			n = l-k;
			if (n <= pst->dw.width[j][1] && i + l < pst->T &&
			    pst->dw.coef[j][n] != 0.0)
			    pst->R[i][l] += wu * pst->dw.coef[j][n];
		    }
		}
	    }
	}
    }

    return;
}

// Choleski: Choleski factorization of Matrix R
static void Choleski(PStreamChol *pst)
{
    register int t, j, k;

    pst->R[0][0] = sqrt(pst->R[0][0]);

    for (j = 1; j < pst->width; j++) pst->R[0][j] /= pst->R[0][0];

    for (t = 1; t < pst->T; t++) {
	for (j = 1; j < pst->width; j++)
	    if (t - j >= 0)
		pst->R[t][0] -= pst->R[t - j][j] * pst->R[t - j][j];
         
	pst->R[t][0] = sqrt(pst->R[t][0]);
         
	for (j = 1; j < pst->width; j++) {
	    for (k = 0; k < pst->dw.maxw[WRIGHT]; k++)
		if (j != pst->width - 1)
		    pst->R[t][j] -=
			pst->R[t - k - 1][j - k] * pst->R[t - k - 1][j + 1];
            
	    pst->R[t][j] /= pst->R[t][0];
	}
    }
   
    return;
}

// Choleski_forward: forward substitution to solve linear equations
static void Choleski_forward(PStreamChol *pst)
{
    register int t, j;
    double hold;
   
    pst->g[0] = pst->r[0] / pst->R[0][0];

    for (t=1; t < pst->T; t++) {
	hold = 0.0;
	for (j = 1; j < pst->width; j++)
	    if (t - j >= 0 && pst->R[t - j][j] != 0.0)
		hold += pst->R[t - j][j] * pst->g[t - j];
	pst->g[t] = (pst->r[t] - hold) / pst->R[t][0];
    }
   
    return;
}

// Choleski_backward: backward substitution to solve linear equations
static void Choleski_backward(PStreamChol *pst, const int m)
{
    register int t, j;
    double hold;
   
    pst->c[pst->T - 1][m] = pst->g[pst->T - 1] / pst->R[pst->T - 1][0];

    for (t = pst->T - 2; t >= 0; t--) {
	hold = 0.0;
	for (j = 1; j < pst->width; j++)
	    if (t + j < pst->T && pst->R[t][j] != 0.0)
		hold += pst->R[t][j] * pst->c[t + j][m];
	pst->c[t][m] = (pst->g[t] - hold) / pst->R[t][0];
   }
   
   return;
}

// generate parameter sequence from pdf sequence using Choleski decomposition
static void mlpgChol(PStreamChol *pst)
{
   register int m;

   // generating parameter in each dimension
   for (m = 0; m <= pst->order; m++) {
       calc_R_and_r(pst, m);
       Choleski(pst);
       Choleski_forward(pst);
       Choleski_backward(pst, m);
   }
   
   return;
}

static void mlgparaChol(dmatrix pdf, PStreamChol *pst, dmatrix mlgp)
{
    int t, d;

    // error check
    if (pst->vSize * 2 != pdf->col || pst->order + 1 != mlgp->col) {
	cst_errmsg("Error mlgparaChol: Different dimension\n");
        cst_error();
    }

    // mseq: U^{-1}*M,	ifvseq: U^{-1}
    for (t = 0; t < pst->T; t++) {
	for (d = 0; d < pst->vSize; d++) {
	    pst->mseq[t][d] = pdf->data[t][d];
	    pst->ivseq[t][d] = pdf->data[t][pst->vSize + d];
	}
    } 

    // ML parameter generation
    mlpgChol(pst);

    // extracting parameters
    for (t = 0; t < pst->T; t++)
	for (d = 0; d <= pst->order; d++)
	    mlgp->data[t][d] = pst->c[t][d];

    return;
}

// diagonal covariance
static dvector xget_detvec_diamat2inv(dmatrix covmat)	// [num class][dim]
{
    long dim, clsnum;
    long i, j;
    double det;
    dvector detvec = NULL;

    clsnum = covmat->row;
    dim = covmat->col;
    // memory allocation
    detvec = xdvalloc(clsnum);
    for (i = 0; i < clsnum; i++) {
	for (j = 0, det = 1.0; j < dim; j++) {
	    det *= covmat->data[i][j];
	    if (det > 0.0) {
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

    for (i=0; i<pst->dw.num; i++)
        cst_free(pst->dw.width[i]);
    cst_free(pst->dw.width); pst->dw.width = NULL;
    for (i=0; i<pst->dw.num; i++)
        cst_free(pst->dw.coef_ptrs[i]);
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
    /* Generate an (mcep) track using Maximum Likelihood Parameter Generation */
    MLPGPARA param = NULL;
    cst_track *out;
    int dim, dim_st;
    //    float like;
    int i,j;
    int nframes;
    PStreamChol pst;

    nframes = param_track->num_frames;
    dim = (param_track->num_channels/2)-1;
    dim_st = dim/2; /* dim2 in original code */
    out = new_track();
    cst_track_resize(out,nframes,dim_st+1);

    param = xmlpgpara_init(dim,dim_st,nframes,nframes);

    // mixture-index sequence
    param->clsidxv = xlvalloc(nframes);
    for (i=0; i<nframes; i++)
        param->clsidxv->data[i] = i;

    // initial static feature sequence
    param->stm = xdmalloc(nframes,dim_st);
    for (i=0; i<nframes; i++)
    {
        for (j=0; j<dim_st; j++)
            param->stm->data[i][j] = param_track->frames[i][(j+1)*2];
    }

    /* Load cluster means */
    for (i=0; i<nframes; i++)
        for (j=0; j<dim_st; j++)
            param->mean->data[i][j] = param_track->frames[i][(j+1)*2];
    
    /* GMM parameters diagonal covariance */
    InitPStreamChol(&pst, cg_db->dynwin, cg_db->dynwinsize, dim_st-1, nframes);
    param->pdf = xdmalloc(nframes,dim*2);
    param->cov = xdmalloc(nframes,dim);
    for (i=0; i<nframes; i++)
        for (j=0; j<dim; j++)
            param->cov->data[i][j] = 
                param_track->frames[i][(j+1)*2+1] *
                param_track->frames[i][(j+1)*2+1];
    param->detvec = xget_detvec_diamat2inv(param->cov);

    /* global variance parameters */
    /* TBD get_gv_mlpgpara(param, vmfile, vvfile, dim2, msg_flag); */

    get_dltmat(param->stm, &pst.dw, 1, param->dltm);

    //like = 
    get_like_pdfseq_vit(dim, dim_st, nframes, param,
			param_track->frames, TRUE);

    /* vlike = get_like_gv(dim2, dnum, param); */

    mlgparaChol(param->pdf, &pst, param->stm);

    /* Put the answer back into the output track */
    for (i=0; i<nframes; i++)
    {
        out->times[i] = param_track->times[i];
        out->frames[i][0] = param_track->frames[i][0]; /* F0 */
        for (j=0; j<dim_st; j++)
            out->frames[i][j+1] = param->stm->data[i][j];
    }

    // memory free
    xmlpgparafree(param);
    pst_free(&pst);

    return out;
}
