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
/*               Date:  October 2000                                     */
/*************************************************************************/
/*                                                                       */
/*  Access to audio devices                                   ,          */
/*                                                                       */
/*************************************************************************/
#include "cst_audio.h"
#include "cst_endian.h"
#include "cst_error.h"
#include "cst_string.h"
#include "cst_wave.h"
#include "native_audio.h"

int audio_bps(cst_audiofmt fmt)
{
    switch (fmt)
    {
    case CST_AUDIO_LINEAR16:
	return 2;
    case CST_AUDIO_LINEAR8:
    case CST_AUDIO_MULAW:
	return 1;
    }
    return 0;
}

cst_audiodev *audio_open(int sps, int channels, cst_audiofmt fmt)
{
    cst_audiodev *ad;

    ad = AUDIO_OPEN_NATIVE(sps, channels, fmt);
    if (ad == NULL)
	return NULL;

    return ad;
}

int audio_close(cst_audiodev *ad)
{
    return AUDIO_CLOSE_NATIVE(ad);
}

int audio_write(cst_audiodev *ad,void *buff,int num_bytes)
{
    void *abuf = buff, *nbuf = NULL;
    int rv, i, real_num_bytes = num_bytes;

    if (ad->real_channels != ad->channels)
    {
	/* Yeah, we only do mono->stereo for now */
	if (ad->real_channels != 2 || ad->channels != 1)
	{
	    cst_errmsg("audio_write: unsupported channel mapping requested (%d => %d).\n",
		       ad->channels, ad->real_channels);
	}
	nbuf = cst_alloc(char, real_num_bytes * ad->real_channels / ad->channels);

	if (audio_bps(ad->fmt) == 2)
	{
	    for (i = 0; i < real_num_bytes / 2; ++i)
	    {
		((short *)nbuf)[i*2] = ((short *)abuf)[i];
		((short *)nbuf)[i*2+1] = ((short *)abuf)[i];
	    }
	}
	else if (audio_bps(ad->fmt) == 1)
	{
	    for (i = 0; i < real_num_bytes / 2; ++i)
	    {
		((unsigned char *)nbuf)[i*2] = ((unsigned char *)abuf)[i];
		((unsigned char *)nbuf)[i*2+1] = ((unsigned char *)abuf)[i];
	    }
	}
	else
	{
	    cst_errmsg("audio_write: unknown format %d\n", ad->fmt);
	    cst_free(nbuf);
	    if (abuf != buff)
		cst_free(abuf);
	    cst_error();
	}

	if (abuf != buff)
	    cst_free(abuf);
	abuf = nbuf;
	real_num_bytes = real_num_bytes * ad->real_channels / ad->channels;
    }
    if (ad->byteswap && audio_bps(ad->real_fmt) == 2)
	swap_bytes_short((short *)abuf, real_num_bytes/2);

    if (real_num_bytes)
	rv = AUDIO_WRITE_NATIVE(ad,abuf,real_num_bytes);
    else
	rv = 0;

    if (abuf != buff)
	cst_free(abuf);

    /* Callers expect to get the same num_bytes back as they passed
       in.  Funny, that ... */
    return (rv == real_num_bytes) ? num_bytes : 0;
}

int audio_drain(cst_audiodev *ad)
{
    return AUDIO_DRAIN_NATIVE(ad);
}

int audio_flush(cst_audiodev *ad)
{
    return AUDIO_FLUSH_NATIVE(ad);
}

int play_wave(cst_wave *w)
{
    cst_audiodev *ad;
    int i,n,r;
    int num_shorts;

    if (!w)
	return CST_ERROR_FORMAT;
    
    if ((ad = audio_open(w->sample_rate, w->num_channels,
			 /* FIXME: should be able to determine this somehow */
			 CST_AUDIO_LINEAR16)) == NULL)
	return CST_ERROR_FORMAT;

    num_shorts = w->num_samples*w->num_channels;
    for (i=0; i < num_shorts; i += r/2)
    {
	if (num_shorts > i+CST_AUDIOBUFFSIZE)
	    n = CST_AUDIOBUFFSIZE;
	else
	    n = num_shorts-i;
	r = audio_write(ad,&w->samples[i],n*2);
	if (r <= 0)
	{
	    cst_errmsg("failed to write %d samples\n",n);
	    break;
	}
    }

    audio_close(ad);

    return CST_OK_FORMAT;
}
