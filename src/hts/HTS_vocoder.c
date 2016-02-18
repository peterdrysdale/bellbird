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

// HTS voices use Pade approximate order 5. Set this for included static code.
#define BELL_PORDER 5

#include <math.h>               /* for sqrt(),log(),exp() */

#include "cst_alloc.h"
/* hts_engine libraries */
#include "HTS_hidden.h"

#ifndef LZERO
#define LZERO (-1.0e+10)        /* ~log(0) */
#endif                          /* !LZERO */

#ifndef PI
#define PI  3.14159265358979323846
#endif                          /* !PI */

#define IRLENG    576  // Interpolation length

static const double HTS_pade[6] = {
   1.00000000000,
   0.49993910000,
   0.11070980000,
   0.01369984000,
   0.00095648530,
   0.00003041721
};

/* HTS_movem: move memory */
static void HTS_movem(double *a, double *b, const int nitem)
{
   long i = (long) nitem;

   if (a > b)
      while (i--)
         *b++ = *a++;
   else {
      a += i;
      b += i;
      while (i--)
         *--b = *--a;
   }
}

// NOTE NOTE NOTE - For performance reasons we "include" static code not header here
#include "../commonsynth/mlsacore.c"

/* HTS_mlsadf: functions for MLSA filter */
static double HTS_mlsadf(double x, const double *c, const int m, const double a,
                         double *d1, int * pd2offset)
{
   x = mlsadf1(x, c, a, d1, HTS_pade);
   x = mlsadf2(x, c, m, a, &d1[2 * (BELL_PORDER + 1)], HTS_pade, pd2offset);

   return (x);
}

/* HTS_nrandom: functions for gaussian random noise generation */
static double HTS_nrandom(HTS_Vocoder * v)
{
   if (v->sw == 0) {
      v->sw = 1;
      do {
         v->r1 = 2 * rnd(&v->next) - 1;
         v->r2 = 2 * rnd(&v->next) - 1;
         v->s = v->r1 * v->r1 + v->r2 * v->r2;
      } while (v->s > 1 || v->s == 0);
      v->s = sqrt(-2 * log(v->s) / v->s);
      return (v->r1 * v->s);
   } else {
      v->sw = 0;
      return (v->r2 * v->s);
   }
}

/* HTS_b2mc: transform MLSA digital filter coefficients to mel-cepstrum */
static void HTS_b2mc(const double *b, double *mc, int m, const double a)
{
   double d, o;

   d = mc[m] = b[m];
   for (m--; m >= 0; m--) {
      o = b[m] + a * d;
      d = b[m];
      mc[m] = o;
   }
}

/* HTS_c2ir: The minimum phase impulse response is evaluated from the minimum phase cepstrum */
static void HTS_c2ir(const double *c, const int nc, double *ir)
{
   int n, k;
   double d;

   ir[0] = exp(c[0]);
   for (n = 1; n < nc; n++) {
      d = 0;
      for (k = 1; k <= n; k++)
         d += k * c[k] * ir[n - k];
      ir[n] = d / n;
   }
}

/* HTS_b2en: calculate frame energy */
static double HTS_b2en(HTS_Vocoder * v, const double *b, const size_t m, const double a)
{
   size_t i;
   double en = 0.0;
   double *cep;
   double *ir;

   if (v->spectrum2en_size < m) {
      if (v->spectrum2en_buff != NULL)
         cst_free(v->spectrum2en_buff);
      v->spectrum2en_buff = cst_alloc(double,((m + 1) + 2 * IRLENG));
      v->spectrum2en_size = m;
   }
   cep = v->spectrum2en_buff + m + 1;
   ir = cep + IRLENG;

   HTS_b2mc(b, v->spectrum2en_buff, m, a);
   freqt(v->spectrum2en_buff, m, cep, IRLENG, -a);
   HTS_c2ir(cep, IRLENG, ir);

   for (i = 0; i < IRLENG; i++)
      en += ir[i] * ir[i];

   return (en);
}

/* HTS_Vocoder_initialize_excitation: initialize excitation */
static void HTS_Vocoder_initialize_excitation(HTS_Vocoder * v, double pitch, size_t nlpf)
{
   size_t i;

   v->pitch_of_curr_point = pitch;
   v->pitch_counter = pitch;
   v->pitch_inc_per_point = 0.0;
   if (nlpf > 0) {
      v->excite_buff_size = nlpf;
      v->excite_ring_buff = cst_alloc(double,v->excite_buff_size);
      for (i = 0; i < v->excite_buff_size; i++)
         v->excite_ring_buff[i] = 0.0;
      v->excite_buff_index = 0;
   } else {
      v->excite_buff_size = 0;
      v->excite_ring_buff = NULL;
      v->excite_buff_index = 0;
   }
}

