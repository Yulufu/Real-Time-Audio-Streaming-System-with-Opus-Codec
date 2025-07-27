#ifndef _AUDIO_H_
#define _AUDIO_H_

#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 960 //for Opus
#define FRAME_SIZE FRAMES_PER_BUFFER
#define CHANNELS 2
#define APPLICATION OPUS_APPLICATION_AUDIO
#define MAX_FRAME_SIZE 6*FRAME_SIZE

#define SYNCH_WORD 0x55abd5aa	//32-bit synch word
#define SEARCH_CNT	3

/* audio pkt header
 * header is 8 bytes long
 *
 * 0, 1, 2, 3 	synch word
 * 4 			pkt type
 * 5 			pkt sequence number
 * 6, 7			audio data length
 */
#define AH_LEN	8

#endif // _AUDIO_H_