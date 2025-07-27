#ifndef _TCPUTILS_H_
#define _TCPUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

int listen_for_client(int port, int reuse);
int connect_to_server(char *server_ip_addr, char *server_port, 
	int *sockfd);
void sockaddr_to_ip(struct sockaddr *addr, char *ip_str);
int ck_pkt_type(unsigned char *buff);
void ck_seq_num(unsigned char *buff, unsigned char *ppsn);

#ifdef __cplusplus
}
#endif

#endif //#ifndef _TCPUTILS_H_
