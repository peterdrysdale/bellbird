/*************************************************************************/
/*                This code has been created for Bellbird.               */
/*                See COPYING for more copyright details.                */
/*************************************************************************/
// Data structure for passing voice parameters inside clustergen module
#include "cst_alloc.h"
#include "cst_val.h"
#include "bell_track.h"

bell_track *new_track(int num_frames, int num_channels, int num_idx_vec)
{
    int i;

    bell_track *w = cst_alloc(struct bell_track_struct,1);
    if (num_idx_vec)
    { // If this is a param_track allocate vectors of mcep indexes.
      // Passing the indexes saves a copying step for the parameters themselves.
        w->idxmcep = cst_alloc(int *, num_frames);
        w->idxmcep[0] = cst_alloc(int,(num_idx_vec*num_frames));
        for (i = 1; i < num_frames; i++)
        {
            w->idxmcep[i] = w->idxmcep[i-1] + num_idx_vec;
        }
    }
    else
    {
        w->idxmcep = NULL;
    }
    w->frames = cst_alloc(float *,num_frames);
    w->frames[0] = cst_alloc(float,(num_channels*num_frames));
    for (i = 1; i < num_frames; i++)
    {
        w->frames[i] = w->frames[i-1] + num_channels;
    }
    w->num_frames = num_frames;
    w->num_channels = num_channels;
    return w;
}

void delete_track(bell_track *w)
{
    if (w)
    {
        if (w->idxmcep)
        {
            cst_free(w->idxmcep[0]);
            cst_free(w->idxmcep);
        }
	cst_free(w->frames[0]);
	cst_free(w->frames);
	cst_free(w);
    }
    return;
}
