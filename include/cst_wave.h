/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  August 2000                                      */
/*************************************************************************/
/*                                                                       */
/*  Waveforms                                                            */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_WAVE_H__
#define _CST_WAVE_H__

#include <stdint.h>
#include "cst_file.h"
#include "cst_val.h"

typedef struct  cst_wave_struct {
    int sample_rate;
    int num_samples;
    int num_channels;
    int16_t *samples;
} cst_wave;

cst_wave *new_wave();
void delete_wave(cst_wave *val);

#define CST_WAVE_NUM_SAMPLES(w) (w?w->num_samples:0)
#define CST_WAVE_NUM_CHANNELS(w) (w?w->num_channels:0)
#define CST_WAVE_SAMPLE_RATE(w) (w?w->sample_rate:0)
#define CST_WAVE_SAMPLES(w) (w->samples)

#define CST_WAVE_SET_NUM_SAMPLES(w,s) w->num_samples=s
#define CST_WAVE_SET_SAMPLE_RATE(w,s) w->sample_rate=s

#define BELL_IO_SUCCESS  0
#define BELL_IO_ERROR   -1

int cst_wave_save_riff(cst_wave *w, const char *filename);
int cst_wave_append_riff(cst_wave *w,const char *filename);

int cst_wave_save_riff_fd(cst_wave *w, cst_file fd);

int cst_wave_load(cst_wave *w, const char *filename, const char *type);
int cst_wave_load_riff(cst_wave *w, const char *filename);

int cst_wave_load_riff_fd(cst_wave *w, cst_file fd);

void cst_wave_resize(cst_wave *w,int samples, int num_channels);

CST_VAL_USER_TYPE_DCLS(wave,cst_wave)

#endif
