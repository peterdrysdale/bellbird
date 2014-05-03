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
/*                        Copyright (c) 2005                             */
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
/*********************************************************************** */
/*             Author:  Lukas Loehrer ()                                 */
/*               Date:  January 2005                                     */
/*************************************************************************/
/*                                                                       */
/*  Native access to alsa audio devices on Linux                         */
/*  Tested with libasound version 1.0.10                                 */
/*                                                                       */
/*  Added snd_config_update_free_global(); after every close to stop     */
/*  (apparent?) memory leaks                                             */
/*************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CST_AUDIO_ALSA

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>

#include "cst_alloc.h"
#include "cst_error.h"
#include "bell_audio.h"
#include "native_audio.h"

#include <alsa/asoundlib.h>

static const char *pcm_dev_name ="default";

cst_audiodev *audio_open_alsa(unsigned int sps, int channels)
{
  cst_audiodev *ad;
  int err;

  /* alsa specific stuff */
  snd_pcm_t *pcm_handle;
  snd_pcm_hw_params_t *hwparams;
  snd_pcm_format_t format;

  /* Allocate the snd_pcm_hw_params_t structure on the stack. */
  snd_pcm_hw_params_alloca(&hwparams);

  /* Open pcm device */
  err = snd_pcm_open(&pcm_handle, pcm_dev_name, SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) 
  {
	cst_errmsg("audio_open_alsa: failed to open audio device %s. %s\n",
			   pcm_dev_name, snd_strerror(err));
	return NULL;
  }

  /* Init hwparams with full configuration space */
  err = snd_pcm_hw_params_any(pcm_handle, hwparams);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to get hardware parameters from audio device. %s\n", snd_strerror(err));
	return NULL;
  }

  /* Set access mode */
  err = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set access mode. %s.\n", snd_strerror(err));
	return NULL;
  }

#ifdef WORDS_BIGENDIAN
  format = SND_PCM_FORMAT_S16_BE;
#else
  format = SND_PCM_FORMAT_S16_LE;
#endif

  /* Set sample format */
  err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, format);
  if (err <0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set format. %s.\n", snd_strerror(err));
	return NULL;
  }

  /* Set sample rate */
  err = snd_pcm_hw_params_set_rate(pcm_handle, hwparams, sps, 0);
  if (err < 0)   
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set sample rate near %d. %s.\n", sps, snd_strerror(err));
	return NULL;
  }

  /* Set number of channels */
  assert(channels >0);
  err = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, channels);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set number of channels to %d. %s.\n", channels, snd_strerror(err));
	return NULL;
  }

  /* Commit hardware parameters */
  err = snd_pcm_hw_params(pcm_handle, hwparams);
  if (err < 0) 
  {
	snd_pcm_close(pcm_handle);
        snd_config_update_free_global();
	cst_errmsg("audio_open_alsa: failed to set hw parameters. %s.\n", snd_strerror(err));
	return NULL;
  }

  /* Make sure the device is ready to accept data */
  assert(snd_pcm_state(pcm_handle) == SND_PCM_STATE_PREPARED);

  /* Write hardware parameters to audio device data structure */
  ad = cst_alloc(cst_audiodev, 1);
  assert(ad != NULL);
  ad->sps = sps;
  ad->channels = channels;
  ad->platform_data = (void *) pcm_handle;

  return ad;
}

int audio_close_alsa(cst_audiodev *ad)
{
  int result;
  snd_pcm_t *pcm_handle;

  if (ad == NULL)
      return 0;

  pcm_handle = (snd_pcm_t *) ad->platform_data;

  snd_pcm_drain(pcm_handle); /* wait for current stuff in buffer to finish */

  result = snd_pcm_close(pcm_handle);
  snd_config_update_free_global();
  if (result < 0)
  {
	cst_errmsg("audio_close_alsa: Error: %s.\n", snd_strerror(result));
  }
  cst_free(ad);
  return result;
}

/* Returns zero if recovery was successful. */
static int recover_from_error(snd_pcm_t *pcm_handle, ssize_t res)
{
  res = snd_pcm_recover(pcm_handle,res,0);
  if (res < 0)
  {
	/* Unknown failure */
	cst_errmsg("audio_recover_from_write_error: %s.\n", snd_strerror(res));
	return res;
  }
  return 0;
}

int audio_write_alsa(cst_audiodev *ad, void *samples, int num_frames)
{
  size_t frame_size;
  ssize_t res;
  snd_pcm_t *pcm_handle;
  char *buf = (char *) samples;
  int frames_to_write;

  frames_to_write = num_frames;
  pcm_handle = (snd_pcm_t *) ad->platform_data;
  frame_size = BELL_AUDIO_16BIT * ad->channels;

  while (frames_to_write > 0)
  {
	res = snd_pcm_writei(pcm_handle, buf, frames_to_write); //Note pcm_handle open in blocked mode
	if (res != frames_to_write)
	{
	  if (res > 0 && res < frames_to_write)
	  {
		snd_pcm_wait(pcm_handle, 100);
	  }
	  else if (recover_from_error(pcm_handle, res) < 0) 
	  {
		return -1;
	  }
	}

	if (res >0) 
	{
	  frames_to_write -= res;
	  buf += res * frame_size;
	}
  }
  return num_frames;
}

#endif // CST_AUDIO_ALSA
