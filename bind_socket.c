#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "bind_socket.h"

/**
	Bind to the current_socket descriptor and listen to the port in PORT
**/
void bind_socket(struct sockaddr_in *address, int current_socket, int port)
{
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_ANY;
	address->sin_port = htons(port);

	if ( bind(current_socket, (struct sockaddr *)address, sizeof(*address)) < 0 )
	{
		perror("Bind to port");
		exit(-1);
	}
}


