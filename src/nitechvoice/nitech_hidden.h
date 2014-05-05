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

#ifndef __NITECH_HIDDEN_H__
#define __NITECH_HIDDEN_H__

#include "cst_wave.h"
#include "nitech_engine.h"

typedef struct _nitechP { // parameters of nitech_engine
   float   rho;        // variable for speaking rate control
   float   alpha;      // all-pass constant - a frequency warping parameter
   float   uv;         // U/V threshold which determines voiced/unvoiced status
} nitechP;

typedef enum {DUR, LF0, MCP} Mtype;

typedef struct _Model {  // Hidden Markov Model (HMM) handler
   char *name;            // the name of this HMM
   int durpdf;            // duration pdf index for this HMM
   int *lf0pdf;           // log(f0) pdf indexes for each state
   int *mceppdf;          // mel-cepstrum pdf indexes for each state
   int *dur;              // duration for each state
   int totaldur;          // total duration of this HMM
   float **lf0mean;       // mean vector of log f0 pdfs for each state
   float **lf0variance;   // variance (diag) elements of log f0 for each state
   float **mcepmean;      // mean vector of mel-cepstrum pdfs for each state
   float **mcepvariance;  // variance (diag) elements of mel-cepstrum for each state
   bell_boolean *voiced;  // voiced/unvoiced status for each state
   struct _Model *next;   // pointer to next HMM
} Model; 

typedef struct _UttModel { // Utterance model handler
   Model *mhead;          // pointer to head HMM
   Model *mtail;          // pointer to tail HMM
   int nModel;            // number of finished HMM
   int nState;
   int totalframe;
} UttModel;

void LoadModelFiles (ModelSet *);
void FindDurPDF (Model *, ModelSet *, float, int );
void FindLF0PDF (int, Model *, ModelSet *, float);
void FindMcpPDF (int, Model *, ModelSet *);
ModelSet * InitModelSet (void);
void DeleteModelSet(ModelSet *ms);

void LoadTreesFile (TreeSet *, Mtype);
int SearchTree (char *, Node *);
TreeSet * InitTreeSet(void);
void nitech_free_treeset(TreeSet *ts);

void init_vocoder(int m, VocoderSetup *vs);
void vocoder (double p, double *mc, int m, cst_wave *w, int samp_offset, nitechP *gp, VocoderSetup *vs);
void free_vocoder(VocoderSetup *vs);

cst_wave * pdf2speech(PStreamChol *, PStreamChol *, nitechP *, ModelSet *, UttModel *, VocoderSetup *);
void ReadWin(PStreamChol *);
void FreeWin(PStreamChol *);

#endif
