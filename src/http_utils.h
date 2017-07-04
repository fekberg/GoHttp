#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#define TRUE 1
#define FALSE 0
#define MAX_CONNECTIONS 3

char *wwwroot;
char *mime_file;

void init(int run_daemon, int *port, char **conf_file, char **log_file);
void accept_connection(int *current_socket, int *connecting_socket, socklen_t *addr_size, struct sockaddr_storage *connector); 
void startListener(int current_socket);

#endif // HTTP_UTILS_H
