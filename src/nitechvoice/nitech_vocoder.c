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
/*   vocoder.c : mel-cepstral vocoder                                */
/*              (pulse/noise excitation & MLSA filter)               */
/*                                                                   */ 
/*                                    2003/06/11 by Heiga Zen        */
/*  ---------------------------------------------------------------  */

#include <math.h>
#include "cst_alloc.h"

#include "nitech_hidden.h"

/* NOTE NOTE NOTE - For performance reasons we "include" static code not header here */
#include "../commonsynth/mlsafunc.c"

void init_vocoder(int m, VocoderSetup *vs)
{
   vs->fprd = 80;      // Nitech voices had frame period 80
   vs->iprd = 1;
   vs->seed = 1;
   vs->pd   = 4;       // Nitech voices had Pade order 4

   vs->next =1;

   /* Pade' approximants */
   vs->pade[ 0]=1.0;
   vs->pade[ 1]=1.0; vs->pade[ 2]=0.0;
   vs->pade[ 3]=1.0; vs->pade[ 4]=0.0;      vs->pade[ 5]=0.0;
   vs->pade[ 6]=1.0; vs->pade[ 7]=0.0;      vs->pade[ 8]=0.0;      vs->pade[ 9]=0.0;
   vs->pade[10]=1.0; vs->pade[11]=0.4999273; vs->pade[12]=0.1067005; vs->pade[13]=0.01170221; vs->pade[14]=0.0005656279;
   vs->pade[15]=1.0; vs->pade[16]=0.4999391; vs->pade[17]=0.1107098; vs->pade[18]=0.01369984; vs->pade[19]=0.0009564853;
   vs->pade[20]=0.00003041721;

   vs->rate=16000;    // Nitech voices had output sample rate of 16000Hz
                  
   vs->c = cst_alloc(double,3*(m+1)+3*(vs->pd+1)+vs->pd*(m+2));
   
   vs->p1 = -1;
   vs->sw = 0;        // init random number switch to unused state
}

void free_vocoder(VocoderSetup *vs)
{
   cst_free(vs->c);
}

void vocoder (double p, double *mc, int m, cst_wave *w, int samp_offset, nitechP *gp, VocoderSetup *vs)
{
   double inc, x;
   int i, j, k; 
   short xs;
   double a = gp->alpha;
   
   if (p!=0.0) 
      p = vs->rate / p;  /* f0 -> pitch */
   
   if (vs->p1 < 0) {
      if (vs->seed != 1) vs->next = srnd ((unsigned)vs->seed);

      vs->p1   = p;
      vs->pc   = vs->p1;
      vs->cc   = vs->c + m + 1;
      vs->cinc = vs->cc + m + 1;
      vs->d1   = vs->cinc + m + 1;
      
      mc2b(mc, vs->c, m, a);
      
      return;
   }

   mc2b(mc, vs->cc, m, a); 
   
   for (k=0; k<=m; k++)
      vs->cinc[k] = (vs->cc[k]-vs->c[k])*(double)vs->iprd/(double)vs->fprd;

   if (vs->p1!=0.0 && p!=0.0) {
      inc = (p-vs->p1)*(double)vs->iprd/(double)vs->fprd;
   }
   else {
      inc = 0.0;
      vs->pc = p;
      vs->p1 = 0.0;
   }

   for (j=vs->fprd, i=(vs->iprd+1)/2; j--;) {
      if (vs->p1 == 0.0) {
             x = nrandom(vs);
      }
      else {
          if ((vs->pc += 1.0)>=vs->p1) {
             x = sqrt (vs->p1);
             vs->pc = vs->pc - vs->p1;
          }
          else
              x = 0.0;
      }

      x *= exp(vs->c[0]);

      x = mlsadf(x, vs->c, m, a, vs->pd, vs->d1, vs);
      xs = (short) x;

      w->samples[samp_offset]=xs;
      samp_offset++;

      if (!--i) {
         vs->p1 += inc;
         for (k=0;k<=m;k++) vs->c[k] += vs->cinc[k];
         i = vs->iprd;
      }
   }
   
   vs->p1 = p;
   memmove(vs->cc,vs->c,sizeof(double)*(m+1));
}
