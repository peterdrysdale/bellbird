// Common structures for cg modules

typedef struct _DWin {
   int num;            // number of static + deltas
   int **width;        // width [0..num-1][0(left) 1(right)]
   double **coef;      // coefficient [0..num-1][length[0]..length[1]]
   double **coef_ptrs; // pointers to the memory being allocated so they can be freed
   int maxw[2];        // max width [0(left) 1(right)]
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
