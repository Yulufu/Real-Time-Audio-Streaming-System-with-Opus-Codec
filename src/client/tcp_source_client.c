/*****************************************************************************
 * 
 * Source Client
 * Connect to Server
 * Read from PortAudio input buffer or audio buffer
 * Encode with and Opus and send packet to Server
 * using TCP protocol
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
/* for audio functions */
#include <errno.h>
#include <string.h>     /* memset() */
#include <ctype.h>      /* tolower() */
#include <sndfile.h>    /* libsndfile */
#include <portaudio.h>  /* portaudio */
// #include <opus.h>       /* opus */
#include "opus/opus.h" // my opus is located in a sub-directory
#include "audio.h"
#include "pkt_types.h"
#include "tcpUtils.h"
#include "paUtils.h"
#include "sfUtils.h"

#define SA struct sockaddr


/* Callback structure */
typedef struct {
    int channels;
    /*Holds the state of the encoder */
    OpusEncoder *encoder;
    /* socket */
    int sockfd;
    int block;
    unsigned char seq_num;
    /* libsndfile */
    SF_Utils *p_sf_utils;
} CBdata;

int main(int argc, char *argv[])
{
	/* command line parameters */
	int bitrate;
	char *server_name;
	char *server_port;
    /* portaudio */
    PaStream *stream;
    CBdata cb_data;
    /* socket */
	int sockfd;
	int err;
    unsigned char d_msg[] = {
        (SYNCH_WORD>>24)&0xFF,
        (SYNCH_WORD>>16)&0xFF,
        (SYNCH_WORD>>8)&0xFF,
        (SYNCH_WORD)&0xFF,
        PKT_CLIENT_DISCONNECT, 0, 0, 0};
    /*libsndfile */
    SF_Utils sf_utils;
    int input_channels;

    /* parse command line arguments
     * open input WAV file using sfUtils (if needed)
     * setup Opus encoder
     * open socket to server using tcpUtils
     * start Port Audio using paUtils
     */
    /* wait for keyboard entry 
     * portaudio callback runs in another thread
     */

    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: %s <bitrate> <server_name> <server_port> [ifile.wav]\n", argv[0]);
        return 1;
    }

    bitrate = atoi(argv[1]);
    server_name = argv[2];
    server_port = argv[3];

    /* If a WAV exist */
    /* From Professor's code */
    if (argc == 5) {
        sf_utils.open_sf(argv[4]);
        cb_data.p_sf_utils = &sf_utils;
        input_channels = 1; // default microphone input
    } else {
        cb_data.p_sf_utils = NULL;
        input_channels = 2; // from digital interface
    }

    /* Default values for audio */
    cb_data.channels = CHANNELS; 
    cb_data.block = 0;

    /* Setup Opus encoder */
    cb_data.encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS /* NUM_CHAN */, OPUS_APPLICATION_AUDIO /* APPLICATION */, &err);
    if (err < 0) {
        fprintf(stderr, "Failed to create an encoder: %s\n", opus_strerror(err));
        return -1;
    }

    /* Connect to server test program */
    /* If failed */
    if (connect_to_server(server_name, server_port, &sockfd) != 1) {
        fprintf(stderr, "Failed to connect to server\n");
        return -1;
    }

    /* When successfully connected */
    cb_data.sockfd = sockfd;

    /* Start Port Audio */
    stream = startupPa(input_channels, cb_data.channels, SAMPLE_RATE, FRAMES_PER_BUFFER, paCallback, &cb_data);

    // stream = startupPa(input_channels, 0, SAMPLE_RATE, FRAMES_PER_BUFFER, paCallback, &cb_data);
    if (stream == NULL) {
        fprintf(stderr, "Failed to start Port Audio\n");
        return -1;
    }

    /* Provided loop and wait for keyboard entry */
	printf("Enter Q to quit, any other character for status\n");
	while (1) {
		int ch = getchar();
		if (tolower(ch) == 'q') {
            printf("Send disconnect message to Server\n");
            send(sockfd, d_msg, sizeof(d_msg), 0);
			break;
		}
		printf("%d", cb_data.block);
	}

    /* shut down Port Audio and Opus encoder
     * close TCP socket
     */

    shutdownPa(stream);
    opus_encoder_destroy(cb_data.encoder);
    close(sockfd);

	return(0);
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
    float *input  = (float *)inputBuffer;
    float *output = (float *)outputBuffer;
    unsigned char tcbits[MAX_PACKET_SIZE];
    int nbBytes;
    int i, n, nsnd;

    if (p->p_sf_utils) {
        /* copy input audio into input buffer */
        p->p_sf_utils->read_sf_buf(input, framesPerBuffer);
    }

    /* encode audio in input buffer */

    nbBytes = opus_encode_float(p->encoder, input, framesPerBuffer, tcbits + AH_LEN, MAX_PACKET_SIZE - AH_LEN);
    if (nbBytes < 0) {
        fprintf(stderr, "opus_encode() failed: %s\n", opus_strerror(nbBytes));
        return -1;
    }

    /* audio pkt header
     * header is 8 bytes long
     *
     * 0, 1, 2, 3   synch word
     * 4            pkt type
     * 5            pkt sequence number
     * 6, 7         audio data length
     */

    i=0;
    /* insert synch word */
    tcbits[i++] = (SYNCH_WORD>>24)&0xFF;
    tcbits[i++] = (SYNCH_WORD>>16)&0xFF;
    tcbits[i++] = (SYNCH_WORD>>8)&0xFF;
    tcbits[i++] = (SYNCH_WORD)&0xFF;
    /* insert pkt type */
    tcbits[i++] = PKT_AUDIO_OPUS;
    /* insert pkt seq num */
    tcbits[i++] = p->seq_num++;
    /* insert audio data length */
    tcbits[i++] = (nbBytes>>8)&0x0FF; //MSB
    tcbits[i++] = nbBytes&0x0FF; //LSB

    /* send encoded audio to server in TCP packet
     * zero output buffer
     */

    if (send(p->sockfd, tcbits, nbBytes + AH_LEN, 0) == -1) {
        perror("send");
        exit(1);
    }

    /* zero output buffer */
    memset(output, 0, framesPerBuffer*sizeof(float) * p->channels);


    return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif
