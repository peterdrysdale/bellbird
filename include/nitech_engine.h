#ifndef __NITECH_ENGINE_H__
#define __NITECH_ENGINE_H__

#include "cst_utterance.h"
#include "cst_file.h"

typedef struct _ModelSet { // HMM set handler
   int nstate;
   int lf0stream;
   int mcepvsize;
   int ndurpdf;
   int *nmceppdf;
   int *nlf0pdf;
   float **durpdf,***mceppdf,****lf0pdf;
   cst_file fp[3];
} ModelSet;

typedef struct _Pattern{ // pattern handler for question storage
   char *pat;               // pattern
   struct _Pattern *next;   // pointer to next pattern
} Pattern;

typedef struct _Question { // question storage
   char *qName;              // name of this question
   Pattern *phead;           // pointer to head of pattern list
   Pattern *ptail;           // pointer to tail of pattern list
   struct _Question *next;   // pointer to next question
} Question;

typedef struct _Node { // node of decision tree
   int idx;                 // index of this node
   int pdf;                 // index of pdf for this node  ( leaf node only )
   struct _Node *yes;       // pointer to its child node (yes)
   struct _Node *no;        // pointer to its child node (no)
   Question *quest;         // question applied at this node
} Node;
   
typedef struct _Tree { // decision tree
   int state;               // state index of this tree
   struct _Tree *next;      // pointer to next tree
   Node *root;              // root node of this decision tree
} Tree;

typedef struct _TreeSet {
   Question *qhead[3];
   Question *qtail[3];

   Tree *thead[3];
   Tree *ttail[3];
   
   cst_file fp[3];
   
} TreeSet;

typedef struct _DWin {
   int num;           // number of static + deltas
   char **fn;         // filename for delta window coefficient
   int **width;       // width [0..num-1][0(left) 1(right)]
   float **coef;      // coefficient [0..num-1][length[0]..length[1]]
   float **coef_ptrs; // pointers to the memory being allocated so they can be freed
   int maxw[2];       // max width [0(left) 1(right)]
   int max_L;
} DWin;

typedef struct _PStreamChol {
   int vSize;
   int order;
   int T;           // number of frames
   int width;
   DWin dw;
   double **mseq;   // sequence of mean vector
   double **ivseq;  // sequence of inversed variance vector
   double **R;
   double *r;
   double *g;
   double **c;      // output parameter vector
} PStreamChol;

typedef struct _VocoderSetup {
   
   int fprd;        //  frame period
   int iprd;        //  interpolation period
   int seed;        //  random number seed holder
   int pd;          //  Pade order
   unsigned long next;
   double p1;
   double pc;
   double pj;
   double pade[21]; //  Pade approximants coefficients
   double *c, *cc, *cinc, *d1;
   double rate;     //  output wave's sample rate
   
   int sw;          //  switch for Gaussian random number generator since numbers are thrown in pairs
   double r1, r2, s;//  intermediate values of Gaussian random number generator
   
   int x;
   
} VocoderSetup;

typedef struct _nitech_engine {
   cst_utterance *utt;
   ModelSet   ms;
   TreeSet    ts;
   PStreamChol    mceppst, lf0pst;
   VocoderSetup vs;
} nitech_engine;

void nitech_engine_initialize(nitech_engine *ntengine, const char * fn_voice);
bell_boolean nitech_engine_synthesize_from_strings(
                      nitech_engine * ntengine, char **lines, size_t num_lines);
void nitech_engine_clear(nitech_engine * ntengine);

#endif
