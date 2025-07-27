#include <stdio.h>
#include <stdlib.h>     /* malloc, free */
#include <string.h>     /* memset */
#include <sndfile.h>    /* libsndfile */
#include "sfUtils.h"

SF_Utils::SF_Utils()
{
    ;
}

SF_Utils::~SF_Utils() 
{
    free(bottom);
}

void SF_Utils::open_sf(char *ifile) 
{
    int count;

    /* zero libsndfile structures */
    memset(&isfinfo, 0, sizeof(isfinfo));

    /* open input WAV file */
    if ( (isndfile = sf_open (ifile, SFM_READ, &isfinfo)) == NULL ) {
        fprintf (stderr, "Error: could not open wav file: %s\n", ifile);
        exit(-1);
    }
    /* Print input file parameters */
    printf("Input audio file %s: Frames: %d Channels: %d Samplerate: %d\n", 
        ifile, (int)isfinfo.frames, isfinfo.channels, isfinfo.samplerate);

    /* malloc storage for signal */
    if ( (bottom = (float*)malloc(
    	isfinfo.channels * isfinfo.frames * sizeof(float) 
    	)) == NULL) {
    	fprintf(stderr, "Error: cannot malloc storage\n");
    	exit(-1);
    }

    /* read signal */
    if ( (count = sf_readf_float(isndfile, bottom, isfinfo.frames)) != 
        isfinfo.frames ) {
        fprintf(stderr, "Error: cannot read audio data\n");
        exit(-1);
    }

    /* set private data */
    channels = isfinfo.channels;
    frames = isfinfo.frames;
    next = bottom;
    top = bottom + isfinfo.channels*(isfinfo.frames-1);
}

void SF_Utils::read_sf_buf(float *inbuf, int buf_frames) {
	int k=0;
	for (int i=0; i<buf_frames; i++) {
		for (int j=0; j<channels; j++) {
			inbuf[k++] = *next++;
		}
		if (next > top) {
			next = bottom;
		}
	}
}
