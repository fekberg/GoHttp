#ifndef BIND_SOCKET_H
#define BIND_SOCKET_H

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

void bind_socket(struct sockaddr_in *address, int current_socket, int port);

#endif // BIND_SOCKET_H
