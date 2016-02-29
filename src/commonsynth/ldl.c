/*************************************************************************/
/*                This code has been created for Bellbird.               */
/*                See COPYING for more copyright details.                */
/*************************************************************************/
static void factorize(double ** R, const int width, const int T)
{
   int i, j, k;

   for (i = 0; i < T; i++)
   {
      for (j = 1; j < width && i-j >= 0; j++)
      {
         R[i][0] -= R[i-j][j] * R[i-j][j] * R[i-j][0];
      }
      for (j = 1; j < width; j++)
      {
         for (k = 1; j+k < width && i-k >= 0; k++)
         {
            R[i][j] -= R[i-k][k] * R[i-k][j+k] * R[i-k][0];
         }
         R[i][j] /= R[i][0];
      }
   }
}

static void forward_subst(double **R, double *g, double *r,
                            const int width, const int T)
{
   int i, j;

   g[0] = r[0];

   for (i = 1; i < T; i++)
   {
      g[i] = r[i];
      for (j = 1; j < width && i-j >= 0; j++)
      {
         g[i] -= R[i-j][j] * g[i-j];
      }
   }
}

static void backward_subst(double **R, double *g, double *c,
                           const int width, const int T,
                           float **frames, const int frame_pos)
{
   int i, j;

   for (i = T-1; i>=0; i--)
   {
      c[i] = g[i] / R[i][0];
      for (j = 1; j < width && i+j < T; j++)
      {
         c[i] -= R[i][j] * c[i+j];
      }
      frames[i][frame_pos] = c[i]; // store result
   }
}

static void solvemateqn(PStream * pst, float **frames, const int frame_pos)
{
// Solve Matrix equation R c = r
// Use LDL decomposition since it is faster than
// Cholesky decomposition due to lack of square roots
// Store answer in frames[i][frame_pos]
   factorize(pst->R,pst->width,pst->T);
   forward_subst(pst->R,pst->g,pst->r,pst->width,pst->T);
   backward_subst(pst->R,pst->g,pst->c,pst->width,pst->T,frames,frame_pos);
}
