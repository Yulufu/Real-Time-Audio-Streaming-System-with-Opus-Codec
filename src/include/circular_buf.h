#ifndef _CIRCULARBUF_H_
#define _CIRCULARBUF_H_

#define CBUF_MAX    150
#define BUF_HI     100
#define BUF_LO     5
#define AUDIO_PKT_LEN   512 //nominal audio pkt len, bytes

class CircularBuf
{
public:
    CircularBuf(int n);
    ~CircularBuf();

    int circular_write_tcp_pkt(unsigned char *buff, int n);
    int circular_read_audio_pkt(unsigned char *buf);
    int circular_buf_len(void);
    bool search_for_synch(void);
private:
    unsigned char *cbuf;
    int head, tail;
    int top, bottom;
};
#endif //_CIRCULARBUF_H_