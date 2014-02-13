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
/*    mlpg.c : speech parameter generation from pdf sequence         */
/*                                                                   */ 
/*                                    2003/06/11 by Heiga Zen        */
/*  ---------------------------------------------------------------  */

#include <math.h>
#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_file.h"
#include "cst_wave.h"
#include "HTS_misc.h"
#include "bell_file.h"

#include "nitech_hidden.h"
#define WLEFT 0
#define WRIGHT 1

/*----------------------------------------------------------------
	matrix calculation functions
----------------------------------------------------------------*/

/* calc_R_and_r : calculate R=W'U^{-1}W and r=W'U^{-1}M */
static void calc_R_and_r(PStream *pst, int m)
{
   register int i, j, k, l, n;
   double   wu;
   
   for (i=0; i<pst->T; i++) {
      pst->r[i] = pst->ivseq[i][m] * pst->mseq[i][m];
      pst->R[i][0] = pst->ivseq[i][m];

      for (j=1; j<pst->width; j++)
         pst->R[i][j]=0.0;

      for (j=1; j<pst->dw.num; j++)
         for (k=pst->dw.width[j][0]; k<=pst->dw.width[j][1]; k++) {
            n = i+k;
            if ( (n>=0) && (n<pst->T) && (pst->dw.coef[j][-k]!=0.0) ) {
               l = j*(pst->order+1)+m;
               wu = pst->dw.coef[j][-k] * pst->ivseq[n][l];
               pst->r[i] += wu*pst->mseq[n][l]; 
            
               for (l=0; l<pst->width; l++) {
                  n = l-k;
                  if (n<=pst->dw.width[j][1] && i+l<pst->T &&
                       pst->dw.coef[j][n] != 0.0)
                     pst->R[i][l] += wu * pst->dw.coef[j][n];
               }
            }
         }
   }

   return;
}

/* Cholesky : Cholesky factorization of Matrix R */
static void Cholesky(PStream *pst)
{
   register int i, j, k;
	
   pst->R[0][0] = sqrt(pst->R[0][0]);

   for (i=1; i<pst->width; i++)
      pst->R[0][i] /= pst->R[0][0];

   for (i=1; i<pst->T; i++) {
      for (j=1; j<pst->width; j++)
         if (i-j >= 0)
            pst->R[i][0] -= pst->R[i-j][j] * pst->R[i-j][j];
         
      pst->R[i][0] = sqrt(pst->R[i][0]);
         
      for (j=1; j<pst->width; j++) {
         for (k=0; k<pst->dw.max_L; k++)
            if (j!=pst->width-1)
               pst->R[i][j] -= pst->R[i-k-1][j-k]*pst->R[i-k-1][j+1];
            
         pst->R[i][j] /= pst->R[i][0];
      }
   }

   return;
}

/* Cholesky_forward : forward substitution to solve linear equations */
static void Cholesky_forward(PStream *pst)
{
   register int i, j;
   double hold;
   
   pst->g[0] = pst->r[0] / pst->R[0][0];

   for (i=1; i<pst->T; i++) {
      hold = 0.0;
      for (j=1; j<pst->width; j++) {
         if (i-j >= 0)
            hold += pst->R[i-j][j]*pst->g[i-j];
      }
      pst->g[i] = (pst->r[i]-hold)/pst->R[i][0];
   }

   return;
}

/* Cholesky_backward : backward substitution to solve linear equations */
static void Cholesky_backward(PStream *pst, int m)
{
   register int i, j;
   double hold;
   
   pst->c[pst->T-1][m] = pst->g[pst->T-1]/pst->R[pst->T-1][0];

   for (i=pst->T-2; i>=0; i--) {
      hold = 0.0;
      for (j=1; j<pst->width; j++) {
         if (pst->R[i][j] != 0.0)
            hold += pst->R[i][j]*pst->c[i+j][m];
      }
      pst->c[i][m] = (pst->g[i] - hold) / pst->R[i][0];
   }

   return;
}

/* generate parameter sequence from pdf sequence */
static void nitech_mlpg(PStream *pst)
{	
   int m;

   for (m=0; m<=pst->order; m++) {
      calc_R_and_r(pst,m);
      Cholesky(pst);
      Cholesky_forward(pst);
      Cholesky_backward(pst,m);
   }

   return;
}


