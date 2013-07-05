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

#include "cst_file.h"
#include "cst_wave.h"

typedef struct _globalP {  
   float   RHO      ;  /* variable for speaking rate control         */   
   float   ALPHA    ;  /* variable for frequency warping parameter   */
   float   UV       ;  /* variable for U/V threshold                 */
} globalP;

typedef enum {DUR, LF0, MCP} Mtype;

typedef struct _Model {  /* HMM handler */
   char *name;            /* the name of this HMM */
   int durpdf;            /* duration pdf index for this HMM */
   int *lf0pdf;           /* mel-cepstrum pdf indexes for each state of this HMM */  
   int *mceppdf;          /* log f0 pdf indexes for each state of this HMM */
   int *dur;              /* duration for each state of this HMM */
   int totaldur;          /* total duration of this HMM */
   float **lf0mean;       /* mean vector of log f0 pdfs for each state of this HMM */
   float **lf0variance;   /* variance (diag) elements of log f0 for each state of this HMM */
   float **mcepmean;      /* mean vector of mel-cepstrum pdfs for each state of this HMM */
   float **mcepvariance;  /* variance (diag) elements of mel-cepstrum for each state of this HMM */
   bell_boolean *voiced;       /* voiced/unvoiced decision for each state of this HMM */
   struct _Model *next;   /* pointer to next HMM */
} Model; 

typedef struct _UttModel { /* Utterance model handler */
   Model *mhead;
   Model *mtail;
   int nModel;
   int nState;
   int totalframe;
} UttModel;

typedef struct _ModelSet { /* HMM set handler */
   int nstate;
   int lf0stream;
   int mcepvsize;
   int ndurpdf;
   int *nmceppdf;
   int *nlf0pdf;
   float **durpdf,***mceppdf,****lf0pdf;
   cst_file fp[3];
} ModelSet;

void LoadModelFiles (ModelSet *);
void FindDurPDF (Model *, ModelSet *, float, int );
void FindLF0PDF (int, Model *, ModelSet *, float);
void FindMcpPDF (int, Model *, ModelSet *);
void InitModelSet (ModelSet *);
void DeleteModelSet(ModelSet *ms);

typedef struct _Pattern{  /* pattern handler for question storage */
   char *pat;               /* pattern */
   struct _Pattern *next;   /* link to next pattern */
} Pattern;

typedef struct _Question { /* question storage */
   char *qName;              /* name of this question */
   Pattern *phead;           /* link to head of pattern list */
   Pattern *ptail;           /* link to tail of pattern list */
   struct _Question *next;  /* link to next question */
} Question;

typedef struct _Node {     /* node of decision tree */
   int idx;                 /* index of this node */
   int pdf;                 /* index of pdf for this node  ( leaf node only ) */
   struct _Node *yes;       /* link to child node (yes) */
   struct _Node *no;        /* link to child node (no)  */
   Question *quest;          /* question applied at this node */
} Node;
   
typedef struct _Tree {      
   int state;                 /* state position of this tree */
   struct _Tree *next;        /* link to next tree */
   Node *root;                 /* root node of this decision tree */
} Tree;

typedef struct _TreeSet {
   Question *qhead[3];
   Question *qtail[3];

   Tree *thead[3];
   Tree *ttail[3];
   
   cst_file fp[3];
   
} TreeSet;

void LoadTreesFile (TreeSet *, Mtype);
int SearchTree (char *, Node *);
void InitTreeSet(TreeSet *);
void FreeTrees(TreeSet *ts, Mtype type);

typedef struct _VocoderSetup {
   
   int fprd;
   int iprd;
   int seed;
   int pd;
   unsigned long next;
   double p1;
   double pc;
   double pj;
   double pade[21];
   double *ppade;
   double *c, *cc, *cinc, *d1;
   double rate;
   
   int sw;
   double r1, r2, s;
   
   int x;
   
} VocoderSetup;

void init_vocoder(int m, VocoderSetup *vs);
void vocoder (double p, float *mc, int m, cst_wave *w, int samp_offset, globalP *gp, VocoderSetup *vs);

typedef struct _DWin {
   int num;           /* number of static + deltas */
   char **fn;         /* delta window coefficient file */
   int **width;       /* width [0..num-1][0(left) 1(right)] */
   float **coef;      /* coefficient [0..num-1][length[0]..length[1]] */
   float **coefr;     /* pointers to the memory being allocated */
   int maxw[2];       /* max width [0(left) 1(right)] */
   int max_L;
} DWin;

typedef struct _PStream {
   int vSize;
   int order;
   int T;
   int width;
   DWin dw;
   double **mseq;   /* sequence of mean vector */
   double **ivseq;	 /* sequence of inversed variance vector */
   double **R;
   double *r;
   double *g;
   float **c;     /* output parameter vector */
} PStream;

cst_wave * pdf2speech(PStream *, PStream *, globalP *, ModelSet *, UttModel *, VocoderSetup *);
void ReadWin(PStream *);
void FreeWin(PStream *);
