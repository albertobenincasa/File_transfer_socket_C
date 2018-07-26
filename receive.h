#ifndef _RECEIVE_H

#define _RECEIVE_H

int receive_file_from_server(int socket, char* filename);
int close_and_remove_file(int file, char* filename);
void estabilish_connection_client(int socket, char* address, char* port);

#endif
