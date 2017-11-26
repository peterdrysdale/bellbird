/*************************************************************************/
/*                This code has been created for Bellbird.               */
/*                See COPYING for more copyright details.                */
/*************************************************************************/
// A PulseAudio audio driver for use with bellbird
#ifdef BELL_AUDIO_PULSE

#include <pulse/simple.h> // for pa_simple_new(), pa_simple_drain(),
                          // pa_simple_free(), pa_simple write().
#include <pulse/error.h>  // for pa_strerror()

#include "cst_alloc.h"
#include "cst_error.h"
#include "bell_audio.h"

cst_audiodev *audio_open_pulse(unsigned int sps, int channels)
{
// Open a Pulseaudio audio device
    cst_audiodev *ad;
    pa_simple *pa_handle;
    pa_sample_spec *sample_spec;
    int err = 0;

    sample_spec = cst_alloc(pa_sample_spec, 1);
    sample_spec->channels = channels;
    sample_spec->rate = sps;
#ifdef WORDS_BIGENDIAN
    sample_spec->format = PA_SAMPLE_S16BE;
#else
    sample_spec->format = PA_SAMPLE_S16LE;
#endif

    pa_handle = pa_simple_new(NULL,        // Use the default server.
                              "bellbird",
                              PA_STREAM_PLAYBACK,
                              NULL,        // Use the default device.
                              "Text to Speech",
                              sample_spec,
                              NULL,        // Use default channel map
                              NULL,        // Use default buffering
                                           // attributes.
                              &err);
    if (!pa_handle)
    {
        cst_errmsg("PulseAudio error - pa_simple_new() failed with: %s\n", pa_strerror(err));
        cst_free(sample_spec);
        return NULL;
    }

    ad = cst_alloc(cst_audiodev,1);
    ad->sps = sps;
    ad->channels = channels;
    ad->platform_data = (void *) pa_handle;
    return ad;
}

int audio_close_pulse(cst_audiodev *ad)
{
// Close Pulseaudio device
    int err;
    pa_simple *pa_handle;

    if (ad == NULL)
        return 0;

    pa_handle = (pa_simple *) ad->platform_data;

    if( pa_simple_drain(pa_handle, &err) < 0)
    {
        cst_errmsg("PulseAudio error - pa_simple_drain() failed with: %s\n", pa_strerror(err));
    }

    pa_simple_free(pa_handle);
    cst_free(ad);
    return 0;
}

int audio_write_pulse(cst_audiodev *ad, void *samples, int num_frames)
{
// Write sound sample to Pulseaudio device
    pa_simple *pa_handle;
    int err;
    int num_bytes = BELL_AUDIO_16BIT * ad->channels * num_frames;

    pa_handle = (pa_simple *) ad->platform_data;
    if (pa_simple_write(pa_handle, samples, num_bytes, &err) < 0)
    {
        cst_errmsg("PulseAudio error - pa_simple_write() failed with: %s\n", pa_strerror(err));
        return 0;
    }

    return num_frames;
}

#endif // BELL_AUDIO_PULSE