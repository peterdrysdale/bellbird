/*************************************************************************/
/*                This code has been created for Bellbird.               */
/*                See COPYING for more copyright details.                */
/*************************************************************************/
// An android audio driver for use with bellbird

#ifdef BELL_AUDIO_ANDROID

#include <SLES/OpenSLES.h>         // Android native audio
#include <SLES/OpenSLES_Android.h> // Android native audio

#include <time.h>      // for nanosleep()
#include "cst_alloc.h"
#include "bell_audio.h"
#include "native_audio.h"
#include "cst_error.h"

typedef struct android_au_platform_struct {
    SLObjectItf engineobject;    // engine interface
    SLEngineItf engineengine;    // engine interface
    SLObjectItf outputmixobject; // output mix interface
    SLObjectItf buffqueueobject;  // buffer queue player interface
    SLPlayItf buffqueueplay;      // buffer queue player interface
    SLAndroidSimpleBufferQueueItf simplebuffqueue; // buffer queue player interface
    short *bell_android_snd_buf; // buffer containing currently playing sample
    int *p_is_playing;           // flag if audio is currently playing
} android_au_platform;

void android_au_callback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    (void) bq;       // Unused parameter of callback
    int *p_is_playing = ((android_au_platform *)context)->p_is_playing;

//  Play back has finished, flag this in is_playing
    if (*p_is_playing > 0) (*p_is_playing)--;
    
    return;
}

cst_audiodev *audio_open_android(unsigned int sps, int channels)
{
    cst_audiodev *ad;
    SLresult res;

//  Create platform specific audio structures
    android_au_platform *plat = cst_alloc(android_au_platform,1);
    plat->engineobject = NULL;
    plat->outputmixobject = NULL;
    plat->buffqueueobject = NULL;

//  Set up sound format data
    SLDataFormat_PCM format;
    format.formatType = SL_DATAFORMAT_PCM;
    format.numChannels = channels;
    if (16000 == sps)
    {
        format.samplesPerSec = SL_SAMPLINGRATE_16;
    }
    else if (32000 == sps)
    {
        format.samplesPerSec = SL_SAMPLINGRATE_32;
    }
    else if (48000 == sps)
    {
        format.samplesPerSec = SL_SAMPLINGRATE_48;
    }
    else
    {
        return NULL;  // Sample rate probably unsupported in Android
    }
    format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    format.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    format.channelMask = SL_SPEAKER_FRONT_CENTER;
    format.endianness = SL_BYTEORDER_LITTLEENDIAN;

//  Create engine
    res = slCreateEngine(&(plat->engineobject), 0, NULL, 0, NULL, NULL);

//  Realize engine
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->engineobject))->Realize(plat->engineobject,
                                               SL_BOOLEAN_FALSE);
    }

//  Get engine interface
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->engineobject))->GetInterface(plat->engineobject,
                                                    SL_IID_ENGINE,
                                                    &(plat->engineengine));
    }

//  Create output mix
    const SLInterfaceID ids[] = {};
    const SLboolean req[] = {};
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->engineengine))->CreateOutputMix(plat->engineengine,
                                                       &(plat->outputmixobject),
                                                       0, ids, req);
    }

//  Realize output mix
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->outputmixobject))->Realize(plat->outputmixobject,
                                                  SL_BOOLEAN_FALSE);
    }

//  Configure audio source
//  data will be in a memory buffer and with two buffers
    SLDataLocator_AndroidSimpleBufferQueue bufqloc;
    bufqloc.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    bufqloc.numBuffers = 2;
    SLDataSource audio_source = {&bufqloc, &format};

//  Configure audio sink
    SLDataLocator_OutputMix outmixloc;
    outmixloc.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    outmixloc.outputMix = plat->outputmixobject;
    SLDataSink audio_sink;
    audio_sink.pLocator = &outmixloc;
    audio_sink.pFormat = NULL;

//  Create player
    const SLInterfaceID ids1[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req1[1] = {SL_BOOLEAN_TRUE};
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->engineengine))->CreateAudioPlayer(plat->engineengine,
                                                         &(plat->buffqueueobject),
                                                         &audio_source, &audio_sink,
                                                         1, ids1, req1);
    }

//  Realize player
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->buffqueueobject))->Realize(plat->buffqueueobject,
                                                 SL_BOOLEAN_FALSE);
    }

