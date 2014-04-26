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
/*                        Copyright (c) 2000                             */
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
/*  Audio                                                                */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_AUDIO_H__
#define _CST_AUDIO_H__

#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_wave.h"

#define BELL_AUDIO_ERROR -1
#define BELL_AUDIO_OK 0

#define CST_AUDIOBUFFSIZE 128

typedef enum {
    CST_AUDIO_LINEAR16 = 0,
    CST_AUDIO_LINEAR8,
    CST_AUDIO_MULAW
} cst_audiofmt;
/* Returns the number of bytes per sample for a given audio format */
int audio_bps(cst_audiofmt fmt);

typedef struct cst_audiodev_struct {
    int sps, real_sps;
    int channels, real_channels;
    cst_audiofmt fmt, real_fmt;
    int byteswap;
    void *platform_data;
} cst_audiodev;

/* Generic audio functions */
cst_audiodev *audio_open(int sps, int channels, cst_audiofmt fmt);
int audio_close(cst_audiodev *ad);
int audio_write(cst_audiodev *ad, void *buff, int num_bytes);
int audio_flush(cst_audiodev *ad); /* wait for buffers to empty */
int audio_drain(cst_audiodev *ad); /* empty buffers now */

/* Generic high level audio functions */
int play_wave(cst_wave *w);

/* For audio streaming */
#define CST_AUDIO_STREAM_STOP -1
#define CST_AUDIO_STREAM_CONT 0
typedef struct cst_audio_streaming_info_struct
{
    int min_buffsize;
    int (*asc)(const cst_wave *w,int start,int size, 
               int last, struct cst_audio_streaming_info_struct *asi);

    const cst_utterance *utt; /* in case you need more information */
    const cst_item *item;     /* because you'll probably want this */
                              /* But this is *not* updated automatically */
    void *userdata;
} cst_audio_streaming_info;
cst_audio_streaming_info *new_audio_streaming_info();
void delete_audio_streaming_info(cst_audio_streaming_info *asi);
CST_VAL_USER_TYPE_DCLS(audio_streaming_info,cst_audio_streaming_info)
typedef int (*cst_audio_stream_callback)(const cst_wave *w,int start,int size, 
                                      int last, cst_audio_streaming_info *asi);

/* An example audio streaming callback function src/audio/au_streaming.c */
int audio_stream_chunk(const cst_wave *w, int start, int size, 
                       int last, cst_audio_streaming_info *asi);

#endif
