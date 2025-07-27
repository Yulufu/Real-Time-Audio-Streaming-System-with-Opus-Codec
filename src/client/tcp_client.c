/*****************************************************************************
 * 
 * Client
 * Connect to Server
 * Receive TCP packet from server
 * Decode with Opus and write to PortAudio output buffer
 *
 * Main thread connects to Server, starts PortAudio and
 * receives pkts from Server and writes to FIFO buffer 
 * 
 * PortAudio callback reads pkts from FIFO buffer, 
 * decodes to audio and writes to output buffer
 * 
 *****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>     /* exit() */
/* for socket functions */
#include <unistd.h> 
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
/* POSIX threads */
#include <pthread.h> 
/* for audio functions */
#include <errno.h>
#include <string.h>     /* memset() */
#include <ctype.h>      /* tolower() */
#include <sndfile.h>    /* libsndfile */
#include <portaudio.h>  /* portaudio */
// #include <opus.h>       /* opus */
#include <opus/opus.h>  // just for my system
/* application includes */   
#include "pkt_types.h"
#include "tcpUtils.h"
#include "paUtils.h"
#include "circular_buf.h"
#include "audio.h"

#define SA struct sockaddr

#include <stdbool.h>

/* Callback structure */
typedef struct {
    int samplerate;
    int channels;
    int frames;
    /*Holds the state of the decoder */
    OpusDecoder *decoder;
    /* curcular buffer */
    CircularBuf *pcb;
    /* socket */
    int sockfd;
    int block;
    unsigned char seq_num;
    bool playing;
} CBdata;

struct RecvData {
    int sockfd;
    CircularBuf  *pcb;
    CBdata *pcbd;
};

/* Mutex for synchronizing FIFO */
pthread_mutex_t mutex_ff = PTHREAD_MUTEX_INITIALIZER;

/* local function prototypes */
void *recv_from_server(void *data); 
int paCallback(const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData);


int main(int argc, char *argv[])
{
	/* command line parameters */
	char *server_name;
	char *server_port;
    /* portaudio */
    PaStream *stream;
    CBdata cb_data;
    /* socket */
	int sockfd;
	int err;
    int n;
    /* circular buffer */
    CircularBuf circular_buf(CBUF_MAX*AUDIO_PKT_LEN);
    CircularBuf *pcb = &circular_buf;
    struct RecvData recv_data;
    /* POSIX threads */
    int rc1;
    pthread_t thread1;

    /* parse command line */
//Your code here
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_name> <server_port>\n", argv[0]);
        return 1;
    }

    server_name = argv[1];
    server_port = argv[2];

    /* default values for audio output */
    cb_data.samplerate = SAMPLE_RATE;
    cb_data.channels = CHANNELS;
    // cb_data.channels = 2;
    cb_data.frames = FRAMES_PER_BUFFER;
    cb_data.block = 0;
    /* curcular buffer and socket */
    cb_data.pcb = pcb;
    cb_data.playing = false; //not playing
    cb_data.seq_num = 0;

    /* create a new Opus decoder state
     * start Port Audio
     * create connection to server
     */
//Your code here
    /* Opus decoder */
   cb_data.decoder = opus_decoder_create(SAMPLE_RATE, 1 , &err);
   if (err<0) {
       fprintf(stderr, "Failed to create decoder: %s\n", opus_strerror(err));
       return(-1);
    }

    /* Start Port Audio */ /* From paUtils.c */
    // int inputChanCount = getDefaultInputChannelCount();

    stream = startupPa(1, 1, cb_data.samplerate, cb_data.frames, paCallback, &cb_data);

    if (!stream) {
        fprintf(stderr, "Error: Cannot start PortAudio\n");
        return 1;
    }

    /* connect to server */
    int connection_status = connect_to_server(server_name, server_port, &sockfd);

    if (connection_status < 0) {
        fprintf(stderr, "couldn't connect to the server\n.");
        return(-1);
    }




    /* start thread
     * to listen for TCP packets from server 
     */
    recv_data.sockfd = sockfd;
    recv_data.pcb = pcb;
    recv_data.pcbd = &cb_data;
    if ( (rc1 = pthread_create( &thread1, NULL, &recv_from_server, 
        &recv_data)) ) {
        printf("Thread creation failed: %d\n", rc1);
        exit(0);
    }

    cb_data.sockfd = sockfd;

    


    /* wait for keyboard entry 
     * portaudio callback and recv_from_server 
     * each run in separate threads
     */
    unsigned char d_msg[] = {
        (SYNCH_WORD>>24)&0xff,
        (SYNCH_WORD>>16)&0xff,
        (SYNCH_WORD>> 8)&0xff,
        (SYNCH_WORD>> 0)&0xff,
        PKT_CLIENT_DISCONNECT, 0, 0, 0};
    printf("Enter Q to quit, any other character for status\n");
    while (1) {
        int ch = getchar();
        if (tolower(ch) == 'q') {
            printf("Send disconnect message to Server\n");
            if ( (n = send(sockfd, d_msg, sizeof(d_msg), 0)) < 0) {
                perror("send");
                exit(1);
            }
            break;
        }
        printf("%d", cb_data.block);
    }

    /* shut down Port Audio, Opus decoder
     * kill thread and free mutex
     * close TCP socket
     */
