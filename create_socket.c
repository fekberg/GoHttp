#include "create_socket.h"

void create_socket(int *current_socket)
{
	*current_socket = socket(AF_INET, SOCK_STREAM, 0);

	if ( *current_socket == -1 )
	{
		perror("Create socket");
		exit(-1);
	}
}
