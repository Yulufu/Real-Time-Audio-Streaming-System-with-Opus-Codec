#ifndef _SF_UTILS_H_
#define _SF_UTILS_H_

class SF_Utils
{
public:
    SF_Utils();
    ~SF_Utils();

    void open_sf(char *ifile);
    void read_sf_buf(float *inbuf, int n);
private:
    /* libsndfile structures */
    SNDFILE *isndfile; 
    SF_INFO isfinfo;
    int channels;
    int frames;
    /* pointers to audio buffer */
    float *bottom;
    float *next;
    float *top;
};

#endif //_SF_UTILS_H_