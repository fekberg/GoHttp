//
// GoHttp is purely written for educational purposes
// DO NOT USE IN PRODUCTION!
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/wait.h>

#define BUFFER_SIZE 512
#define MAX_FILE_SIZE 5*1024
#define MAX_CONNECTIONS 3
#define TRUE 1
#define FALSE 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int port;
int deamon = FALSE;
char *wwwroot;
char *conf_file;
char *log_file;
char *mime_file;

FILE *filePointer = NULL;

struct sockaddr_in address;
struct sockaddr_storage connector;
int current_socket;
int connecting_socket;
socklen_t addr_size;

static void daemonize(void)
{
	pid_t pid, sid;

	/* already a daemon */
	if ( getppid() == 1 ) return;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* At this point we are executing as the child process */

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory.  This prevents the current
	directory from being locked; hence not being able to remove it. */
	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	/* Redirect standard files to /dev/null */
	// freopen( "/dev/null", "r", stdin);
	// freopen( "/dev/null", "w", stdout);
	// freopen( "/dev/null", "w", stderr);
}

sendString(char *message, int socket)
{
	int length, bytes_sent;
	length = strlen(message);

	bytes_sent = send(socket, message, length, 0);

	return bytes_sent;
}

int sendBinary(int *byte, int length)
{
	int bytes_sent;

	bytes_sent = send(connecting_socket, byte, length, 0);

	return bytes_sent;


	return 0;
}
void sendHeader(char *Status_code, char *Content_Type, int TotalSize, int socket)
{
	char *head = "\r\nHTTP/1.1 ";
	char *content_head = "\r\nContent-Type: ";
	char *server_head = "\r\nServer: PT06";
	char *length_head = "\r\nContent-Length: ";
	char *date_head = "\r\nDate: ";
	char *newline = "\r\n";
	char contentLength[100];

	time_t rawtime;

	time ( &rawtime );

	// int contentLength = strlen(HTML);
	sprintf(contentLength, "%i", TotalSize);

	char *message = malloc((
		strlen(head) +
		strlen(content_head) +
		strlen(server_head) +
		strlen(length_head) +
		strlen(date_head) +
		strlen(newline) +
		strlen(Status_code) +
		strlen(Content_Type) +
		strlen(contentLength) +
		28 +
		sizeof(char)) * 2);

	if ( message != NULL )
	{

		strcpy(message, head);

		strcat(message, Status_code);

		strcat(message, content_head);
		strcat(message, Content_Type);
		strcat(message, server_head);
		strcat(message, length_head);
		strcat(message, contentLength);
		strcat(message, date_head);
		strcat(message, (char*)ctime(&rawtime));
		strcat(message, newline);

		sendString(message, socket);

		free(message);
	}    
}

void sendHTML(char *statusCode, char *contentType, char *content, int size, int socket)
{
	sendHeader(statusCode, contentType, size, socket);
	sendString(content, socket);
}

void sendFile(FILE *fp, int file_size)
{
	int current_char = 0;

	do{
		current_char = fgetc(fp);
		sendBinary(&current_char, sizeof(char));
	}
	while(current_char != EOF);
}

int scan(char *input, char *output, int start, int max)
{
	if ( start >= strlen(input) )
		return -1;

	int appending_char_count = 0;
	int i = start;
	int count = 0;

	for ( ; i < strlen(input); i ++ )
	{
		if ( *(input + i) != '\t' && *(input + i) != ' ' && *(input + i) != '\n' && *(input + i) != '\r')
		{
			if (count < (max-1))
			{
				*(output + appending_char_count) = *(input + i ) ;
				appending_char_count += 1;

				count++;
			}		
		}	
		else
			break;
	}
	*(output + appending_char_count) = '\0';	

	// Find next word start
	i += 1;

	for (; i < strlen(input); i ++ )
	{
		if ( *(input + i ) != '\t' && *(input + i) != ' ' && *(input + i) != '\n' && *(input + i) != '\r')
			break;
	}

	return i;
}




int checkMime(char *extension, char *mime_type)
{
	char *current_word = malloc(600);
	char *word_holder = malloc(600);
	char *line = malloc(200);
	int startline = 0;

	FILE *mimeFile = fopen(mime_file, "r");

	free(mime_type);

	mime_type = (char*)malloc(200);

	memset (mime_type,'\0',200);

	while(fgets(line, 200, mimeFile) != NULL) { 

		if ( line[0] != '#' )
		{
			startline = scan(line, current_word, 0, 600);
			while ( 1 )
			{
				startline = scan(line, word_holder, startline, 600);
				if ( startline != -1 )
				{
					if ( strcmp ( word_holder, extension ) == 0 )
					{
						memcpy(mime_type, current_word, strlen(current_word));
						free(current_word);
						free(word_holder);
						free(line);
						return 1;	
					}
				}
				else
				{
					break;
				}
			}
		}

		memset (line,'\0',200);
	}

	free(current_word);
	free(word_holder);
	free(line);

	return 0;
}

