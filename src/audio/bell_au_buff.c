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
#include <sys/types.h> // for pid_t,WNOHANG
#include <sys/wait.h>  // for waitpid()
#include <unistd.h>    // for fork(),pipe()
#include <time.h>      // for nanosleep()

#include "cst_alloc.h"
#include "cst_error.h"
#include "bell_audio.h"
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

static pid_t send_wave(bell_q_elmt *elmt,bell_queue *queue)
{
//  Start a process for playing a cst_wave
    pid_t pid; //PID for play_wave output

    pid=fork();
    if (0 == pid)
    {
//  This is the child audio playing process
        play_wave(elmt->content);
        free_q_elmt(elmt);
//  clean up queue in child since child processes is exiting here
        free_bell_queue(queue);
        _exit(0);
    }
    else if (pid > 0)
    {
//  This is the parent process
        ;
    }
    else
    {
        cst_errmsg("send_wave: failed to fork audio play process");
        return -1;
    }

    return pid;
}

static pid_t audio_dispatch(pid_t audio_play_pid, bell_queue *q)
{
//  Send cst_wave for playing if nothing is currently playing
//  Return PID for currently audio process currently playing
    int status;
    bell_q_elmt *elmt;

    if ( (waitpid(audio_play_pid,&status,WNOHANG) != 0) &&
         ((elmt=bell_dequeue(q)) != NULL) )
// first subcondition of if statement checks if audio has finished playing
// second subcondition checks if more audio is available in the queue to play
    {
        audio_play_pid=send_wave(elmt,q);
        if (-1 == audio_play_pid)
        {
            // On fork() error re-queue wave and try again on next iteration
            // Ignore return value since we already have a valid queue
            // as we have dequeued from it just above
            bell_enqueue(elmt->content,q);
            cst_free(elmt);  // Free queue element structure but not cst_wave
                             // since we have requeued the cst_wave
        }
        else
        {
            free_q_elmt(elmt);
        }
    }
    return audio_play_pid;
}

int audio_scheduler()
{
//  Start an audio scheduler process
    int fd[2];               //Pipe for audio_scheduler
    fd_set rfds;             //file descriptor set for select()
    int retval = -1;         //file descriptor for pipe to scheduler
    pid_t pid;               //PID for audio_scheduler
    pid_t audio_play_pid=-1; //PID for current audio playing process
    int status;
    bell_queue *q=NULL;
    char readbuffer[2];
    int stopreading = FALSE;
    struct timespec sleepdelay;
    struct timeval  selecttime;
    cst_wave *w;

    sleepdelay.tv_sec=0;
    sleepdelay.tv_nsec=100000000; // 100ms delay

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
            if (FALSE == stopreading && (BELL_QUEUE_LEN(q) < BELL_MAX_QUEUE_LEN) )
            {
                FD_ZERO(&rfds);
                FD_SET(fd[0],&rfds);
                selecttime.tv_sec = 0;
                selecttime.tv_usec = 0;
                if (select(fd[0]+1, &rfds, NULL, NULL, &selecttime) > 0)
                {  // Only read if data is available from main process
                    read(fd[0],readbuffer,2);
                    if (cst_streq("W",readbuffer)) // About to receive a cst_wave
                    {
                        w=new_wave();
                        deserialize_wave(w,fd[0]);
                        q=bell_enqueue(w,q);
                    }
                    if (cst_streq("E",readbuffer)) // No more cst_wave to receive
                    {
                        stopreading=TRUE;
                    }
                }
            }
            audio_play_pid=audio_dispatch(audio_play_pid,q);
            nanosleep(&sleepdelay,NULL); // sleep while waiting on input or output
        }

        free_bell_queue(q);
        waitpid(audio_play_pid,&status,0); // Wait for child audio play process
                                           // to exit
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