/* HTS_Vocoder_start_excitation: start excitation of each frame */
static void HTS_Vocoder_start_excitation(HTS_Vocoder * v, double pitch)
{
   if (v->pitch_of_curr_point != 0.0 && pitch != 0.0) {
      v->pitch_inc_per_point = (pitch - v->pitch_of_curr_point) / v->fprd;
   } else {
      v->pitch_inc_per_point = 0.0;
      v->pitch_of_curr_point = pitch;
      v->pitch_counter = pitch;
   }
}

/* HTS_Vocoder_excite_unvoiced_frame: ping noise to ring buffer */
static void HTS_Vocoder_excite_unvoiced_frame(HTS_Vocoder * v, double noise)
{
   size_t center = (v->excite_buff_size - 1) / 2;
   v->excite_ring_buff[(v->excite_buff_index + center) % v->excite_buff_size] += noise;
}

/* HTS_Vocoder_excite_vooiced_frame: ping noise and pulse to ring buffer */
static void HTS_Vocoder_excite_voiced_frame(HTS_Vocoder * v, double noise, double pulse, const double *lpf)
{
   size_t i;
   size_t center = (v->excite_buff_size - 1) / 2;

   if (noise != 0.0) {
      for (i = 0; i < v->excite_buff_size; i++) {
         if (i == center)
            v->excite_ring_buff[(v->excite_buff_index + i) % v->excite_buff_size] += noise * (1.0 - lpf[i]);
         else
            v->excite_ring_buff[(v->excite_buff_index + i) % v->excite_buff_size] += noise * (0.0 - lpf[i]);
      }
   }
   if (pulse != 0.0) {
      for (i = 0; i < v->excite_buff_size; i++)
         v->excite_ring_buff[(v->excite_buff_index + i) % v->excite_buff_size] += pulse * lpf[i];
   }
}

/* HTS_Vocoder_get_excitation: get excitation of each sample */
static double HTS_Vocoder_get_excitation(HTS_Vocoder * v, const double *lpf)
{
   double x;
   double noise, pulse = 0.0;

   if (v->excite_buff_size > 0) {
      noise = HTS_nrandom(v);
      pulse = 0.0;
      if (v->pitch_of_curr_point == 0.0) {
         HTS_Vocoder_excite_unvoiced_frame(v, noise);
      } else {
         v->pitch_counter += 1.0;
         if (v->pitch_counter >= v->pitch_of_curr_point) {
            pulse = sqrt(v->pitch_of_curr_point);
            v->pitch_counter -= v->pitch_of_curr_point;
         }
         HTS_Vocoder_excite_voiced_frame(v, noise, pulse, lpf);
         v->pitch_of_curr_point += v->pitch_inc_per_point;
      }
      x = v->excite_ring_buff[v->excite_buff_index];
      v->excite_ring_buff[v->excite_buff_index] = 0.0;
      v->excite_buff_index++;
      if (v->excite_buff_index >= v->excite_buff_size)
         v->excite_buff_index = 0;
   } else {
      if (v->pitch_of_curr_point == 0.0) {
         x = HTS_nrandom(v);
      } else {
         v->pitch_counter += 1.0;
         if (v->pitch_counter >= v->pitch_of_curr_point) {
            x = sqrt(v->pitch_of_curr_point);
            v->pitch_counter -= v->pitch_of_curr_point;
         } else {
            x = 0.0;
         }
         v->pitch_of_curr_point += v->pitch_inc_per_point;
      }
   }

   return x;
}

/* HTS_Vocoder_end_excitation: end excitation of each frame */
static void HTS_Vocoder_end_excitation(HTS_Vocoder * v, double pitch)
{
   v->pitch_of_curr_point = pitch;
}

