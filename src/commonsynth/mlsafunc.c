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

static double mlsadf(double x, const double *c, const int m, const double a,
                      double *d1, VocoderSetup *vs)
{
// the mel log spectrum approximation (MLSA) digital filter
// x : input
// c : MLSA filter coefficients
// m : order of cepstrum
// a : alpha, the all-pass constant
// d1: working memory - history terms
    
   x = mlsadf1 (x, c, a, d1, vs->pade);
   x = mlsadf2 (x, c, m, a, &d1[2*(BELL_PORDER+1)], vs->pade, &(vs->d2offset));

   return(x);
}

static unsigned long srnd( unsigned long seed )
{
   return(seed);
}

static double nrandom(VocoderSetup *vs)
{
// Gaussian random number generator throwing numbers in pairs with switch
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