//  Get play interface
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->buffqueueobject))->GetInterface(plat->buffqueueobject,
                                                      SL_IID_PLAY,
                                                      &(plat->buffqueueplay));
    }

//  Get buffer queue interface
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->buffqueueobject))->GetInterface(plat->buffqueueobject,
                                                      SL_IID_BUFFERQUEUE,
                                                      &(plat->simplebuffqueue));
    }

//  Register callback on the buffer queue
    if (SL_RESULT_SUCCESS == res)
    {
        res = (*(plat->simplebuffqueue))->RegisterCallback(plat->simplebuffqueue,
                                                               android_au_callback,
                                                               (void *)plat);
    }
    if (SL_RESULT_SUCCESS != res)
    {
        audio_close_android(NULL);
        return NULL;
    }

    plat->p_is_playing = cst_alloc(int,1);
    *(plat->p_is_playing) = 0; // start playing flag in off state
    plat->bell_android_snd_buf = NULL;

    ad = cst_alloc(cst_audiodev,1);
    ad->sps = sps;
    ad->channels = channels;
    ad->platform_data = plat;

    return ad;
}

int audio_close_android(cst_audiodev *ad)
{ // Clean up and close audio device
    struct timespec sleepdelay;
    android_au_platform *plat = ad->platform_data;

//  Wait for audio to finish
    sleepdelay.tv_sec=0;
    sleepdelay.tv_nsec=10000000;  // 10ms delay
    while (*(plat->p_is_playing) != 0)
    {
        // sleep while waiting for sound to finish
        nanosleep(&sleepdelay,NULL);
    }

//  Clean up most recently played sound
    if (plat->bell_android_snd_buf != NULL)
    {
        cst_free(plat->bell_android_snd_buf);
        plat->bell_android_snd_buf = NULL;
    }

//  Clean up is_playing flag
    cst_free(plat->p_is_playing);

//  Clean up buffer queue object
    if (plat->buffqueueobject != NULL)
    {
        (*(plat->buffqueueobject))->Destroy(plat->buffqueueobject);
        plat->buffqueueobject = NULL;
        plat->buffqueueplay = NULL;
        plat->simplebuffqueue = NULL;
    }

//  Clean up output mix object
    if (plat->outputmixobject != NULL)
    {
        (*(plat->outputmixobject))->Destroy(plat->outputmixobject);
        plat->outputmixobject = NULL;
    }

//  Clean up engine object
    if (plat->engineobject != NULL)
    {
        (*(plat->engineobject))->Destroy(plat->engineobject);
        plat->engineobject = NULL;
        plat->engineengine = NULL;
    }

//  Free platform specific data structures
    cst_free(plat);

//  Free bellbird's audio device data
    if (ad != NULL) cst_free(ad);

    return 0;
}

int audio_write_android(cst_audiodev *ad, void *samples, int num_frames)
{
    struct timespec sleepdelay;
    SLresult res;
    int num_bytes = BELL_AUDIO_16BIT * ad->channels * num_frames;
    android_au_platform *plat = ad->platform_data;

//  Sleep while waiting for sound to finish
    sleepdelay.tv_sec=0;
    sleepdelay.tv_nsec=10000000;  // 10ms delay
    while (*(plat->p_is_playing) != 0)
    {
        nanosleep(&sleepdelay,NULL);
    }

//  Clean up most recently played sound
    if (plat->bell_android_snd_buf != NULL)
    {
        cst_free(plat->bell_android_snd_buf);
        plat->bell_android_snd_buf = NULL;
    }

//  Copy next wave to buffer
    plat->bell_android_snd_buf = (void *)cst_alloc(char, num_bytes);
    memcpy(plat->bell_android_snd_buf, samples, num_bytes);

//  Queue up next sound for playing
    *(plat->p_is_playing) = 1;
    res = (*(plat->simplebuffqueue))->Enqueue(plat->simplebuffqueue,
                                                  plat->bell_android_snd_buf,
                                                  num_bytes);
    if (SL_RESULT_SUCCESS != res)
    {
        return 0;
    }

//  Start playing sound
    res = (*(plat->buffqueueplay))->SetPlayState(plat->buffqueueplay,
                                                SL_PLAYSTATE_PLAYING);
    if (SL_RESULT_SUCCESS != res)
    {
        return 0;
    }

    return num_bytes;
}

#endif //BELL_AUDIO_ANDROID