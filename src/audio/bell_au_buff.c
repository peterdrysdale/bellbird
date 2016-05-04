/*************************************************************************/
/*                This code has been created for Bellbird.               */
/*                See COPYING for more copyright details.                */
/*************************************************************************/
// An Audio buffer for POSIX systems for use with bellbird

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CST_AUDIO_ALSA

#include <sys/select.h>// for select()
#include <sys/types.h> // for pid_t
#include <sys/wait.h>  // for waitpid()
#include <unistd.h>    // for fork(),pipe()
#include <time.h>      // for nanosleep()

#include "cst_alloc.h"
#include "cst_error.h"
#include "bell_audio.h"
#include "native_audio.h"
#include "cst_string.h"

// A queue is implemented holding cst_wave objects awaiting
// playing through audio device

#define BELL_QUEUE_LEN(q) ( (q) ? ((q)->length) : 0)
#define BELL_MAX_QUEUE_LEN 4

typedef struct bell_q_elmt {
    cst_wave * content;
    struct bell_q_elmt *next;
} bell_q_elmt;

typedef struct bell_queue_struct {
    bell_q_elmt *front;
    bell_q_elmt *rear;
    int length;
} bell_queue;

static bell_q_elmt *new_q_elmt(cst_wave * content)
{
//  Constructor for queued wave element
    bell_q_elmt *n;

    n = cst_alloc(bell_q_elmt,1);
    n->content = content;
    n->next = NULL;

    return n;
}

static void free_q_elmt(bell_q_elmt *elmt)
{
//  Delete queued wave element
    delete_wave(elmt->content);
    cst_free(elmt);
    return;
}

static bell_queue *new_bell_queue()
{
//  Constructor for new queue
    bell_queue *n;

    n = cst_alloc(bell_queue,1);
    n->front = NULL;
    n->rear = NULL;
    n->length = 0;

    return n;
}

static bell_queue *bell_enqueue(cst_wave *content,bell_queue *queue)
{
//  Add new cst_wave to queue at the rear of the queue
    bell_q_elmt *temp;

    temp = new_q_elmt(content);
    if (NULL == queue) queue = new_bell_queue();

    if (NULL == queue->rear)
    {
        queue->front = queue->rear = temp;
    }
    else
    {
        queue->rear->next = temp;
        queue->rear = temp;
    }
    (queue->length)++;

    return queue;
}

static bell_q_elmt *bell_dequeue(bell_queue *queue)
{
//  Remove cst_wave from front of queue to play
    bell_q_elmt *retval;

    if (NULL == queue || NULL == queue->front)
    {
        return NULL;
    }
    retval = queue->front;
    queue->front = queue->front->next;
    if (NULL == queue->front)
    {
        queue->rear = NULL;
    }

    (queue->length)--;
    
    return retval;
}

static const cst_wave *bell_observe_queue_front(bell_queue *queue)
{
//  Observe the next cst_wave in play out queue.
//  This cst_wave should be used in read only mode, it should
//  not be modified. Ownership is not taken by the caller.
    if (NULL == queue || NULL == queue->front)
    {
        return NULL;
    }
    return queue->front->content;
}

static void free_bell_queue(bell_queue *queue)
{
//  Delete queue and its contents
    bell_q_elmt *tempelmt; //queue elements for cleanup

    while( (tempelmt=bell_dequeue(queue)) !=NULL)
        free_q_elmt(tempelmt);
    cst_free(queue);
    return;
}

static int serialize_wave(cst_wave *w, int fd)
{
//  Serialize cst_wave structure and send over pipe to
//  audio scheduler process from main process
    int num_samples=CST_WAVE_NUM_SAMPLES(w);
    int sample_rate=CST_WAVE_SAMPLE_RATE(w);
    int num_channels=CST_WAVE_NUM_CHANNELS(w);
    unsigned int writtencount;
    int rv = 0;

    if (write(fd,&sample_rate,sizeof(int))!=sizeof(int))
    {
        rv = BELL_IO_ERROR;
    }
    if (write(fd,&num_samples,sizeof(int))!=sizeof(int))
    {
        rv = BELL_IO_ERROR;
    }
    if (write(fd,&num_channels,sizeof(int))!=sizeof(int))
    {
        rv = BELL_IO_ERROR;
    }
    if (BELL_IO_ERROR != rv)
    {
        writtencount=write(fd,w->samples,(num_samples*num_channels*sizeof(int16_t)));
        if (writtencount != (sizeof(int16_t)*num_channels*num_samples))
        {
            rv = BELL_IO_ERROR;
        }
    }
    if (BELL_IO_ERROR == rv)
    {
        cst_errmsg("serialize_wave:failed to write bytes to scheduler\n");
    }
    return rv;
}

int buffer_wave(cst_wave *w, int fd)
{
//  Command audio_scheduler to expect a cst_wave and serialize wave
//  over the pipe to audio scheduler
    if (!w || w->num_samples == 0) return FALSE;

    if (write(fd,"W",2)!=2)  // Command audio_scheduler to expect a cst_wave
    {
        cst_errmsg("buffer_wave:failed to command scheduler to accept audio\n");
        return BELL_IO_ERROR;
    }
    else
    {
        return serialize_wave(w,fd);
    }
}

