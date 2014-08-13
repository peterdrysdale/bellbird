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
/*-------------------------------------------------------------------*/

/* This source file is included in more than one compilation unit.      */
/* Ordinarily we might use normal functions but we use static functions */
/* as experience has showed us this code is highly performance critical */
/* and the compiler likes to inline and improve performance of these    */
/* functions if we declare them static.                                 */

static double mlsafir(const double x, const double *c, const int m, const double a, const double aa, double *d3, const int d2offset, const int offsetend1, const int offsetend2, const int offsetstart2, const int secelement)
{
// Filtering but without shuffling history terms.
// d2offset is index of start of history terms which
// are stored in a wrap around buffer.
// Elements d3[0] and d3[m+3] are 'ghost' edge elements
// to avoid conditionals in stencil (d3[i+1]-d3[i-1])
   double y = 0.0;
   int i;
   int j = 2; // index for MLSA filter coefficients

   d3[secelement] = aa*x + a*d3[secelement];
   for (i=d2offset+2; i<=offsetend1; i++,j++) {
      d3[i] += a*(d3[i+1]-d3[i-1]);
      y += d3[i]*c[j];
   }
   d3[0] = d3[m+2]; // Update 'ghost' edge element for stencil
   for (i=offsetstart2; i<=offsetend2; i++,j++) {
      d3[i] += a*(d3[i+1]-d3[i-1]);
      y += d3[i]*c[j];
   }

// Copy element which is not shuffled in usual implement. of this function
   d3[d2offset] = d3[secelement];

// Copy 'ghost' edge elements for stencil in next call of this function
   d3[0] = d3[m+2];
   d3[m+3] = d3[1];

   return(y);
}

static double mlsadf1(double x, const double *c, const double a,
                       const int pd, double *d1, const double *ppade)
{
   double v, out = 0.0, *pt, aa;
   int i;

   aa = 1 - a*a;
   pt = &d1[pd+1];

   for (i=pd; i>=1; i--) {
      d1[i] = aa*pt[i-1] + a*d1[i];
      pt[i] = d1[i] * c[1];
      v = pt[i] * ppade[i];
      x += (1 & i) ? v : -v;
      out += v;
   }

   pt[0] = x;
   out += x;

   return(out);
}

static double mlsadf2(double x, const double *c, const int m, const double a,
                       const int pd, double *d2, const double *ppade, int *pd2offset)
{
// This function and mlsafir() are modified to avoid memory copy
// operations.
// This improves performance at the cost of
// slightly more memory (a few hundred bytes).
   double v, out = 0.0, *pt;
   double aa;
   int i;
// default values for start and end of mlsafir loops
   int offsetend1 = m+2;   // end of first loop
   int offsetend2 = *pd2offset-2; // end of second loop
   int offsetstart2 = 1;   // start of second loop
   int secelement = *pd2offset + 1; // index of second element
// some special cases for loop lengths
   if (*pd2offset == 1) {
      offsetend1 = m+1;
   }
   if (*pd2offset == m+2) {
      offsetstart2 = 2;
      secelement = 1;
   }

   aa = 1 - a*a;
   pt = &d2[pd * (m+4)];

   for (i=pd; i>=1; i--) {
       pt[i] = mlsafir (pt[i-1], c, m, a, aa, &d2[(i-1)*(m+4)], *pd2offset, offsetend1, offsetend2, offsetstart2, secelement);

       v = pt[i] * ppade[i];

       x  += (1&i) ? v : -v;
       out += v;
   }
    
   pt[0] = x;
   out  += x;

// update index to start of history terms
   (*pd2offset)--;
   if (*pd2offset<1) *pd2offset = m+2;

   return(out);
}
