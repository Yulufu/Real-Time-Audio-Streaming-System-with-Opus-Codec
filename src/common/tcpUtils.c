#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>     /* exit() */
#include <string.h>     /* memset() */
/* for socket functions */
#include <unistd.h> 
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
/* application */
#include "pkt_types.h"
#include "audio.h"

#define SA struct sockaddr
#define BACKLOG 5

/* This function is typically used by Server
 * 
 * Create TCP socket 
 * bind to port
 * and listen on that Port for client
 */
int listen_for_client(int port, int reuse)
{
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("server: socket");
        exit(0);
    }
    else {
        printf("TCP socket successfully created\n");
    }

    // set to reuse socket (e.g. for client connecttions)
    if (reuse == 1) {
        int yes=1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
             sizeof(int)) == -1) {
         perror("setsockopt");
         exit(0);
        }
    }

    // zero struct
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        perror("server: bind");
        exit(0);
    }
    else {
        printf("TCP socket bind successful\n");
    }

    // Now server is ready to listen
    if ((listen(sockfd, BACKLOG)) != 0) {
        perror("listen");
        exit(0);
    }
    else {
        printf("TCP server listening on port %d\n", port);
    }

    return sockfd;
}

/* This function is typically used by Client
 * 
 * translate server name to server IP address
 * create socket and connect to server
 * server must be listening for the client connection request
 * 
 * returns socket file descriptor
 */
int connect_to_server(char *server_ip_addr, char *server_port, 
    int *psockfd)
{
    int sockfd, status;
    struct addrinfo hints, *res, *p;

    // assign IP, PORT
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if ((status = getaddrinfo(server_ip_addr, server_port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return(-1);
    }
    else {
        printf("getaddrinfo success\n");    
    }

    // socket create and connect to server
    for (p=res; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue; //try next entry in list
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue; //try next entry in list
        }
        // Connected succesfully!
        printf("connected to the server\n");
        break;
    }
    if (p == NULL) {
        // The loop wasn't able to connect to the server
        fprintf(stderr, "couldn't connect to the server\n.");
        return(-1);
    }
        
    freeaddrinfo(res); // free the linked list

    //return socket fd
    *psockfd = sockfd;

    return(1);
}

/* local function */

/* get sockaddr, IPv4 or IPv6 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Get IP address of connecting Client 
 * input is sockaddr structure
 * output is IP address as string
 */
void sockaddr_to_ip(struct sockaddr *addr, char *ip_str)
{
    char s[INET6_ADDRSTRLEN];
    inet_ntop(addr->sa_family,
        get_in_addr( (struct sockaddr *)addr),
        s, sizeof(s) );
    
    strcpy(ip_str, s);
}

/* first verify that pkt begins with synch word
 * if not, then return -1 (not found)
 * otherwise, return pkt type
 */
int ck_pkt_type(unsigned char *buff)
{
    int i, pkt_type;
    long synch;
    i=0;
    synch = buff[i]<<24 | buff[i+1]<<16 | buff[i+2]<<8 | buff[i+3];
    if (synch == SYNCH_WORD) {
        pkt_type = buff[i+4];
    }
    else {
        pkt_type = -1;
    }
    return pkt_type;
}

#ifdef __cplusplus
} // extern "C"
#endif