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

#ifndef HTS_ENGINE_H
#define HTS_ENGINE_H

#ifdef __cplusplus
#define HTS_ENGINE_H_START extern "C" {
#define HTS_ENGINE_H_END   }
#else
#define HTS_ENGINE_H_START
#define HTS_ENGINE_H_END
#endif                          /* __CPLUSPLUS */

HTS_ENGINE_H_START;

#include <stdio.h>
#include <stdint.h>

/* common ---------------------------------------------------------- */

typedef char HTS_Boolean;

#ifndef TRUE
#define TRUE  1
#endif                          /* !TRUE */

#ifndef FALSE
#define FALSE 0
#endif                          /* !FALSE */

#ifndef HTS_NODATA
#define HTS_NODATA (-1.0e+10)
#endif                          /* HTS_NODATA */

/* copyright ------------------------------------------------------- */

#define HTS_COPYRIGHT "The HMM-Based Speech Synthesis Engine \"hts_engine API\"\nVersion 1.08 (http://hts-engine.sourceforge.net/)\nCopyright (C) 2001-2013 Nagoya Institute of Technology\n              2001-2008 Tokyo Institute of Technology\nAll rights reserved.\n"

/* model ----------------------------------------------------------- */

/* HTS_Window: window coefficients to calculate dynamic features. */
typedef struct _HTS_Window {
   size_t size;                 /* # of windows (static + deltas) */
   int *l_width;                /* left width of windows */
   int *r_width;                /* right width of windows */
   double **coefficient;        /* window coefficient */
   size_t max_width;            /* maximum width of windows */
} HTS_Window;

/* HTS_Pattern: list of patterns in a question and a tree. */
typedef struct _HTS_Pattern {
   char *string;                /* pattern string */
   struct _HTS_Pattern *next;   /* pointer to the next pattern */
} HTS_Pattern;

/* HTS_Question: list of questions in a tree. */
typedef struct _HTS_Question {
   char *string;                /* name of this question */
   HTS_Pattern *head;           /* pointer to the head of pattern list */
   struct _HTS_Question *next;  /* pointer to the next question */
} HTS_Question;

/* HTS_Node: list of tree nodes in a tree. */
typedef struct _HTS_Node {
   int index;                   /* index of this node */
   size_t pdf;                  /* index of PDF for this node (leaf node only) */
   struct _HTS_Node *yes;       /* pointer to its child node (yes) */
   struct _HTS_Node *no;        /* pointer to its child node (no) */
   struct _HTS_Node *next;      /* pointer to the next node */
   HTS_Question *quest;         /* question applied at this node */
} HTS_Node;

/* HTS_Tree: list of decision trees in a model. */
typedef struct _HTS_Tree {
   HTS_Pattern *head;           /* pointer to the head of pattern list for this tree */
   struct _HTS_Tree *next;      /* pointer to next tree */
   HTS_Node *root;              /* root node of this tree */
   size_t state;                /* state index of this tree */
} HTS_Tree;

/* HTS_Model: set of PDFs, decision trees and questions. */
typedef struct _HTS_Model {
   size_t vector_length;        /* vector length (static features only) */
   size_t num_windows;          /* # of windows for delta */
   HTS_Boolean is_msd;          /* flag for MSD */
   size_t ntree;                /* # of trees */
   size_t *npdf;                /* # of PDFs at each tree */
   float ***pdf;                /* PDFs */
   HTS_Tree *tree;              /* pointer to the list of trees */
   HTS_Question *question;      /* pointer to the list of questions */
} HTS_Model;

/* HTS_ModelSet: set of duration models, HMMs and GV models. */
typedef struct _HTS_ModelSet {
   char *hts_voice_version;     /* version of HTS voice format */
   size_t sampling_frequency;   /* sampling frequency */
   size_t frame_period;         /* frame period */
   size_t num_states;           /* # of HMM states */
   size_t num_streams;          /* # of streams */
   char *stream_type;           /* stream type */
   char *fullcontext_format;    /* fullcontext label format */
   char *fullcontext_version;   /* version of fullcontext label */
   HTS_Question *gv_off_context;        /* GV switch */
   char **option;               /* options for each stream */
   HTS_Model *duration;         /* duration PDFs and trees */
   HTS_Window *window;          /* window coefficients for delta */
   HTS_Model *stream;          /* parameter PDFs and trees */
   HTS_Model *gv;              /* GV PDFs and trees */
} HTS_ModelSet;

/* label ----------------------------------------------------------- */

/* HTS_LabelString: individual label string with time information */
typedef struct _HTS_LabelString {
   struct _HTS_LabelString *next;       /* pointer to next label string */
   char *name;                  /* label string */
   double start;                /* start frame specified in the given label */
   double end;                  /* end frame specified in the given label */
} HTS_LabelString;

/* HTS_Label: list of label strings */
typedef struct _HTS_Label {
   HTS_LabelString *head;       /* pointer to the head of label string */
   size_t size;                 /* # of label strings */
} HTS_Label;

/* sstream --------------------------------------------------------- */

/* HTS_SStream: individual state stream */
typedef struct _HTS_SStream {
   size_t vector_length;        /* vector length (static features only) */
   double **mean;               /* mean vector sequence */
   double **vari;               /* variance vector sequence */
   double *msd;                 /* MSD parameter sequence */
   size_t win_size;             /* # of windows (static + deltas) */
   int *win_l_width;            /* left width of windows */
   int *win_r_width;            /* right width of windows */
   double **win_coefficient;    /* window cofficients */
   size_t win_max_width;        /* maximum width of windows */
   double *gv_mean;             /* mean vector of GV */
   double *gv_vari;             /* variance vector of GV */
   HTS_Boolean *gv_switch;      /* GV flag sequence */
} HTS_SStream;