static int deserialize_wave(cst_wave *w,int fd)
{
//  Deserialize cst_wave structure from pipe received by 
//  audio scheduler process from main process
    int sample_rate;
    int num_samples;
    int num_channels;
    unsigned int readcount;
    int rv = 0;

    if (read(fd,&sample_rate,sizeof(int))!=sizeof(int))
    {
        rv = BELL_IO_ERROR;
    }
    else
    {
        CST_WAVE_SET_SAMPLE_RATE(w,sample_rate);
    }
    if (read(fd,&num_samples,sizeof(int))!=sizeof(int))
    {
        rv = BELL_IO_ERROR;
    }
    if (read(fd,&num_channels,sizeof(int))!=sizeof(int))
    {
        rv = BELL_IO_ERROR;
    }
    if (BELL_IO_ERROR != rv)
    {
        cst_wave_resize(w,num_samples,num_channels);
        readcount=read(fd,w->samples,(num_samples*num_channels*sizeof(int16_t)));
        if (readcount !=(sizeof(int16_t)*num_channels*num_samples))
        {
            rv = BELL_IO_ERROR;
        }
    }
    if (BELL_IO_ERROR == rv)
    {
        cst_errmsg("deserialize_wave:scheduler failed to read bytes");
    }
    return rv;
}

static bell_queue *check_new_wave(bell_queue *q, int *p_stopreading, int *fd)
{
// Check if main thread is waiting to send the next cst_wave
// to audio scheduler and if so add it to the queue for play out
    fd_set rfds;             // file descriptor set for select()
    char readbuffer[2];      // Holds command from main process to audio
                             // scheduler
    struct timeval  selecttime;
    size_t readnum;
    cst_wave *w;

    if (FALSE == *p_stopreading && (BELL_QUEUE_LEN(q) < BELL_MAX_QUEUE_LEN) )
    {
        FD_ZERO(&rfds);
        FD_SET(fd[0],&rfds);
        selecttime.tv_sec = 0;
        selecttime.tv_usec = 0;
        if (select(fd[0]+1, &rfds, NULL, NULL, &selecttime) > 0)
        {  // Only read if data is available from main process
            readnum = read(fd[0],readbuffer,2);
            if ((readnum == 2) &&
                (cst_streq("W",readbuffer))) // About to receive a cst_wave
            {
                w=new_wave();
                if (deserialize_wave(w,fd[0]) != BELL_IO_ERROR)
                {
                    q=bell_enqueue(w,q);
                }
            }
            if ((readnum == 2) &&
                (cst_streq("E",readbuffer))) // No more cst_wave to receive
            {
                *p_stopreading=TRUE;
            }
        }
    }
    return q;
}

static cst_audiodev *audio_dispatch(bell_queue **p_q, cst_audiodev *ad,
                                    int *p_stopreading, int *fd)
{
//  Send cst_wave for playing in two halves
//  Between halves check for new cst_wave becoming available
//  Return audio device pointer for currently currently playing audio
    bell_q_elmt *elmt;
    const cst_wave *w;
    struct timespec sleepdelay;
    int first_samples;

    if ( (w = bell_observe_queue_front(*p_q)) != NULL )
// condition checks if more audio is available in the queue to play
    {
        if (NULL == ad)
        { // Open audio device if it has not been opened already
            if ((ad = audio_open_alsa(w->sample_rate, w->num_channels)) == NULL)
            {
                cst_errmsg("audio_dispatch: failed to open audio device.\n");
                _exit(1);
            }
        }

        first_samples = w->num_samples/2;
        // Write first half of audio data
        if (audio_write_alsa(ad,&w->samples[0],first_samples) <= 0)
        {   // On error try to write whole cst_wave in second half write
            first_samples = 0;
        }
        // Check for new data available for audio scheduler
        *p_q = check_new_wave(*p_q, p_stopreading, fd);
        // Write second half of audio data
        if (audio_write_alsa(ad,
                             &w->samples[0]+first_samples,
                             w->num_samples-first_samples)
            <= 0)
        {   // On error try again on next iteration
            ;
        }
        else
        {   // On success dequeue the element which has been successfully
            // played and free it.
            elmt = bell_dequeue(*p_q);
            free_q_elmt(elmt);
        }
    }
    else
    {
        // sleep while waiting on input for queue
        sleepdelay.tv_sec=0;
        sleepdelay.tv_nsec=10000000;  // 10ms delay
        nanosleep(&sleepdelay,NULL);
    }
    return ad;
}

int audio_scheduler()
{
//  Start an audio scheduler process
    int fd[2];               //Pipe for audio_scheduler
    int retval = -1;         //file descriptor for pipe to scheduler
    pid_t pid;               //PID for audio_scheduler
    bell_queue *q=NULL;
    int stopreading = FALSE;
    cst_audiodev *ad = NULL;

    if (pipe(fd) != 0)
    {
        cst_errmsg("audio_scheduler: failed to create pipe\n");
    }
    pid=fork();
    if (0 == pid)
    {
//  This is the child audio scheduler process
        while(FALSE == stopreading || (TRUE == stopreading && BELL_QUEUE_LEN(q)>0 ) )
        {
            q = check_new_wave(q, &stopreading, fd);
            ad = audio_dispatch(&q, ad, &stopreading, fd);
        }
        // Audio scheduler has been commanded to end -
        // Clean up and drain last sound. Then exit.
        free_bell_queue(q);
        if (audio_close_alsa(ad) < 0)
        {
            cst_errmsg("audio scheduler: failed to close audio device.\n");
        }
        _exit(0);
    }
    else if (pid > 0)
    {
//  This is the parent process
        retval = fd[1]; // return value allows comunicating with
                        // audio scheduler via a pipe
    }
    else
    {
        cst_errmsg("audio_scheduler: failed to fork scheduler process");
    }

    return retval;
}

void audio_scheduler_close(int fd)
{
//  Close audio scheduler process
    int status;

    if (write(fd,"E",2)!=2)       // Send command to audio scheduler to exit
    {
        cst_errmsg("audio_scheduler_close:failed to send exit command\n");
    }
    waitpid(-1,&status,0);
}
#endif // CST_AUDIO_ALSA