int getHttpVersion(char *input, char *output)
{
	char *filename = malloc(100);
	int start = scan(input, filename, 4, 100);
	if ( start > 0 )
	{
		if ( scan(input, output, start, 20) )
		{

			output[strlen(output)+1] = '\0';

			if ( strcmp("HTTP/1.1" , output) == 0 )
				return 1;

			else if ( strcmp("HTTP/1.0", output) == 0 )

				return 0;
			else
				return -1;
		}
		else
			return -1;
	}

	return -1;
}

int GetExtension(char *input, char *output, int max)
{
	int in_position = 0;
	int appended_position = 0;
	int i = 0;
	int count = 0;

	for ( ; i < strlen(input); i ++ )
	{		
		if ( in_position == 1 )
		{
			if(count < max)
			{
				output[appended_position] = input[i];
				appended_position +=1;
				count++;
			}
		}

		if ( input[i] == '.' )
			in_position = 1;

	}

	output[appended_position+1] = '\0';

	if ( strlen(output) > 0 )
		return 1;

	return -1;
}

int Content_Lenght(FILE *fp)
{
	int filesize = 0;

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	rewind(fp);

	return filesize;
}

int handleHttpGET(char *input)
{
	// IF NOT EXISTS
	// RETURN -1
	// IF EXISTS
	// RETURN 1

	char *filename = (char*)malloc(200 * sizeof(char));
	char *path = (char*)malloc(1000 * sizeof(char));
	char *extension = (char*)malloc(10 * sizeof(char));
	char *mime = (char*)malloc(200 * sizeof(char));
	char *httpVersion = (char*)malloc(20 * sizeof(char));

	int contentLength = 0;
	int mimeSupported = 0;
	int fileNameLenght = 0;


	memset(path, '\0', 1000);
	memset(filename, '\0', 200);
	memset(extension, '\0', 10);
	memset(mime, '\0', 200);
	memset(httpVersion, '\0', 20);

	fileNameLenght = scan(input, filename, 5, 200);


	if ( fileNameLenght > 0 )
	{

		if ( getHttpVersion(input, httpVersion) != -1 )
		{
			FILE *fp;

			if ( GetExtension(filename, extension, 10) == -1 )
			{
				printf("File extension not existing");

				sendString("400 Bad Request\n", connecting_socket);

				free(filename);
				free(mime);
				free(path);
				free(extension);

				return -1;
			}

			mimeSupported =  checkMime(extension, mime);


			if ( mimeSupported != 1)
			{
				printf("Mime not supported");

				sendString("400 Bad Request\n", connecting_socket);

				free(filename);
				free(mime);
				free(path);
				free(extension);

				return -1;
			}

			// Open the requesting file as binary //

			strcpy(path, wwwroot);

			strcat(path, filename);			

			fp = fopen(path, "rb");

			if ( fp == NULL )
			{
				printf("Unable to open file");

				sendString("404 Not Found\n", connecting_socket);

				free(filename);
				free(mime);
				free(extension);
				free(path);

				return -1;
			}


			// Calculate Content Length //
			contentLength = Content_Lenght(fp);
			if (contentLength  < 0 )
			{
				printf("File size is zero");

				free(filename);
				free(mime);
				free(extension);
				free(path);

				fclose(fp);

				return -1;
			}

			// Send File Content //
			sendHeader("200 OK", mime,contentLength, connecting_socket);

			sendFile(fp, contentLength);

			free(filename);
			free(mime);
			free(extension);
			free(path);

			fclose(fp);

			return 1;
		}
		else
		{
			sendString("501 Not Implemented\n", connecting_socket);
		}
	}

	return -1;
}

int getRequestType(char *input)
{
	// IF NOT VALID REQUEST 
	// RETURN -1
	// IF VALID REQUEST
	// RETURN 1 IF GET
	// RETURN 2 IF HEAD
	// RETURN 0 IF NOT YET IMPLEMENTED

	int type = -1;

	if ( strlen ( input ) > 0 )
	{
		type = 1;
	}

	char *requestType = malloc(5);

	scan(input, requestType, 0, 5);

	if ( type == 1 && strcmp("GET", requestType) == 0)
	{
		type = 1;
	}
	else if (type == 1 && strcmp("HEAD", requestType) == 0)
	{
		type = 2;
	}
	else if (strlen(input) > 4 && strcmp("POST", requestType) == 0 )
	{
		type = 0;
	}
	else
	{
		type = -1;
	}
	return type;
}