/* InitPStream : Initialise PStream for parameter generation */
static void InitPStream(PStream *pst)
{
   pst->width	   = pst->dw.max_L*2+1;  /* band width of R */

   pst->mseq  = bell_alloc_dmatrix(pst->T, pst->vSize);
   pst->ivseq = bell_alloc_dmatrix(pst->T, pst->vSize);
   pst->g     = cst_alloc(double,pst->T);
   pst->R     = bell_alloc_dmatrix(pst->T, pst->width);
   pst->r     = cst_alloc(double,pst->T);
   pst->c     = bell_alloc_dmatrix(pst->T,pst->order+1);
}

/* FreePStream : Free PStream */
static void FreePStream(PStream *pst)
{
   register int t;
   
   for (t=0; t<pst->T; t++) {
      cst_free(pst->c[t]);
   }

   for (t=0; t<pst->dw.num; t++)
       cst_free(pst->dw.width[t]);
   cst_free(pst->dw.width);
	
   bell_free_dmatrix(pst->mseq,pst->T);
   bell_free_dmatrix(pst->ivseq,pst->T);
   bell_free_dmatrix(pst->R,pst->T);
   cst_free(pst->g);
   cst_free(pst->r);
   cst_free(pst->c);
}

/* InitDWin : Initialise dynamic window */
static void InitDWin(PStream *pst)
{   
   int i;
   int fsize, leng;

   /* memory allocation */
   pst->dw.width = cst_alloc(int *,pst->dw.num);
  
   for (i=0; i<pst->dw.num; i++) {
      pst->dw.width[i] = cst_alloc(int,2);
   }
   
   /* window for static parameter */
   pst->dw.width[0][WLEFT] = pst->dw.width[0][WRIGHT] = 0;

   /* set delta coefficients */
   for (i=1; i<pst->dw.num; i++)
   {
      /* All six nitech voices have 3 coefficients */
      fsize = 3;

      /* set pointer */
      leng = fsize / 2;
      pst->dw.width[i][WLEFT] = -leng;
      pst->dw.width[i][WRIGHT] = leng;
         
      if (fsize % 2 == 0)
         pst->dw.width[i][WRIGHT]--;
   }
 
   pst->dw.maxw[WLEFT] = pst->dw.maxw[WRIGHT] = 0;
      
   for (i=0; i<pst->dw.num; i++) {
      if (pst->dw.maxw[WLEFT] > pst->dw.width[i][WLEFT])
         pst->dw.maxw[WLEFT] = pst->dw.width[i][WLEFT];
      if (pst->dw.maxw[WRIGHT] < pst->dw.width[i][WRIGHT])
         pst->dw.maxw[WRIGHT] = pst->dw.width[i][WRIGHT];
   }

   /* calcurate max_L to determine size of band matrix */
   if ( pst->dw.maxw[WLEFT] >= pst->dw.maxw[WRIGHT] )
      pst->dw.max_L = pst->dw.maxw[WLEFT];
   else
      pst->dw.max_L = pst->dw.maxw[WRIGHT];

}

