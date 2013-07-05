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
/*    model.c : read model and search pdf from models                */
/*                                                                   */ 
/*                                    2003/06/11 by Heiga Zen        */
/*  ---------------------------------------------------------------  */
#include "cst_alloc.h"
#include "cst_file.h"
#include "bell_file.h"

#include "nitech_hidden.h"

/* LoadModelFiles : load model files from files to pdf array */
void LoadModelFiles(ModelSet *ms)
{
   int i, j, k;

   /*-------------------- load pdfs for duration --------------------*/
   /* read the number of states & the number of pdfs (leaf nodes) */
   bell_fread(&ms->nstate,  sizeof(int), 1, ms->fp[DUR]);
#ifdef WORDS_BIGENDIAN
   ms->nstate = SWAPINT(ms->nstate);
#endif                          /* WORDS_BIGENDIAN */
   bell_fread(&ms->ndurpdf, sizeof(int), 1, ms->fp[DUR]);
#ifdef WORDS_BIGENDIAN
   ms->ndurpdf = SWAPINT(ms->ndurpdf);
#endif                          /* WORDS_BIGENDIAN */

   ms->durpdf = cst_alloc(float *,ms->ndurpdf+2);
   
   /* read pdfs (mean & variance) */
   for (i=1; i<=ms->ndurpdf; i++) {
      ms->durpdf[i] = cst_alloc(float,2*ms->nstate+2);
      bell_fread(ms->durpdf[i]+2, sizeof(float), 2*ms->nstate, ms->fp[DUR]);
#ifdef WORDS_BIGENDIAN
      swap_bytes_float(ms->durpdf[i]+2,2*ms->nstate);
#endif                          /* WORDS_BIGENDIAN */
   }

   /*-------------------- load pdfs for mcep --------------------*/
   /* read vector size for spectrum */
   bell_fread(&ms->mcepvsize, sizeof(int), 1, ms->fp[MCP]);
#ifdef WORDS_BIGENDIAN
   ms->mcepvsize = SWAPINT(ms->mcepvsize);
#endif                          /* WORDS_BIGENDIAN */
   ms->nmceppdf = cst_alloc(int,ms->nstate);

   /* read the number of pdfs for each state position */
   bell_fread(ms->nmceppdf, sizeof(int), ms->nstate, ms->fp[MCP]);
#ifdef WORDS_BIGENDIAN
   swap_bytes_int(ms->nmceppdf,ms->nstate);
#endif                          /* WORDS_BIGENDIAN */
   ms->mceppdf = cst_alloc(float **,ms->nstate+2);
   
   /* read pdfs (mean, variance) */
   for (i=2; i<=ms->nstate+1; i++) {
      ms->mceppdf[i] = cst_alloc(float *,ms->nmceppdf[i-2]+2);
      for (j=1; j<=ms->nmceppdf[i-2]; j++) {
         ms->mceppdf[i][j] = cst_alloc(float,ms->mcepvsize*2);
         bell_fread(ms->mceppdf[i][j], sizeof(float), ms->mcepvsize*2, ms->fp[MCP]);
#ifdef WORDS_BIGENDIAN
	 swap_bytes_float(ms->mceppdf[i][j],ms->mcepvsize*2);
#endif                          /* WORDS_BIGENDIAN */
      }
   } 

   /*-------------------- load pdfs for log F0 --------------------*/
   /* read the number of streams for f0 modeling */
   bell_fread(&ms->lf0stream, sizeof(int), 1, ms->fp[LF0]);
#ifdef WORDS_BIGENDIAN
   ms->lf0stream = SWAPINT(ms->lf0stream);
#endif                          /* WORDS_BIGENDIAN */
   ms->nlf0pdf = cst_alloc(int,ms->nstate+2);
   /* read the number of pdfs for each state position */
   bell_fread(ms->nlf0pdf, sizeof(int), ms->nstate, ms->fp[LF0]);
#ifdef WORDS_BIGENDIAN
   swap_bytes_int(ms->nlf0pdf,ms->nstate);
#endif                          /* WORDS_BIGENDIAN */
   ms->lf0pdf = cst_alloc(float ***,ms->nstate+3);
   
   /* read pdfs (mean, variance & weight) */
   for (i=2; i<=ms->nstate+1; i++) {
      ms->lf0pdf[i] = cst_alloc(float **,ms->nlf0pdf[i-2]+1);
      for (j=1; j<=ms->nlf0pdf[i-2]; j++) {
         ms->lf0pdf[i][j] = cst_alloc(float *,ms->lf0stream+1);
         for (k=1; k<=ms->lf0stream; k++) {
            ms->lf0pdf[i][j][k] = cst_alloc(float,4);
            bell_fread(ms->lf0pdf[i][j][k], sizeof(float), 4, ms->fp[LF0]);
#ifdef WORDS_BIGENDIAN
            swap_bytes_float(ms->lf0pdf[i][j][k],4);
#endif                          /* WORDS_BIGENDIAN */
         }
      }
   } 
}

