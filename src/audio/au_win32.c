/*************************************************************************/
/*                This code has been created for Bellbird.               */
/*                See COPYING for more copyright details.                */
/*************************************************************************/
// A Windows Multimedia audio driver for use with bellbird
// The Windows Multimedia driver should work with Win2000+
#ifdef BELL_AUDIO_WIN32

#include <windows.h>
#include <mmsystem.h> // Windows Multimedia API aka MME API

#include "cst_alloc.h"
#include "cst_error.h"
#include "bell_audio.h"

// Structure for Win32 platform specific audio data
typedef struct win32_au_platform_struct {
   HWAVEOUT hwo;
   HANDLE done_event;  // Event triggered by callback when sample finishes playing
   WAVEHDR * done_hdr; // Header of finished sound sample
} win32_au_platform;

void CALLBACK win32_au_callback(HWAVEOUT hwo, UINT uMsg,
			  DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
// Callback to signal end of sound playing and enable passing of header for clean up
// Callback is required to be of form waveOutProc
    (void) dwParam2; // Unused parameter of callback
    (void) hwo;      // Unused parameter of callback
    WAVEHDR *hdr = (WAVEHDR *)dwParam1;
    win32_au_platform *win32_au_data = (win32_au_platform *)dwInstance;

    if (MM_WOM_DONE == uMsg) {
        win32_au_data->done_hdr = hdr;
        SetEvent(win32_au_data->done_event);
    }
}

cst_audiodev *audio_open_win32(unsigned int sps, int channels)
{
// Open a Win32 MME audio device
    cst_audiodev *ad;
    win32_au_platform *win32_au_data;
    HWAVEOUT hwo;
    WAVEFORMATEX pwfx;

    win32_au_data = cst_alloc(win32_au_platform,1);
// Create WAVEFORMATEX data about sound sample
    pwfx.wFormatTag = WAVE_FORMAT_PCM;
    pwfx.nChannels = channels;
    pwfx.nSamplesPerSec = sps;
    pwfx.nAvgBytesPerSec = sps * channels * BELL_AUDIO_16BIT;
    pwfx.nBlockAlign = channels * BELL_AUDIO_16BIT;
    pwfx.wBitsPerSample = BELL_AUDIO_16BIT * 8;
    pwfx.cbSize =0;
// Open audio for output
    if ( MMSYSERR_NOERROR != waveOutOpen(&hwo,WAVE_MAPPER,&pwfx,
                                        (DWORD_PTR)win32_au_callback,
                                        (DWORD_PTR)win32_au_data,
                                        CALLBACK_FUNCTION))
    {
        cst_errmsg("audio_open_win32: failed to open audio device\n");
        cst_free(win32_au_data);
        return NULL;
    }

    win32_au_data->hwo = hwo;
// create event which will signal when sample has finished playing
    win32_au_data->done_event = CreateEvent(NULL,FALSE,FALSE,NULL);

    ad = cst_alloc(cst_audiodev,1);
    ad->sps = sps;
    ad->channels = channels;
    ad->platform_data = win32_au_data;
    return ad;
}

int audio_close_win32(cst_audiodev *ad)
{
// Close audio device
    int retval;
    win32_au_platform *win32_au_data;

    if (ad == NULL) return 0;

    win32_au_data = (win32_au_platform *)ad->platform_data;

    if (MMSYSERR_NOERROR != waveOutClose(win32_au_data->hwo))
    {
        cst_errmsg("audio_close_win32: failed to close output device\n");
        retval = -1;
    }
    else
    {
        retval = 0;
    }
    cst_free(win32_au_data); // free win32 specific data
    cst_free(ad); // free bellbird's audio device data

    return retval;
}

int audio_write_win32(cst_audiodev *ad, void *samples, int num_frames)
{
// Write sound sample to audio device and wait for it to finish playing
    win32_au_platform *win32_au_data = (win32_au_platform *)ad->platform_data;
    HWAVEOUT hwo = win32_au_data->hwo;
    WAVEHDR *hdr; // Header for sending to audio device
    int num_bytes = BELL_AUDIO_16BIT * ad->channels * num_frames;

    if (num_frames == 0) return 0;
// Create header with sample info to send to audio device
    hdr = cst_alloc(WAVEHDR,1);
    hdr->lpData = samples;
    hdr->dwBufferLength = num_bytes;
// Prepare header for sending to audio device
    if (MMSYSERR_NOERROR != waveOutPrepareHeader(hwo, hdr, sizeof(*hdr)))
    {
        cst_errmsg("audio_write_win32: waveOutPrepareHeader failed\n");
        cst_free(hdr);
        return 0;
    }
// Write sample to audio device
    if (MMSYSERR_NOERROR != waveOutWrite(hwo, hdr, sizeof(*hdr)))
    {
        cst_errmsg("audio_write_win32: waveOutWrite failed\n");
        cst_free(hdr);
        return 0;
    }
// Wait for sound to finish
    WaitForSingleObject(win32_au_data->done_event, INFINITE);
// Reset device
    if (MMSYSERR_NOERROR != waveOutReset(hwo))
    {
        cst_errmsg("audio_write_win32: failed to reset output device\n");
        return 0;
    }
// Clean up header of sample which has finished playing
    if (MMSYSERR_NOERROR != waveOutUnprepareHeader(hwo,
                                                   win32_au_data->done_hdr,
                                                   sizeof(*(win32_au_data->done_hdr))))
    {
        cst_errmsg("audio_write_win32: failed to unprepare header\n");
        return 0;
    }
    cst_free(win32_au_data->done_hdr);

    return num_bytes;
}

#endif // BELL_AUDIO_WIN32