/* pdf2speech : parameter generation from pdf sequence */
cst_wave * pdf2speech(PStream *mceppst, PStream *lf0pst, nitechP *gp, ModelSet *ms, UttModel *um,
                         VocoderSetup *vs)
{
   cst_wave *w;
   int fperiod=80;
   int frame, mcepframe, lf0frame;
   int state, lw, rw, k, n;
   Model *m;
   bell_boolean nobound, *voiced;
   
   float f0;

   lf0pst->vSize = ms->lf0stream;
   lf0pst->order = 0;
   mceppst->vSize = ms->mcepvsize;
   mceppst->order = mceppst->vSize / mceppst->dw.num - 1;

   InitDWin(lf0pst);
   InitDWin(mceppst);

   mcepframe  = 0;
   lf0frame = 0;
 
   voiced = cst_alloc(bell_boolean,um->totalframe+1);
   
   for (m=um->mhead; m!=um->mtail ; m=m->next) {
      for (state=2; state<=ms->nstate+1; state++) {
         for (frame=1; frame<=m->dur[state]; frame++) {
            voiced[mcepframe++] = m->voiced[state];
            if (m->voiced[state]) {
               lf0frame++;
            }
         }
      }
   }
         
   mceppst->T = mcepframe;
   lf0pst->T  = lf0frame;

   w=new_wave();
   w->num_channels=1;
   CST_WAVE_SET_NUM_SAMPLES(w,mceppst->T*fperiod);
   CST_WAVE_SAMPLES(w) = cst_alloc(short,CST_WAVE_NUM_SAMPLES(w));
   CST_WAVE_SET_SAMPLE_RATE(w,vs->rate);
  
   InitPStream(mceppst);      
   InitPStream(lf0pst);

   mcepframe = 0;
   lf0frame  = 0;

   for (m=um->mhead; m!=um->mtail; m=m->next) {
      for (state=2; state<=ms->nstate+1; state++) {
         for (frame=1; frame<=m->dur[state]; frame++) {
            for (k=0; k<ms->mcepvsize; k++) {
               mceppst->mseq[mcepframe][k]  = m->mcepmean[state][k];
               mceppst->ivseq[mcepframe][k] = HTS_finv(m->mcepvariance[state][k]);
            }
            for (k=0; k<ms->lf0stream; k++) {
               lw = lf0pst->dw.width[k][WLEFT];
               rw = lf0pst->dw.width[k][WRIGHT];
               nobound = (bell_boolean)1;
               
               for (n=lw; n<=rw;n++)
                  if (mcepframe+n<0 || um->totalframe<mcepframe+n)
                     nobound = (bell_boolean)0;
                  else
		      nobound = (bell_boolean)((int)nobound & voiced[mcepframe+n]);
                  
               if (voiced[mcepframe]) {
                  lf0pst->mseq[lf0frame][k] = m->lf0mean[state][k+1];
                  if (nobound || k==0)
                     lf0pst->ivseq[lf0frame][k] = HTS_finv(m->lf0variance[state][k+1]);
                  else
                     lf0pst->ivseq[lf0frame][k] = 0.0;
               }
            }
            if (voiced[mcepframe])
               lf0frame++;
            mcepframe++;
         }
      }
   }

   nitech_mlpg(mceppst);
   
   if (lf0frame>0)
      nitech_mlpg(lf0pst);

   lf0frame = 0;
   
   for (mcepframe=0; mcepframe<mceppst->T; mcepframe++) { 
      if (voiced[mcepframe])
         f0 = exp(lf0pst->c[lf0frame++][0]);  
      else                  
         f0 = 0.0;

      vocoder(f0, mceppst->c[mcepframe], mceppst->order, w, mcepframe*fperiod, gp, vs);
   }

   FreePStream(mceppst);
   FreePStream(lf0pst);
   cst_free(voiced);
   return w;
}

void ReadWin(PStream *pst)
{
    int i;
    cst_file fp;
    int fsize, leng;

    pst->dw.coef= cst_alloc(float *,pst->dw.num);
    /* because the pointers are moved, keep an original of the memory
      being allocated */
    pst->dw.coefr= cst_alloc(float *,pst->dw.num);
    pst->dw.coef[0] = cst_alloc(float,1);
    pst->dw.coefr[0] = pst->dw.coef[0];
    pst->dw.coef[0][0] = 1;

    /* read delta coefficients */
    for (i=1; i<pst->dw.num; i++)
    {
       if ((fp = bell_fopen (pst->dw.fn[i],"r")) == NULL)
       {
          fprintf(stderr, "file %s not found\n", pst->dw.fn[i]);
          cst_error();
       }

       /* All six nitech voices have 3 coefficients */
       fsize = 3;

       /* read coefficients */
       pst->dw.coefr[i] = cst_alloc(float,fsize);
       bell_fread(pst->dw.coefr[i], sizeof(float), fsize, fp);
#ifdef WORDS_BIGENDIAN
       swap_bytes_float(pst->dw.coef[i],fsize);
#endif                          /* WORDS_BIGENDIAN */
       bell_fclose(fp);

       pst->dw.coef[i] = pst->dw.coefr[i];

       /* set pointer */
       leng = fsize / 2;
       pst->dw.coef[i] += leng;
    }  
}

void FreeWin(PStream *pst)
{
    int t;

    for (t=0; t<pst->dw.num; t++)
        cst_free(pst->dw.coefr[t]);
    cst_free(pst->dw.coefr);
    cst_free(pst->dw.coef);
}