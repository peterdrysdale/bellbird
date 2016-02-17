/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/* ----------------------------------------------------------------- */
/*           The HMM-Based Speech Synthesis Engine "hts_engine API"  */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2001-2013  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/*                2001-2008  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the HTS working group nor the names of its  */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */
#include <math.h>               /* for sqrt() */
#include <limits.h>             // for LONG_MAX

#include "cst_alloc.h"
#include "cst_error.h"
/* hts_engine libraries */
#include "HTS_hidden.h"

// Parameters for global variance (GV)
#define STEPINIT 0.1
#define STEPDEC  0.5
#define STEPINC  1.2
#define W1       1.0
#define W2       1.0
#define GV_MAX_ITERATION 5

/* HTS_PStream_calc_wuw_and_wum: calculate W'U^{-1}W and W'U^{-1}M */
static void HTS_PStream_calc_wuw_and_wum(HTS_PStream * pst, size_t m)
{
   size_t i, j;
   long t, length_long;
   long shift;
   double wu;

// Safely cast pst->length to long so it can be used in signed index arithmetic with variable 'shift'
// The 'shift' algorithm design should be revisited when possible
   if (pst->length < LONG_MAX)
   {
       length_long = (long) pst->length;
   }
   else
   {
//  this condition should not be tripped in normal code operation since >2GB pstreams are unlikely/impossible in real speech
       cst_errmsg("calc_wuw_and_wum: pstream length exceeds algorithm design");
       cst_error();
   }

   for (t = 0; t < length_long; t++) {
      /* initialize */
      pst->sm.wum[t] = 0.0;
      for (i = 0; i < pst->width; i++)
         pst->sm.wuw[t][i] = 0.0;

      /* calc WUW & WUM */
      for (i = 0; i < pst->win_size; i++)
         for (shift = pst->win_l_width[i]; shift <= pst->win_r_width[i]; shift++)
            if ((t + shift >= 0) && (t + shift < length_long) && (pst->win_coefficient[i][-shift] != 0.0)) {
               wu = pst->win_coefficient[i][-shift] * pst->sm.ivar[t + shift][i * pst->vector_length + m];
               pst->sm.wum[t] += wu * pst->sm.mean[t + shift][i * pst->vector_length + m];
               for (j = 0; (j < pst->width) && (t + j < pst->length); j++)
                  if (((int) j <= pst->win_r_width[i] + shift) && (pst->win_coefficient[i][j - shift] != 0.0))
                     pst->sm.wuw[t][j] += wu * pst->win_coefficient[i][j - shift];
            }
   }
}


/* HTS_PStream_ldl_factorization: Factorize W'*U^{-1}*W to L*D*L' (L: lower triangular, D: diagonal) */
static void HTS_PStream_ldl_factorization(HTS_PStream * pst)
{
   size_t t, i, j;

   for (t = 0; t < pst->length; t++) {
      for (i = 1; (i < pst->width) && (t >= i); i++)
         pst->sm.wuw[t][0] -= pst->sm.wuw[t - i][i] * pst->sm.wuw[t - i][i] * pst->sm.wuw[t - i][0];

      for (i = 1; i < pst->width; i++) {
         for (j = 1; (i + j < pst->width) && (t >= j); j++)
            pst->sm.wuw[t][i] -= pst->sm.wuw[t - j][j] * pst->sm.wuw[t - j][i + j] * pst->sm.wuw[t - j][0];
         pst->sm.wuw[t][i] /= pst->sm.wuw[t][0];
      }
   }
}

/* HTS_PStream_forward_substitution: forward subtitution for mlpg */
static void HTS_PStream_forward_substitution(HTS_PStream * pst)
{
   size_t t, i;

   for (t = 0; t < pst->length; t++) {
      pst->sm.g[t] = pst->sm.wum[t];
      for (i = 1; (i < pst->width) && (t >= i); i++)
         pst->sm.g[t] -= pst->sm.wuw[t - i][i] * pst->sm.g[t - i];
   }
}

/* HTS_PStream_backward_substitution: backward subtitution for mlpg */
static void HTS_PStream_backward_substitution(HTS_PStream * pst, size_t m)
{
   size_t rev, t, i;

   for (rev = 0; rev < pst->length; rev++) {
      t = pst->length - 1 - rev;
      pst->par[t][m] = pst->sm.g[t] / pst->sm.wuw[t][0];
      for (i = 1; (i < pst->width) && (t + i < pst->length); i++)
         pst->par[t][m] -= pst->sm.wuw[t][i] * pst->par[t + i][m];
   }
}

