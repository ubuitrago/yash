#ifndef SERVE_TCPSOCKS_H
#define SERVE_TCPSOCKS_H

#include <sys/socket.h>
#include <netinet/in.h>

#define MAXHOSTNAME 80
#define MAXCONNECTIONS 20

void reusePort(int sock);
void EchoServe(int psd, struct sockaddr_in from);

struct ClInfo {
    int sock;
    struct sockaddr_in from;
};

int serve_inet_socket();
void *serve_yash(void * input);

#endif