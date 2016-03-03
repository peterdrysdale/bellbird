// Common structures for cg modules

typedef struct _DWin {
   int num;           // number of static + deltas
   int width[2];      // width [0(left) 1(right)]
   double *coef;      // coefficient [length[0]..length[1]]
   double *coef_ptrs; // pointers to the memory being allocated so they can be freed
} DWin;

typedef struct _PStream {
   int T;           // number of frames
   int width;
   DWin dw;
   double **mseq;   // sequence of mean vector
   double **ivseq;  // sequence of inversed variance vector
   double **R;
   double *r;
   double *g;
   double *c;       // output parameter vector
} PStream;