/* HTS_PStream_calc_gv: subfunction for mlpg using GV */
static void HTS_PStream_calc_gv(HTS_PStream * pst, size_t m, double *mean, double *vari)
{
   size_t t;

   *mean = 0.0;
   for (t = 0; t < pst->length; t++)
      if (pst->gv_switch[t])
         *mean += pst->par[t][m];
   *mean /= pst->gv_length;
   *vari = 0.0;
   for (t = 0; t < pst->length; t++)
      if (pst->gv_switch[t])
         *vari += (pst->par[t][m] - *mean) * (pst->par[t][m] - *mean);
   *vari /= pst->gv_length;
}

/* HTS_PStream_conv_gv: subfunction for mlpg using GV */
static void HTS_PStream_conv_gv(HTS_PStream * pst, size_t m)
{
   size_t t;
   double ratio;
   double mean;
   double vari;

   HTS_PStream_calc_gv(pst, m, &mean, &vari);
   ratio = sqrt(pst->gv_mean[m] / vari);
   for (t = 0; t < pst->length; t++)
      if (pst->gv_switch[t])
         pst->par[t][m] = ratio * (pst->par[t][m] - mean) + mean;
}

/* HTS_PStream_calc_derivative: subfunction for mlpg using GV */
static double HTS_PStream_calc_derivative(HTS_PStream * pst, size_t m)
{
   size_t t, i;
   double mean;
   double vari;
   double dv;
   double h;
   double gvobj;
   double hmmobj;
   double w = 1.0 / (pst->win_size * pst->length);

   HTS_PStream_calc_gv(pst, m, &mean, &vari);
   gvobj = -0.5 * W2 * vari * pst->gv_vari[m] * (vari - 2.0 * pst->gv_mean[m]);
   dv = -2.0 * pst->gv_vari[m] * (vari - pst->gv_mean[m]) / pst->length;

   for (t = 0; t < pst->length; t++) {
      pst->sm.g[t] = pst->sm.wuw[t][0] * pst->par[t][m];
      for (i = 1; i < pst->width; i++) {
         if (t + i < pst->length)
            pst->sm.g[t] += pst->sm.wuw[t][i] * pst->par[t + i][m];
         if (t + 1 > i)
            pst->sm.g[t] += pst->sm.wuw[t - i][i] * pst->par[t - i][m];
      }
   }

   for (t = 0, hmmobj = 0.0; t < pst->length; t++) {
      hmmobj += W1 * w * pst->par[t][m] * (pst->sm.wum[t] - 0.5 * pst->sm.g[t]);
      h = -W1 * w * pst->sm.wuw[t][1 - 1] - W2 * 2.0 / (pst->length * pst->length) * ((pst->length - 1) * pst->gv_vari[m] * (vari - pst->gv_mean[m]) + 2.0 * pst->gv_vari[m] * (pst->par[t][m] - mean) * (pst->par[t][m] - mean));
      if (pst->gv_switch[t])
         pst->sm.g[t] = 1.0 / h * (W1 * w * (-pst->sm.g[t] + pst->sm.wum[t]) + W2 * dv * (pst->par[t][m] - mean));
      else
         pst->sm.g[t] = 1.0 / h * (W1 * w * (-pst->sm.g[t] + pst->sm.wum[t]));
   }

   return (-(hmmobj + gvobj));
}

/* HTS_PStream_gv_parmgen: function for mlpg using GV */
static void HTS_PStream_gv_parmgen(HTS_PStream * pst, size_t m)
{
   size_t t, i;
   double step = STEPINIT;
   double prev = 0.0;
   double obj;

   if (pst->gv_length == 0)
      return;

   HTS_PStream_conv_gv(pst, m);
   if (GV_MAX_ITERATION > 0) {
      HTS_PStream_calc_wuw_and_wum(pst, m);
      for (i = 1; i <= GV_MAX_ITERATION; i++) {
         obj = HTS_PStream_calc_derivative(pst, m);
         if (i > 1) {
            if (obj > prev)
               step *= STEPDEC;
            if (obj < prev)
               step *= STEPINC;
         }
         for (t = 0; t < pst->length; t++)
            pst->par[t][m] += step * pst->sm.g[t];
         prev = obj;
      }
   }
}

