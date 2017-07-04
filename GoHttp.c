//
// GoHttp is purely written for educational purposes
// DO NOT USE IN PRODUCTION!
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "daemonize.h"
#include "create_socket.h"
#include "bind_socket.h"
#include "http_utils.h"

#define MAX_FILE_SIZE 5*1024
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int run_daemon = FALSE;
char *conf_file;
char *log_file;
int port;

struct sockaddr_in address;
struct sockaddr_storage connector;
int current_socket;
int connecting_socket;
socklen_t addr_size;

void start()
{
	create_socket(&current_socket);

	bind_socket(&address, current_socket, port);

	startListener(current_socket);

	while ( 1 )
	{
        accept_connection(&current_socket, &connecting_socket, &addr_size, &connector);
	}
}

int main(int argc, char* argv[])
{
	int parameterCount;

	init(run_daemon, &port, &conf_file, &log_file);

	for (parameterCount = 1; parameterCount < argc; parameterCount++)
	{
		// If flag -p is used, set port
		if (strcmp(argv[parameterCount], "-p") == 0)
		{
			// Indicate that we want to jump over the next parameter
			parameterCount++;
			printf("Setting port to %i\n", atoi(argv[parameterCount]));
			port = atoi(argv[parameterCount]);
		}

		// If flag -d is used, set run_daemon to TRUE;
		else if (strcmp(argv[parameterCount], "-d") == 0)
		{ 
			printf("Setting run_daemon = TRUE");
			run_daemon = TRUE;
		}

		else if (strcmp(argv[parameterCount], "-l") == 0)
		{
			// Indicate that we want to jump over the next parameter
			parameterCount++;
			printf("Setting logfile = %s\n", argv[parameterCount]);
			log_file = (char*)argv[parameterCount];
		}
		else
		{
			printf("Usage: %s [-p port] [-d] [-l logfile]\n", argv[0]);
			printf("\t\t-p port\t\tWhich port to listen to.\n");
			printf("\t\t-d\t\tEnables run_daemon mode.\n");
			printf("\t\t-l logfile\tWhich file to store the log to.\n");
			return -1;
		}
	}

	printf("Settings:\n");
	printf("Port:\t\t\t%i\n", port);
	printf("Server root:\t\t%s\n", wwwroot);
	printf("Configuration file:\t%s\n", conf_file);
	printf("Logfile:\t\t%s\n", log_file);
	printf("Deamon:\t\t\t%i\n", run_daemon);

	if ( run_daemon == TRUE )
	{
		daemonize();
	}

	start();

	return 0;
}
