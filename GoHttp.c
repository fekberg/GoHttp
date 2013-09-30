/** This works!
** SendHTML("HTTP/1.1 200 OK", "text/html", "<html><head><title>Hej hopp!</title></head><body><h1>It's alive!</h1></body></html>");
**/

/** Includes **/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <wait.h>


/** Socket depending variables **/
#define BUFFER_SIZE 512
#define MAX_FILE_SIZE 5*1024
#define MAX_CONNECTIONS 3
#define TRUE 1
#define FALSE 0
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int port;                               // Listen port
int deamon = FALSE;                     // Run as deamon?
char *wwwroot;							// Full address to server root directory
char *conf_file;                        // Full address to configuration file
char *log_file;                         // Full address to log file
char *mime_file;

FILE *pfile = NULL;                     // Pointer to file

struct sockaddr_in address;             // scokaddr for listening address
struct sockaddr_storage connector;      // sockaddr for connecting address
int current_socket;                     // The listening socket descriptor
int connecting_socket;                  // The connecting socket descriptior
socklen_t addr_size;                    // Address lenght

struct stat buf;                        // Struct to check user rights

void Log(char *logtext)
{
	printf(logtext);
}

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

void AddCLF(char *remotehost, char *rfc931, char *authuser, char *request, char *status, char *bytes)
{
	char *message = malloc((
		strlen(remotehost) +
		strlen(rfc931) +
		strlen(authuser) +
		strlen(request) +
		strlen(status) +
		strlen(bytes) +
		26 + /* date */
		5 + /* spaces */
		sizeof(char)) * 2);

	char *space = " ";

	time_t rawtime;

	time ( &rawtime );

	strcpy(message, remotehost);

	strcat(message, space);

	strcat(message, rfc931);

	strcat(message, space);

	strcat(message, authuser);

	strcat(message, space);

	strcat(message, (char*)ctime(&rawtime));

	strcat(message, space);

	strcat(message, request);

	strcat(message, space);

	strcat(message, status);

	Log(message);
}

Send(char *message, int socket)
{
	int length, bytes_sent;
	length = strlen(message);

	bytes_sent = send(socket, message, length, 0);

	return bytes_sent;
}