/* HTS_PStream_mlpg: generate sequence of speech parameter vector maximizing its output probability for given pdf sequence */
static void HTS_PStream_mlpg(HTS_PStream * pst)
{
   size_t m;

   if (pst->length == 0)
      return;

   for (m = 0; m < pst->vector_length; m++) {
      HTS_PStream_calc_wuw_and_wum(pst, m);
      HTS_PStream_ldl_factorization(pst);       /* LDL factorization */
      HTS_PStream_forward_substitution(pst);    /* forward substitution   */
      HTS_PStream_backward_substitution(pst, m);        /* backward substitution  */
      if (pst->gv_length > 0)
         HTS_PStream_gv_parmgen(pst, m);
   }
}

/* HTS_PStreamSet_initialize: initialize parameter stream set */
void HTS_PStreamSet_initialize(HTS_PStreamSet * pss)
{
   pss->pstream = NULL;
   pss->nstream = 0;
   pss->total_frame = 0;
}

/* HTS_PStreamSet_create: parameter generation using GV weight */
HTS_Boolean HTS_PStreamSet_create(HTS_PStreamSet * pss, HTS_SStreamSet * sss, double *msd_threshold, double *gv_weight)
{
   size_t i, j, k, l, m;
   int shift;
   long frame_long;         // Long frame count for using with shift
   long total_frame_long;   // Long of total_frame for comparison with 'frame_long + shift'
   size_t frame, msd_frame, state;

   HTS_PStream *pst;
   HTS_Boolean not_bound;

   if (pss->nstream != 0) {
      cst_errmsg("HTS_PstreamSet_create: HTS_PStreamSet should be clear.\n");
      cst_error();
      return FALSE;
   }

   /* initialize */
   pss->nstream = HTS_SStreamSet_get_nstream(sss);
   pss->pstream = cst_alloc(HTS_PStream,pss->nstream);
   pss->total_frame = HTS_SStreamSet_get_total_frame(sss);

// Safely cast total_frame to long so that index arithmetic operates correctly with 'shift' variable
// The 'shift' algorithm design should be revisited when possible
   if (pss->total_frame < LONG_MAX)
   {
       total_frame_long = (long) pss->total_frame;
   }
   else
   {
//  this condition should not be tripped in normal code operation since >2GB pstreams are unlikely in real speech
       cst_errmsg("HTS_PStreamSet_create: total_frame exceeds algorithm design limit");
       cst_error();
   }

   /* create */
   for (i = 0; i < pss->nstream; i++) {
      pst = &pss->pstream[i];
      if (HTS_SStreamSet_is_msd(sss, i)) {      /* for MSD */
         pst->length = 0;
         for (state = 0; state < HTS_SStreamSet_get_total_state(sss); state++)
            if (HTS_SStreamSet_get_msd(sss, i, state) > msd_threshold[i])
               pst->length += HTS_SStreamSet_get_duration(sss, state);
         pst->msd_flag = cst_alloc(HTS_Boolean,pss->total_frame);
         for (state = 0, frame = 0; state < HTS_SStreamSet_get_total_state(sss); state++)
            if (HTS_SStreamSet_get_msd(sss, i, state) > msd_threshold[i])
               for (j = 0; j < HTS_SStreamSet_get_duration(sss, state); j++) {
                  pst->msd_flag[frame] = TRUE;
                  frame++;
            } else
               for (j = 0; j < HTS_SStreamSet_get_duration(sss, state); j++) {
                  pst->msd_flag[frame] = FALSE;
                  frame++;
               }
      } else {                  /* for non MSD */
         pst->length = pss->total_frame;
         pst->msd_flag = NULL;
      }
      pst->vector_length = HTS_SStreamSet_get_vector_length(sss, i);
      pst->width = HTS_SStreamSet_get_window_max_width(sss, i) * 2 + 1; /* band width of R */
      pst->win_size = HTS_SStreamSet_get_window_size(sss, i);
      pst->sm.mean = bell_alloc_dmatrix(pst->length, pst->vector_length * pst->win_size);
      pst->sm.ivar = bell_alloc_dmatrix(pst->length, pst->vector_length * pst->win_size);
      pst->sm.wum = cst_alloc(double,pst->length);
      pst->sm.wuw = bell_alloc_dmatrix(pst->length, pst->width);
      pst->sm.g = cst_alloc(double,pst->length);
      pst->par = bell_alloc_dmatrix(pst->length, pst->vector_length);
      /* copy dynamic window */
      pst->win_l_width = cst_alloc(int,pst->win_size);
      pst->win_r_width = cst_alloc(int,pst->win_size);
      pst->win_coefficient = cst_alloc(double *,pst->win_size);
      for (j = 0; j < pst->win_size; j++) {
         pst->win_l_width[j] = HTS_SStreamSet_get_window_left_width(sss, i, j);
         pst->win_r_width[j] = HTS_SStreamSet_get_window_right_width(sss, i, j);
         if (pst->win_l_width[j] + pst->win_r_width[j] == 0)
            pst->win_coefficient[j] = cst_alloc(double,(-2 * pst->win_l_width[j] + 1));
         else
            pst->win_coefficient[j] = cst_alloc(double,(-2 * pst->win_l_width[j]));
         pst->win_coefficient[j] -= pst->win_l_width[j];
         for (shift = pst->win_l_width[j]; shift <= pst->win_r_width[j]; shift++)
            pst->win_coefficient[j][shift] = HTS_SStreamSet_get_window_coefficient(sss, i, j, shift);
      }
      /* copy GV */
      if (HTS_SStreamSet_use_gv(sss, i)) {
         pst->gv_mean = cst_alloc(double,pst->vector_length);
         pst->gv_vari = cst_alloc(double,pst->vector_length);
         for (j = 0; j < pst->vector_length; j++) {
            pst->gv_mean[j] = HTS_SStreamSet_get_gv_mean(sss, i, j) * gv_weight[i];
            pst->gv_vari[j] = HTS_SStreamSet_get_gv_vari(sss, i, j);
         }
         pst->gv_switch = cst_alloc(HTS_Boolean,pst->length);
         if (HTS_SStreamSet_is_msd(sss, i)) {   /* for MSD */
            for (state = 0, frame = 0, msd_frame = 0; state < HTS_SStreamSet_get_total_state(sss); state++)
               for (j = 0; j < HTS_SStreamSet_get_duration(sss, state); j++, frame++)
                  if (pst->msd_flag[frame])
                     pst->gv_switch[msd_frame++] = HTS_SStreamSet_get_gv_switch(sss, i, state);
         } else {               /* for non MSD */
            for (state = 0, frame = 0; state < HTS_SStreamSet_get_total_state(sss); state++)
               for (j = 0; j < HTS_SStreamSet_get_duration(sss, state); j++)
                  pst->gv_switch[frame++] = HTS_SStreamSet_get_gv_switch(sss, i, state);
         }
         for (j = 0, pst->gv_length = 0; j < pst->length; j++)
            if (pst->gv_switch[j])
               pst->gv_length++;
      } else {
         pst->gv_switch = NULL;
         pst->gv_length = 0;
         pst->gv_mean = NULL;
         pst->gv_vari = NULL;
      }
      /* copy pdfs */
      if (HTS_SStreamSet_is_msd(sss, i)) {      /* for MSD */
         for (state = 0, frame_long = 0, msd_frame = 0; state < HTS_SStreamSet_get_total_state(sss); state++) {
            for (j = 0; j < HTS_SStreamSet_get_duration(sss, state); j++) {
               if (pst->msd_flag[frame_long]) {
                  /* check current frame is MSD boundary or not */
                  for (k = 0; k < pst->win_size; k++) {
                     not_bound = TRUE;
                     for (shift = pst->win_l_width[k]; shift <= pst->win_r_width[k]; shift++)
                        if (frame_long + shift < 0 || total_frame_long <= frame_long + shift ||
                             !pst->msd_flag[frame_long + shift]) {
                           not_bound = FALSE;
                           break;
                        }
                     for (l = 0; l < pst->vector_length; l++) {
                        m = pst->vector_length * k + l;
                        pst->sm.mean[msd_frame][m] = HTS_SStreamSet_get_mean(sss, i, state, m);
                        if (not_bound || k == 0)
                           pst->sm.ivar[msd_frame][m] = HTS_finv(HTS_SStreamSet_get_vari(sss, i, state, m));
                        else
                           pst->sm.ivar[msd_frame][m] = 0.0;
                     }
                  }
                  msd_frame++;
               }
               frame_long++;
            }
         }
      } else {                  /* for non MSD */
         for (state = 0, frame_long = 0; state < HTS_SStreamSet_get_total_state(sss); state++) {
            for (j = 0; j < HTS_SStreamSet_get_duration(sss, state); j++) {
               for (k = 0; k < pst->win_size; k++) {
                  not_bound = TRUE;
                  for (shift = pst->win_l_width[k]; shift <= pst->win_r_width[k]; shift++)
                     if (frame_long + shift < 0 || total_frame_long <= frame_long + shift) {
                        not_bound = FALSE;
                        break;
                     }
                  for (l = 0; l < pst->vector_length; l++) {
                     m = pst->vector_length * k + l;
                     pst->sm.mean[frame_long][m] = HTS_SStreamSet_get_mean(sss, i, state, m);
                     if (not_bound || k == 0)
                        pst->sm.ivar[frame_long][m] = HTS_finv(HTS_SStreamSet_get_vari(sss, i, state, m));
                     else
                        pst->sm.ivar[frame_long][m] = 0.0;
                  }
               }
               frame_long++;
            }
         }
      }
      /* parameter generation */
      HTS_PStream_mlpg(pst);
   }

   return TRUE;
}