/* HTS_SStreamSet: set of state stream */
typedef struct _HTS_SStreamSet {
   HTS_SStream *sstream;        /* state streams */
   size_t nstream;              /* # of streams */
   size_t nstate;               /* # of states */
   size_t *duration;            /* duration sequence */
   size_t total_state;          /* total state */
   size_t total_frame;          /* total frame */
} HTS_SStreamSet;

/* pstream --------------------------------------------------------- */

/* HTS_SMatrices: matrices/vectors used in the speech parameter generation algorithm. */
typedef struct _HTS_SMatrices {
   double **mean;               /* mean vector sequence */
   double **ivar;               /* inverse diag variance sequence */
   double *g;                   /* vector used in the forward substitution */
   double **wuw;                /* W' U^-1 W  */
   double *wum;                 /* W' U^-1 mu */
} HTS_SMatrices;

/* HTS_PStream: individual PDF stream. */
typedef struct _HTS_PStream {
   size_t vector_length;        /* vector length (static features only) */
   size_t length;               /* stream length */
   size_t width;                /* width of dynamic window */
   double **par;                /* output parameter vector */
   HTS_SMatrices sm;            /* matrices for parameter generation */
   size_t win_size;             /* # of windows (static + deltas) */
   int *win_l_width;            /* left width of windows */
   int *win_r_width;            /* right width of windows */
   double **win_coefficient;    /* window coefficients */
   HTS_Boolean *msd_flag;       /* Boolean sequence for MSD */
   double *gv_mean;             /* mean vector of GV */
   double *gv_vari;             /* variance vector of GV */
   HTS_Boolean *gv_switch;      /* GV flag sequence */
   size_t gv_length;            /* frame length for GV calculation */
} HTS_PStream;

/* HTS_PStreamSet: set of PDF streams. */
typedef struct _HTS_PStreamSet {
   HTS_PStream *pstream;        /* PDF streams */
   size_t nstream;              /* # of PDF streams */
   size_t total_frame;          /* total frame */
} HTS_PStreamSet;

/* gstream --------------------------------------------------------- */

/* HTS_GStream: generated parameter stream. */
typedef struct _HTS_GStream {
   size_t vector_length;        /* vector length (static features only) */
   double **par;                /* generated parameter */
} HTS_GStream;

/* HTS_GStreamSet: set of generated parameter stream. */
typedef struct _HTS_GStreamSet {
   size_t total_nsample;        /* total sample */
   size_t total_frame;          /* total frame */
   size_t nstream;              /* # of streams */
   HTS_GStream *gstream;        /* generated parameter streams */
   int16_t *gspeech;             /* generated speech */
} HTS_GStreamSet;

/* engine ---------------------------------------------------------- */

/* HTS_Condition: synthesis condition */
typedef struct _HTS_Condition {
   /* global */
   size_t sampling_frequency;   /* sampling frequency */
   size_t fperiod;              /* frame period */
   size_t audio_buff_size;      /* audio buffer size (for audio device) */
   HTS_Boolean stop;            /* stop flag */
   double volume;               /* volume */
   double *msd_threshold;       /* MSD thresholds */
   double *gv_weight;           /* GV weights */

   /* duration */
   HTS_Boolean phoneme_alignment_flag;  /* flag for using phoneme alignment in label */
   double speed;                /* speech speed */

   /* spectrum */
   double alpha;                /* all-pass constant */
   double beta;                 /* postfiltering coefficient */

   /* log F0 */
   double additional_half_tone; /* additional half tone */

} HTS_Condition;

/* HTS_Engine: Engine itself. */
typedef struct _HTS_Engine {
   HTS_Condition condition;     /* synthesis condition */
   HTS_ModelSet ms;             /* set of duration models, HMMs and GV models */
   HTS_Label label;             /* label */
   HTS_SStreamSet sss;          /* set of state streams */
   HTS_PStreamSet pss;          /* set of PDF streams */
   HTS_GStreamSet gss;          /* set of generated parameter streams */
} HTS_Engine;

/* engine method --------------------------------------------------- */

/* HTS_Engine_initialize: initialize engine */
void HTS_Engine_initialize(HTS_Engine * engine);

/* HTS_Engine_load: load HTS voices */
HTS_Boolean HTS_Engine_load(HTS_Engine * engine, char **voices);

/* HTS_Engine_get_sampling_frequency: get sampling frequency */
size_t HTS_Engine_get_sampling_frequency(HTS_Engine * engine);

/* HTS_Egnine_set_msd_threshold: set MSD threshold */
void HTS_Engine_set_msd_threshold(HTS_Engine * engine, size_t stream_index, double f);

/* HTS_Engine_set_gv_weight: set GV weight */
void HTS_Engine_set_gv_weight(HTS_Engine * engine, size_t stream_index, double f);

/* HTS_Engine_set_speed: set speech speed */
void HTS_Engine_set_speed(HTS_Engine * engine, double f);

/* HTS_Engine_set_beta: set beta */
void HTS_Engine_set_beta(HTS_Engine * engine, double f);

/* HTS_Engine_add_half_tone: add half tone */
void HTS_Engine_add_half_tone(HTS_Engine * engine, double f);

/* HTS_Engine_synthesize_from_strings: synthesize speech from string list */
HTS_Boolean HTS_Engine_synthesize_from_strings(HTS_Engine * engine, char **lines, size_t num_lines);

/* HTS_Engine_refresh: free memory per one time synthesis */
void HTS_Engine_refresh(HTS_Engine * engine);

/* HTS_Engine_clear: free engine */
void HTS_Engine_clear(HTS_Engine * engine);

HTS_ENGINE_H_END;

#endif                          /* !HTS_ENGINE_H */
