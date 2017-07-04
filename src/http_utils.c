#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "http_utils.h"

#define BUFFER_SIZE 512

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

/**
  Start listening for connections and accept no more than MAX_CONNECTIONS in the Quee
 **/
void startListener(int current_socket)
{
    if ( listen(current_socket, MAX_CONNECTIONS) < 0 )
    {
        perror("Listen on port");
        exit(-1);
    }
}

void init(int run_daemon, int *port, char **conf_file, char **log_file)
{
    FILE *filePointer = NULL;
    char* currentLine = malloc(100);
    wwwroot = malloc(100);
    *conf_file = malloc(100);
    *log_file = malloc(100);
    mime_file = malloc(600);

    // Setting default values
    *conf_file = "config/httpd.conf";
    *log_file = ".log";
    strcpy(mime_file, "config/mime.types");

    // Set run_daemon to FALSE
    run_daemon = FALSE;

    filePointer = fopen(*conf_file, "r");

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
    if (fscanf(filePointer, "%s %i", currentLine, port) != 2)
    {
        fprintf(stderr, "Error in configuration file on line 2!\n");
        exit(1);
    }

    fclose(filePointer);
    free(currentLine);
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

int sendString(char *message, int socket)
{
    int length, bytes_sent;
    length = strlen(message);

    bytes_sent = send(socket, message, length, 0);

    return bytes_sent;
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

int Content_Lenght(FILE *fp)
{
    int filesize = 0;

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);

    return filesize;
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

int sendBinary(int *byte, int length, int connecting_socket)
{
    int bytes_sent;

    bytes_sent = send(connecting_socket, byte, length, 0);

    return bytes_sent;


    return 0;
}

void sendFile(FILE *fp, int file_size, int connecting_socket)
{
    int current_char = 0;

    do{
        current_char = fgetc(fp);
        sendBinary(&current_char, sizeof(char), connecting_socket);
    }
    while(current_char != EOF);
}

int handleHttpGET(char *input, int connecting_socket)
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

            sendFile(fp, contentLength, connecting_socket);

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

static int receive(int socket)
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
        handleHttpGET(buffer, socket);
    }
    else if ( request == 2 )		// HEAD
    {
        // SendHeader();
    }
    else if ( request == 0 )		// POST
    {
        sendString("501 Not Implemented\n", socket);
    }
    else							// GARBAGE
    {
        sendString("400 Bad Request\n", socket);
    }

    return 1;
}

/**
  Handles the current connector
 **/
static void handle(int socket)
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

void accept_connection(int *current_socket, int *connecting_socket, socklen_t *addr_size, struct sockaddr_storage *connector)
{
    *addr_size = sizeof(*connector);

    *connecting_socket = accept(*current_socket, (struct sockaddr *)connector, addr_size);


    if ( *connecting_socket < 0 )
    {
        perror("Accepting sockets");
        exit(-1);
    }

    handle(*connecting_socket);

    close(*connecting_socket);
}