/* HTS_PStreamSet_get_nstream: get number of stream */
size_t HTS_PStreamSet_get_nstream(HTS_PStreamSet * pss)
{
   return pss->nstream;
}

/* HTS_PStreamSet_get_vector_length: get feature length */
size_t HTS_PStreamSet_get_vector_length(HTS_PStreamSet * pss, size_t stream_index)
{
   return pss->pstream[stream_index].vector_length;
}

/* HTS_PStreamSet_get_total_frame: get total number of frame */
size_t HTS_PStreamSet_get_total_frame(HTS_PStreamSet * pss)
{
   return pss->total_frame;
}

/* HTS_PStreamSet_get_parameter: get parameter */
double HTS_PStreamSet_get_parameter(HTS_PStreamSet * pss, size_t stream_index, size_t frame_index, size_t vector_index)
{
   return pss->pstream[stream_index].par[frame_index][vector_index];
}

/* HTS_PStreamSet_get_msd_flag: get generated MSD flag per frame */
HTS_Boolean HTS_PStreamSet_get_msd_flag(HTS_PStreamSet * pss, size_t stream_index, size_t frame_index)
{
   return pss->pstream[stream_index].msd_flag[frame_index];
}

/* HTS_PStreamSet_is_msd: get MSD flag */
HTS_Boolean HTS_PStreamSet_is_msd(HTS_PStreamSet * pss, size_t stream_index)
{
   return pss->pstream[stream_index].msd_flag ? TRUE : FALSE;
}