int receive(int socket)
{
	int msgLen = 0;
	char buffer[BUFFER_SIZE];

	memset (buffer,'\0', BUFFER_SIZE);

	if ((msgLen = recv(socket, buffer, BUFFER_SIZE, 0)) == -1)
	{
		printf("Error handling incoming request");
		return -1;
	}

	int request = getRequestType(buffer);

	if ( request == 1 )				// GET
	{
		handleHttpGET(buffer);
	}
	else if ( request == 2 )		// HEAD
	{
		// SendHeader();
	}
	else if ( request == 0 )		// POST
	{
		sendString("501 Not Implemented\n", connecting_socket);
	}
	else							// GARBAGE
	{
		sendString("400 Bad Request\n", connecting_socket);
	}

	return 1;
}

/**
	Create a socket and assign current_socket to the descriptor
**/
void createSocket()
{
	current_socket = socket(AF_INET, SOCK_STREAM, 0);

	if ( current_socket == -1 )
	{
		perror("Create socket");
		exit(-1);
	}
}

/**
	Bind to the current_socket descriptor and listen to the port in PORT
**/
void bindSocket()
{
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if ( bind(current_socket, (struct sockaddr *)&address, sizeof(address)) < 0 )
	{
		perror("Bind to port");
		exit(-1);
	}
}

/**
	Start listening for connections and accept no more than MAX_CONNECTIONS in the Quee
**/
void startListener()
{
	if ( listen(current_socket, MAX_CONNECTIONS) < 0 )
	{
		perror("Listen on port");
		exit(-1);
	}
}

/**
Handles the current connector
**/
void handle(int socket)
{
	// --- Workflow --- //
	// 1. Receive ( recv() ) the GET / HEAD
	// 2. Process the request and see if the file exists
	// 3. Read the file content
	// 4. Send out with correct mine and http 1.1

	if (receive((int)socket) < 0)
	{
		perror("Receive");
		exit(-1);
	}
}

void acceptConnection()
{
	// signal(SIGCHLD, SIG_IGN);

	// int child_process = fork();

	addr_size = sizeof(connector);

	connecting_socket = accept(current_socket, (struct sockaddr *)&connector, &addr_size);


	if ( connecting_socket < 0 )
	{
		perror("Accepting sockets");
		exit(-1);
	}

	handle(connecting_socket);

	close(connecting_socket);
	/*
	if ( child_process == 0 )
	{
	exit(0);
	}

	while (-1 != waitpid (-1, NULL, WNOHANG));

	*/		// while (-1 != waitpid (-1, NULL, WNOHANG));

}

void start()
{
	createSocket();

	bindSocket();

	startListener();

	while ( 1 )
	{
		acceptConnection();
	}
}

void initConfiguration()
{

}

void init()
{
	char* currentLine = malloc(100);
	wwwroot = malloc(100);
	conf_file = malloc(100);
	log_file = malloc(100);
	mime_file = malloc(600);

	// Setting default values
	conf_file = "httpd.conf";
	log_file = ".log";
	strcpy(mime_file, "mime.types");

	// Set deamon to FALSE
	deamon = FALSE;
		
	filePointer = fopen(conf_file, "r");

	// Ensure that the configuration file is open
	if (filePointer == NULL)
	{
		fprintf(stderr, "Can't open configuration file!\n");
		exit(1);
	}

	// Get server root directory from configuration file
	if (fscanf(filePointer, "%s %s", currentLine, wwwroot) != 2)
	{
		fprintf(stderr, "Error in configuration file on line 1!\n");
		exit(1);
	}

	// Get default port from configuration file
	if (fscanf(filePointer, "%s %i", currentLine, &port) != 2)
	{
		fprintf(stderr, "Error in configuration file on line 2!\n");
		exit(1);
	}

	fclose(filePointer);
	free(currentLine);
}

int main(int argc, char* argv[])
{
	int parameterCount;
	char* fileExt = malloc(10);
	char* mime_type = malloc(800);

	init();

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

		// If flag -d is used, set deamon to TRUE;
		else if (strcmp(argv[parameterCount], "-d") == 0)
		{ 
			printf("Setting deamon = TRUE");
			deamon = TRUE;
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
			printf("\t\t-d\t\tEnables deamon mode.\n");
			printf("\t\t-l logfile\tWhich file to store the log to.\n");
			return -1;
		}
	}

	printf("Settings:\n");
	printf("Port:\t\t\t%i\n", port);
	printf("Server root:\t\t%s\n", wwwroot);
	printf("Configuration file:\t%s\n", conf_file);
	printf("Logfile:\t\t%s\n", log_file);
	printf("Deamon:\t\t\t%i\n", deamon);

	if ( deamon == TRUE )
	{
		daemonize();
	}

	start();

	return 0;
}
