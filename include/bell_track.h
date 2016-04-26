#ifndef _BELL_TRACK_H__
#define _BELL_TRACK_H__

#include "cst_val.h"

typedef struct  bell_track_struct {
    int num_frames;
    int num_channels;
    int **idxmcep; // vectors of indexes of mcep
    float **frames;
} bell_track;

bell_track *new_track(int num_frames, int num_channels, int num_idx_vec);
void delete_track(bell_track *val);

#endif