//Your code here
    shutdownPa(stream);
    opus_decoder_destroy(cb_data.decoder);
    pthread_cancel(thread1);
    close(sockfd);

	return(0);
}

/* this routine runs in its own thread
 * It receives pkts from server and writes them
 * to FIFO buffer
 */
void *recv_from_server(void *data)
{
    unsigned char rcbits[MAX_PACKET_SIZE];
    struct RecvData *pd = (struct RecvData*)data;

    int nrcv;
    int sockfd = pd->sockfd;
    CircularBuf *pcb = pd->pcb;
    CBdata *pcbd = pd->pcbd;
    int cbuf_len;

    /* fill TCP jitter buffer */
    while (1) {
        /* get ptk from server */
        if ( (nrcv = recv(sockfd, rcbits, sizeof(rcbits), 0)) < 0) {
            perror("recv");
            exit(1);
        }
        if (nrcv == 0) {
            printf("Connection closed by Server\n");
            exit(1);
        }
        if (nrcv > 0) {
            /* lock out other threads */
            pthread_mutex_lock( &mutex_ff );
            /* write tcp pkt to circular buffer */
            cbuf_len = pcb->circular_write_tcp_pkt(rcbits, nrcv);
            /* unlock */
            pthread_mutex_unlock( &mutex_ff );
        }
        if (pcbd->block%50 == 0) {
            printf("CBUF len %3d %s\n", cbuf_len,
                pcbd->playing ? "Playing" : "Not Playing");
        }
    }
    printf("ERROR: unexpected return\n");
    return 0;
}

/* This routine will be called by the PortAudio engine when audio is needed.
 * It may called at interrupt level on some machines so don't do anything
 * in the routine that would block or stall processing.
 */
int paCallback(const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    /* Cast data passed through stream to our structure. */
    CBdata *p = (CBdata *)userData;
    //float *input  = (float *)inputBuffer;
    float *output = (float *)outputBuffer;
    unsigned char rcbits[MAX_PACKET_SIZE];
    int nbBytes;
    int n;
    /* local values */
    int cbuf_len;
    bool decode_fail = false;


    /* get status (length) of FIFO buffer 
     * be sure to put mutex lock before and mutex unlock after
     * the call to circular_buf_len()
     *
     * if playing and fifo is below low water mark, set playing flag
     * if not playing and fifo is above high water mark, clear playing flag
     * 
     * if playing flag is set, get audio pkt by calling 
     * circular_read_audio_pkt(rcbits). Use mutex lock and unlock
     * before and after this call
     *
     * Check sequence number in audio pkt header
     * 
     * Decode audio pkt to output buffer.  
     * If play flag is false or if there is a decoding failure 
     * then zero output buffer
     */
//Your code here

    /* Set and update playing flag */
    const int low_water_mark = 20;
    const int high_water_mark = 100;

    /* Get buffer status */
    pthread_mutex_lock( &mutex_ff );
    cbuf_len = p->pcb->circular_buf_len();
    pthread_mutex_unlock( &mutex_ff );


    /* Update the playing flag based on buffer length and watermarks */
    if (p->playing && cbuf_len < low_water_mark) {
        p->playing = false;
    } else if (!p->playing && cbuf_len > high_water_mark) {
        p->playing = true;
    }


    /* Read packet from buffer if playing */
    if (p->playing) {
        pthread_mutex_lock(&mutex_ff);
        n = p->pcb->circular_read_audio_pkt(rcbits);
        pthread_mutex_unlock(&mutex_ff);

        /* if packet read succeed and sequence number matches */
        if (n > 0 && rcbits[5] == p->seq_num) {
            p->seq_num++;

            /* Decode audio packet */
            nbBytes = opus_decode_float(p->decoder, rcbits + AH_LEN, n - AH_LEN, output, framesPerBuffer, 0);
            if (nbBytes < 0) {
                fprintf(stderr, "Decoding error: %s\n", opus_strerror(nbBytes));
                decode_fail = true;
            }
        } else {
            decode_fail = true;
        }
    } else {
        decode_fail = true;
    }

    /* output 0 if decode fail */
    if (decode_fail) {
        memset(output, 0, framesPerBuffer * p->channels * sizeof(float));
    }
    
    return paContinue;


}

#ifdef __cplusplus
} // extern "C"
#endif
