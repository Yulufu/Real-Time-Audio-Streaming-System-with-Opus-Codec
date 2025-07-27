#include <stdio.h>
#include <stdlib.h>     /* malloc, free */
#include "audio.h"
#include "circular_buf.h"

#include <stdbool.h>

/* Circular buffer
 * 
 * buffer is contained in array cbuf[]
 * write to tail
 * read from head
 *
 * top is last element of cbuf (FIFO_MAX*AUDIO_PKT_LEN)
 * bottom is first element of cbuf (0)
 * tail is index of next (empty) byte in cbuf to write
 * head is index of next (full) byte in cbuf to read
 *
 * if head == tail cbuf is empty
 */

CircularBuf::CircularBuf(int n)
{
	top = n-1;
	bottom = 0;
	head = tail = bottom;
	if ( (cbuf = (unsigned char *)
		malloc(n*sizeof(unsigned char))) == NULL) {
		fprintf(stderr, 
			"ERROR: malloc failed in CircularBuf constructor\n");
	}
}

CircularBuf::~CircularBuf()
{
	free(cbuf);
}

/* write tcp pkt to cbuf starting at tail position
 *
 * returns "length" of buffer 
 */
int CircularBuf::circular_write_tcp_pkt(unsigned char *buff, int n)
{
	for (int i=0; i<n; i++) {
		cbuf[tail] = buff[i];
		tail++;
		if (tail > top) {
			tail = bottom;
		}
		// maybe don't have to check for full
		// if (head == tail) {
		// 	/* buffer full, so 
		// 	 * move head to make tail point to empty element
		// 	 */
		// 	head++;
		// 	if (head > top) {
		// 		head = bottom;
		// 	}
		// }
	}
	return circular_buf_len();
}

/* read audio pkt from cbuf starting at head position
 *
 * returns number of bytes read 
 * returns 0 if fifo is empty
 */
int CircularBuf::circular_read_audio_pkt(unsigned char *buff)
{
	int n;
	if (head == tail) {
		/* fifo empty */
		return 0;
	}
	if (!search_for_synch()) {
		printf("Cannot find synch word\n");
		return 0;
	}
	/* head now points to first byte of synch word 
	 * length of header is 8 bytes
	 */
	n = (cbuf[head+6]<<8 | cbuf[head+7]); //len of audio data
	n += AH_LEN; //lenth of header plus audio pkt
	for (int i=0; i<n; i++) {
		buff[i] = cbuf[head];
		head++;
		if (head > top) {
			head = bottom;
		}
		// check for empty
		if (head == tail) {
			/* fifo empty */
			return 0;
		}
	}
	return n;
}

/* returns circular buffer in length of "nominal" audio pkts
 */
int CircularBuf::circular_buf_len(void)
{
	int t = tail;
	int h = head;
	if (t < h) {
		t += (top+1); //+ length of cbuf
	}
	return ((t-h)/AUDIO_PKT_LEN);
}

/* returns true if cbuf[head] is start of SYNCH_WORD
 * otherwise, increments head by one until SYNCH_WORD is found
 * then uses audio_pkt_len to find 3 next SYNCH_WORD
 * then returns true
 * if no synch found returns false
 */

/* audio pkt header
 * header is 8 bytes long
 *
 * 0, 1, 2, 3 	synch word
 * 4 			pkt type
 * 5 			pkt sequence number
 * 6, 7			audio data length
 */

bool CircularBuf::search_for_synch(void)
{
	/* read from head */
	int i = head;
	long synch;
	synch = cbuf[i]<<24 | cbuf[i+1]<<16 | cbuf[i+2]<<8 | cbuf[i+3];
	if (synch == SYNCH_WORD) {
		//maintaining synchronization, so return true
		return true;
	}

	//otherwise search for SYNCH_WORD
	i++;
	int search_range = 5*AUDIO_PKT_LEN; //for example
	printf("Searching for synch\n");
	for (int j=0; j<search_range; j++) {
		synch = cbuf[i]<<24 | cbuf[i+1]<<16 | cbuf[i+2]<<8 | cbuf[i+3];
		if (synch == SYNCH_WORD) {
			printf("Found synch. Skipped %d bytes\n", j);
			head = i;
			return true;
		}
		else {
			i++;
			if (i>top) {
				i = bottom;
			}
		}
	}
	//searched over searcg range and did not find synch
	return false;
}