/* HTS_PStreamSet_clear: free parameter stream set */
void HTS_PStreamSet_clear(HTS_PStreamSet * pss)
{
   size_t i, j;
   HTS_PStream *pstream;

   if (pss->pstream) {
      for (i = 0; i < pss->nstream; i++) {
         pstream = &pss->pstream[i];
         cst_free(pstream->sm.wum);
         cst_free(pstream->sm.g);
         bell_free_dmatrix(pstream->sm.wuw);
         bell_free_dmatrix(pstream->sm.ivar);
         bell_free_dmatrix(pstream->sm.mean);
         bell_free_dmatrix(pstream->par);
         if (pstream->msd_flag)
            cst_free(pstream->msd_flag);
         if (pstream->win_coefficient) {
            for (j = 0; j < pstream->win_size; j++) {
               pstream->win_coefficient[j] += pstream->win_l_width[j];
               cst_free(pstream->win_coefficient[j]);
            }
         }
         if (pstream->gv_mean)
            cst_free(pstream->gv_mean);
         if (pstream->gv_vari)
            cst_free(pstream->gv_vari);
         cst_free(pstream->win_coefficient);
         cst_free(pstream->win_l_width);
         cst_free(pstream->win_r_width);
         if (pstream->gv_switch)
            cst_free(pstream->gv_switch);
      }
      cst_free(pss->pstream);
   }
   HTS_PStreamSet_initialize(pss);
}