/* HTS_Vocoder_postfilter_mcp: postfilter for MCP */
static void HTS_Vocoder_postfilter_mcp(HTS_Vocoder * v, double *mcp, const size_t m, double alpha, double beta)
{
   double e1, e2;
   size_t k;

   if (beta > 0.0 && m > 1) {
      if (v->postfilter_size < m) {
         if (v->postfilter_buff != NULL)
            cst_free(v->postfilter_buff);
         v->postfilter_buff = cst_alloc(double,m + 1);
         v->postfilter_size = m;
      }
      mc2b(mcp, v->postfilter_buff, m, alpha);
      e1 = HTS_b2en(v, v->postfilter_buff, m, alpha);

      v->postfilter_buff[1] -= beta * alpha * v->postfilter_buff[2];
      for (k = 2; k <= m; k++)
         v->postfilter_buff[k] *= (1.0 + beta);

      e2 = HTS_b2en(v, v->postfilter_buff, m, alpha);
      v->postfilter_buff[0] += log(e1 / e2) / 2;
      HTS_b2mc(v->postfilter_buff, mcp, m, alpha);
   }
}

/* HTS_Vocoder_initialize: initialize vocoder */
void HTS_Vocoder_initialize(HTS_Vocoder * v, size_t m, size_t rate, size_t fperiod)
{
   /* set parameter */
   v->is_first = TRUE;
   v->fprd = fperiod;
   v->next = 1;
   v->rate = rate;
   v->pitch_of_curr_point = 0.0;
   v->pitch_counter = 0.0;
   v->pitch_inc_per_point = 0.0;
   v->excite_ring_buff = NULL;
   v->excite_buff_size = 0;
   v->excite_buff_index = 0;
   v->sw = 0;
   /* init buffer */
   v->postfilter_buff = NULL;
   v->postfilter_size = 0;
   v->spectrum2en_buff = NULL;
   v->spectrum2en_size = 0;

   v->c = cst_alloc(double,(m * (3 + BELL_PORDER) + 7 * BELL_PORDER + 6));
   v->cc = v->c + m + 1;
   v->cinc = v->cc + m + 1;
   v->d1 = v->cinc + m + 1;
   v->d2offset = 1;
}

/* HTS_Vocoder_synthesize: pulse/noise excitation and MLSA/MGLSA filster based waveform synthesis */
void HTS_Vocoder_synthesize(HTS_Vocoder * v, size_t m, double lf0, double *spectrum, size_t nlpf, double *lpf, double alpha, double beta, double volume, int16_t *wavedata)
{
   double x;
   size_t i,j;
   int waveidx = 0;
   double p;

   /* lf0 -> pitch */
   if (lf0 == LZERO)
      p = 0.0;
   else if (lf0 <= MIN_LF0)
      p = v->rate / MIN_F0;
   else if (lf0 >= MAX_LF0)
      p = v->rate / MAX_F0;
   else
      p = v->rate / exp(lf0);

   /* first time */
   if (v->is_first == TRUE) {
      HTS_Vocoder_initialize_excitation(v, p, nlpf);
      mc2b(spectrum, v->c, m, alpha);
      v->is_first = FALSE;
   }

   HTS_Vocoder_start_excitation(v, p);
   HTS_Vocoder_postfilter_mcp(v, spectrum, m, alpha, beta);
   mc2b(spectrum, v->cc, m, alpha);
   for (i = 0; i <= m; i++)
      v->cinc[i] = (v->cc[i] - v->c[i]) / v->fprd;

   for (j = 0; j < v->fprd; j++) {
      x = HTS_Vocoder_get_excitation(v, lpf);
      if (x != 0.0)
         x *= exp(v->c[0]);
      x = HTS_mlsadf(x, v->c, m, alpha, v->d1, &(v->d2offset));
      x *= volume;

      /* output */
      if (wavedata)
         wavedata[waveidx++] = (int16_t) x;

      for (i = 0; i <= m; i++)
         v->c[i] += v->cinc[i];
   }

   HTS_Vocoder_end_excitation(v, p);
   HTS_movem(v->cc, v->c, m + 1);
}

/* HTS_Vocoder_clear: clear vocoder */
void HTS_Vocoder_clear(HTS_Vocoder * v)
{
   if (v != NULL) {
      /* free buffer */
      if (v->postfilter_buff != NULL) {
         cst_free(v->postfilter_buff);
         v->postfilter_buff = NULL;
      }
      v->postfilter_size = 0;
      if (v->spectrum2en_buff != NULL) {
         cst_free(v->spectrum2en_buff);
         v->spectrum2en_buff = NULL;
      }
      v->spectrum2en_size = 0;
      if (v->c != NULL) {
         cst_free(v->c);
         v->c = NULL;
      }
      v->excite_buff_size = 0;
      v->excite_buff_index = 0;
      if (v->excite_ring_buff != NULL) {
         cst_free(v->excite_ring_buff);
         v->excite_ring_buff = NULL;
      }
   }
}
