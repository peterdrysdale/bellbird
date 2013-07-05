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
/*  This is originally part of Heiga Zen's mlsa code.                */
/*  It was modified by Toda and Black eventually ending up in Flite. */
/*  Bellbird extracts these routines for common use between          */
/*  different voice types. For performance reasons we statically     */
/*  link each voice type with a copy of this library (sorry).        */
/*-------------------------------------------------------------------*/

/* This source file is included in more than one compliation unit.      */
/* Ordinarily we might use normal functions but we use static functions */
/* as experience has showed us this code is highly performance critical */
/* and the compiler likes to inline and improve performance of these    */
/* functions if we declare them static.                                 */

#define RANDMAX 32767

static double mlsafir (double x, double *b, int m, double a, double *d)
{  
   double y = 0.0;
   double aa;
   register int i;

   aa = 1 - a*a;

   d[0] = x;
   d[1] = aa*d[0] + a*d[1];
   for (i=2; i<= m; i++) {
      d[i] = d[i] + a*(d[i+1]-d[i-1]);
      y += d[i]*b[i];
   }

   for (i=m+1; i>1; i--) 
      d[i] = d[i-1];

   return(y);
}

static double mlsadf1(double x, double *b, int m, double a, int pd, double *d, VocoderSetup *vs)
{
   double v, out = 0.0, *pt, aa;
   register int i;

   aa = 1 - a*a;
   pt = &d[pd+1];

   for (i=pd; i>=1; i--) {
      d[i] = aa*pt[i-1] + a*d[i];
      pt[i] = d[i] * b[1];
      v = pt[i] * vs->ppade[i];
      x += (1 & i) ? v : -v;
      out += v;
   }

   pt[0] = x;
   out += x;

   return(out);
}

static double mlsadf2 (double x, double *b, int m, double a, int pd, double *d, VocoderSetup *vs)
{
  double v, out = 0.0, *pt;
  //  double aa;
  register int i;
    
  //aa = 1 - a*a;
   pt = &d[pd * (m+2)];

   for (i=pd; i>=1; i--) {
       pt[i] = mlsafir (pt[i-1], b, m, a, &d[(i-1)*(m+2)]);

       v = pt[i] * vs->ppade[i];

       x  += (1&i) ? v : -v;
       out += v;
   }
    
   pt[0] = x;
   out  += x;

   return(out);
}

static double mlsadf(double x, double *b, int m, double a, int pd, double *d, VocoderSetup *vs)
{

   vs->ppade = &(vs->pade[pd*(pd+1)/2]);
    
   x = mlsadf1 (x, b, m, a, pd, d, vs);
   x = mlsadf2 (x, b, m, a, pd, &d[2*(pd+1)], vs);

   return(x);
}

static double rnd (unsigned long *next)
{
   double r;

   *next = *next * 1103515245L + 12345;
   r = (*next / 65536L) % 32768L;

   return(r/RANDMAX); 
}

static unsigned long srnd ( unsigned long seed )
{
   return(seed);
}

static double nrandom (VocoderSetup *vs)
{
   if (vs->sw == 0) {
      vs->sw = 1;
      do {
         vs->r1 = 2.0 * rnd(&vs->next) - 1.0;
         vs->r2 = 2.0 * rnd(&vs->next) - 1.0;
         vs->s  = vs->r1 * vs->r1 + vs->r2 * vs->r2;
      } while (vs->s > 1 || vs->s == 0);

      vs->s = sqrt (-2 * log(vs->s) / vs->s);
      
      return(vs->r1*vs->s);
   }
   else {
      vs->sw = 0;
      
      return (vs->r2*vs->s);
   }
}