/* FindDurPDF : find duration pdf from pdf array */
void FindDurPDF (Model *m, ModelSet *ms, float rho, int diffdur)
{
   float data, mean, variance;
   int s, idx; 

   idx = m->durpdf;

   m->dur = cst_alloc(int,ms->nstate+2);
   m->totaldur = 0;
   
   for (s=2; s<=ms->nstate+1; s++) {
      mean = ms->durpdf[idx][s];
      variance = ms->durpdf[idx][ms->nstate+s];
      data = mean + rho*variance;
      
      if (data < 0.0) data = 0.0;
         
      m->dur[s] = (int) (data+diffdur+0.5);
      m->totaldur += m->dur[s];
      diffdur += (int)(data-(float)m->dur[s]);
   }
}

/* FindLF0PDF : find required pdf for log F0 from pdf array */
void FindLF0PDF (int s, Model *m, ModelSet *ms, float uvthresh)
{
   int idx, stream;
   float *weight;

   idx = m->lf0pdf[s];

   if (m->lf0mean[s]) cst_free(m->lf0mean[s]);
   m->lf0mean[s]     = cst_alloc(float,ms->lf0stream+1);
   if (m->lf0variance[s]) cst_free(m->lf0variance[s]);
   m->lf0variance[s] = cst_alloc(float,ms->lf0stream+1);
   
   for (stream=1; stream<=ms->lf0stream; stream++) {
      m->lf0mean    [s][stream] = ms->lf0pdf[s][idx][stream][0];
      m->lf0variance[s][stream] = ms->lf0pdf[s][idx][stream][1];
      weight = ms->lf0pdf[s][idx][stream]+2;
      
      if (stream==1) {
         if (weight[0] > uvthresh)
            m->voiced[s] = 1;
         else
            m->voiced[s] = 0;
      }
   }
}

/* FindMcpPDF : find pdf for mel-cepstrum from pdf array */
void FindMcpPDF (int s, Model *m, ModelSet *ms)
{
   int idx;
   
   idx = m->mceppdf[s];

   m->mcepmean[s] = ms->mceppdf[s][idx];
   m->mcepvariance[s] = ms->mceppdf[s][idx]+ms->mcepvsize;
}

void InitModelSet (ModelSet *ms)
{
   ms->fp[DUR] = NULL;
   ms->fp[LF0] = NULL;
   ms->fp[MCP] = NULL;
   
   return;
} 

void DeleteModelSet(ModelSet *ms)
{
    int i,j,k;
    
    for (i=1; i<=ms->ndurpdf; i++)
	cst_free(ms->durpdf[i]);
    cst_free(ms->durpdf);

    for (i=2; i<=ms->nstate+1; i++)
    {
	for (j=1; j<=ms->nmceppdf[i-2]; j++)
	    cst_free(ms->mceppdf[i][j]);
	cst_free(ms->mceppdf[i]);
    }
    cst_free(ms->nmceppdf);
    cst_free(ms->mceppdf);

    for (i=2; i<=ms->nstate+1; i++)
    {
	for (j=1; j<=ms->nlf0pdf[i-2]; j++)
	{
	    for (k=1; k <=ms->lf0stream; k++)
		cst_free(ms->lf0pdf[i][j][k]);
	    cst_free(ms->lf0pdf[i][j]);
	}
	cst_free(ms->lf0pdf[i]);
    }
    cst_free(ms->nlf0pdf);
    cst_free(ms->lf0pdf);
}