int SendBinary(int *byte, int length)
{
	int bytes_sent;

	bytes_sent = send(connecting_socket, byte, length, 0);

	return bytes_sent;


	return 0;
}
void SendHeader(char *Status_code, char *Content_Type, int TotalSize, int socket)
{
	char *head = "\r\nHTTP/1.1 ";
	char *content_head = "\r\nContent-Type: ";
	char *server_head = "\r\nServer: PT06";
	char *length_head = "\r\nContent-Length: ";
	char *date_head = "\r\nDate: ";
	char *newline = "\r\n";
	char Content_Length[100];

	time_t rawtime;

	time ( &rawtime );

	// int content_length = strlen(HTML);
	sprintf(Content_Length, "%i", TotalSize);

	char *message = malloc((
		strlen(head) +
		strlen(content_head) +
		strlen(server_head) +
		strlen(length_head) +
		strlen(date_head) +
		strlen(newline) +
		strlen(Status_code) +
		strlen(Content_Type) +
		strlen(Content_Length) +
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
		strcat(message, Content_Length);
		strcat(message, date_head);
		strcat(message, (char*)ctime(&rawtime));
		strcat(message, newline);

		Send(message, socket);

		free(message);
	}    
}

void SendHTML(char *Status_Code, char *Content_Type, char *HTML, int TotalSize, int socket)
{
	SendHeader(Status_Code, Content_Type, TotalSize, socket);
	Send(HTML, socket);
}

void SendFile(FILE *fp, int file_size)
{
	char *file_buffer = malloc(MAX_FILE_SIZE);
	int current_char = 0;

	do{
		current_char = fgetc(fp);
		SendBinary(&current_char, sizeof(char));
	}
	while(current_char != EOF);
}

int scan(char *input, char *output, int start)
{
	if ( start >= strlen(input) )
		return -1;

	int appending_char_count = 0;
	int i = start;

	for ( ; i < strlen(input); i ++ )
	{
		if ( *(input + i) != '\t' && *(input + i) != ' ' && *(input + i) != '\n' && *(input + i) != '\r')
		{
			*(output + appending_char_count) = *(input + i ) ;

			appending_char_count += 1;
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




int CheckMime(char *extension, char *mime_type)
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
			startline = scan(line, current_word, 0);
			while ( 1 )
			{
				startline = scan(line, word_holder, startline);
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

int GetHTTPVersion(char *input, char *output)
{
	char *filename = malloc(100);
	int start = scan(input, filename, 4);
	if ( start > 0 )
	{
		if ( scan(input, output, start) )
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

int GetExtension(char *input, char *output)
{
	int in_position = 0;
	int appended_position = 0;
	int i = 0;

	for ( ; i < strlen(input); i ++ )
	{		
		if ( in_position == 1 )
		{
			output[appended_position] = input[i];
			appended_position +=1;
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

int ProcessGET(char *input)
{
	// IF NOT EXISTS
	// RETURN -1
	// IF EXISTS
	// RETURN 1

	char *filename = (char*)malloc(200 * sizeof(char));
	char *absolute_path = (char*)malloc(1000 * sizeof(char));
	char *file_extension = (char*)malloc(10 * sizeof(char));
	char *file_mime = (char*)malloc(200 * sizeof(char));
	char httpVersion[20];

	int Content_Length = 0;
	int Mime_Support = 0;
	int FileNameLenght = 0;


	memset(absolute_path, '\0', 1000);
	memset(filename, '\0', 200);
	memset(file_extension, '\0', 10);
	memset(file_mime, '\0', 200);
	memset(httpVersion, '\0', 20);

	FileNameLenght = scan(input, filename, 5);


	if ( FileNameLenght > 0 )
	{

		if ( GetHTTPVersion(input, httpVersion) != -1 )
		{
			FILE *fp;

			if ( GetExtension(filename, file_extension) == -1 )
			{
				Log("File extension not existing");

				Send("400 Bad Request\n", connecting_socket);

				free(filename);
				free(file_mime);
				free(absolute_path);
				free(file_extension);

				return -1;
			}

			Mime_Support =  CheckMime(file_extension, file_mime);


			if ( Mime_Support != 1)
			{
				Log("Mime not supported");

				Send("400 Bad Request\n", connecting_socket);

				free(filename);
				free(file_mime);
				free(absolute_path);
				free(file_extension);

				return -1;
			}

			// Open the requesting file as binary //

			strcpy(absolute_path, wwwroot);

			strcat(absolute_path, filename);			

			fp = fopen(absolute_path, "rb");

			if ( fp == NULL )
			{
				Log("Unable to open file");

				Send("404 Not Found\n", connecting_socket);

				free(filename);
				free(file_mime);
				free(file_extension);
				free(absolute_path);

				return -1;
			}


			// Calculate Content Length //
			Content_Length = Content_Lenght(fp);
			if (Content_Length  < 0 )
			{
				Log("File size is zero");

				free(filename);
				free(file_mime);
				free(file_extension);
				free(absolute_path);

				fclose(fp);

				return -1;
			}

			// Send File Content //
			SendHeader("200 OK", file_mime,Content_Length, connecting_socket);

			SendFile(fp, Content_Length);

			free(filename);
			free(file_mime);
			free(file_extension);
			free(absolute_path);

			fclose(fp);

			return 1;
		}
		else
		{
			Send("501 Not Implemented\n", connecting_socket);
		}
	}

	return -1;
}

int ValidateRequest(char *input)
{
	// IF NOT VALID REQUEST 
	// RETURN -1
	// IF VALID REQUEST
	// RETURN 1 IF GET
	// RETURN 2 IF HEAD
	// RETURN 0 IF NOT YET IMPLEMENTED

	int Valid = -1;

	if ( strlen ( input ) > 0 )
		Valid = 1;
	else
		Valid = -1;
	// CHECK IF GET

	char *requestType = malloc(5);

	scan(input, requestType, 0);

	if ( Valid == TRUE && strcmp("GET", requestType) == 0)
		Valid = 1;

	// CHECK IF HEAD
	else if (Valid == TRUE && strcmp("HEAD", requestType) == 0)
		Valid = 2;

	else if (strlen(input) > 4 && strcmp("POST", requestType) == 0 )
	{
		Valid = 0;
	}
	else
		Valid = -1;

	return Valid;
}

int Receive(int socket)
{
	int msgLen = 0;
	char buffer[BUFFER_SIZE];

	memset (buffer,'\0', BUFFER_SIZE);

	if ((msgLen = recv(socket, buffer, BUFFER_SIZE, 0)) == -1)
	{
		Log("Error handling incoming request");
		return -1;
	}

	int Request = ValidateRequest(buffer);

	if ( Request == 1 )				// GET
	{
		ProcessGET(buffer);
	}
	else if ( Request == 2 )		// HEAD
	{
		// SendHeader();
	}
	else if ( Request == 0 )		// POST
	{
		Send("501 Not Implemented\n", connecting_socket);
	}
	else							// GARBAGE
	{
		Send("400 Bad Request\n", connecting_socket);
	}

	return 1;
}

/**
Create a socket and assign current_socket to the descriptor
**/
void CreateSocket()
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
void BindSocket()
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
void ListenOnSocket()
{
	if ( listen(current_socket, MAX_CONNECTIONS) < 0 )
	{
		perror("Listen on port");
		exit(-1);
	}
}

/**
Accept a connection that connects to the socket
**/


/**
Handles the current connector
**/
void HandleCurrentConnection(int socket)
{
	// --- Workflow --- //
	// 1. Receive ( recv() ) the GET / HEAD
	// 2. Process the request and see if the file exists
	// 3. Read the file content
	// 4. Send out with correct mine and http 1.1

	if (Receive((int)socket) < 0)
	{
		perror("Receive");
		exit(-1);
	}
}

/**
Dispose the current connector
**/
void DisposeCurrentConnection()
{
	close(connecting_socket);
}
void ProcessConnections()
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

	HandleCurrentConnection(connecting_socket);


	DisposeCurrentConnection();
	/*
	if ( child_process == 0 )
	{
	exit(0);
	}

	while (-1 != waitpid (-1, NULL, WNOHANG));

	*/		// while (-1 != waitpid (-1, NULL, WNOHANG));

}
/**
Starting the actual webserver
**/
void BeginListen()
{
	CreateSocket();

	BindSocket();

	ListenOnSocket();

	while ( 1 )
	{
		ProcessConnections();
	}
}

void Prepare()
{
	Log("Loading Configuration...\n");
}

int main(int argc, char* argv[])
{


	// Init variables
	int i;
	char* curLine = malloc(100);
	char* fileExt = malloc(10);

	// * temp * //
	char* mime_type = malloc(800);

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

	pfile = fopen(conf_file, "r");

	if (pfile == NULL)
	{
		fprintf(stderr, "Can't open configuration file!\n");
		exit(1);
	}

	// Get server root directory from configuration file
	if (fscanf(pfile, "%s %s", curLine, wwwroot) != 2)
	{
		fprintf(stderr, "Error in configuration file on line 1!\n");
		exit(1);
	}

	// Get default port from configuration file
	if (fscanf(pfile, "%s %i", curLine, &port) != 2)
	{
		fprintf(stderr, "Error in configuration file on line 2!\n");
		exit(1);
	}
	fclose(pfile);

	free(curLine);

	// Loop through all arguments
	for (i = 1; i < argc; i++)
	{
		// If flag -p is used, set port
		if (strcmp(argv[i], "-p") == 0)
		{
			i++;
			printf("Setting port to %i\n", atoi(argv[i]));
			port = atoi(argv[i]);
		}

		// If flag -d is used, set deamon to TRUE;
		else if (strcmp(argv[i], "-d") == 0)
		{
			printf("Setting deamon = TRUE");
			deamon = TRUE;
		}

		else if (strcmp(argv[i], "-l") == 0)
		{
			i++;
			printf("Setting logfile = %s\n", argv[i]);
			log_file = (char*)argv[i];
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


	if ( CheckMime("htm", mime_type) > 0 )
		printf("Mime type: %s\r\n", mime_type);

	if ( CheckMime("ico", mime_type) > 0 )

		printf("Mime type: %s\r\n", mime_type);
	if ( CheckMime("jpg", mime_type) > 0 )
		printf("Mime type: %s\r\n", mime_type);


	Prepare();

	BeginListen();

	return 0;

}