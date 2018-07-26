#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "sockwrap.h"
#include "errlib.h"
#include "receive.h"

#define BUFF_LENGTH 1500

char *prog_name;

int receive_file_from_server(int socket, char* filename)
{
	char buf[BUFF_LENGTH];
	struct timeval tval;
	fd_set cset;
	int t = 5;
	int read_char;
	char date[80];
	struct tm *lt;	

	memset(buf, 0, BUFF_LENGTH);

	//receive and convert file size
	if(read(socket, buf, 4) != 4) return -1;
	uint32_t size = ntohl((*(uint32_t *)buf));
	printf("File size: %u byte", size);
	
	memset(buf, 0, BUFF_LENGTH);

	//receive, convert and print timestamp 
	if(read(socket, buf, 4) != 4) return -1;
	uint32_t timestamp = ntohl((*(uint32_t *)buf));
	time_t ts = timestamp;
	lt = localtime(&ts);
	strftime(date, sizeof(date), "%d.%m.%Y %H:%M:%S", lt);
	printf(" last edit: %s\n", date);

	//receive file
	int file, read_return = 0;
	if((file = open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR)) != -1){
		read_char = 0;
		while(read_char < size){

			FD_ZERO(&cset);
			FD_SET(socket, &cset);
			tval.tv_sec = t;
			tval.tv_usec = 0;

			if(Select(FD_SETSIZE, &cset, NULL, NULL, &tval)){
				memset(buf, 0, BUFF_LENGTH);
				if(BUFF_LENGTH > size - read_char){
					read_return = read(socket, buf, size - read_char);
					if(read_return != size - read_char)						
						return close_and_remove_file(file, filename);
				} else{
					read_return = read(socket, buf, BUFF_LENGTH);	
					if(read_return != BUFF_LENGTH)
						return close_and_remove_file(file, filename);	
				}

				read_char += read_return;

				if(write(file, buf, read_return) != read_return)
					return close_and_remove_file(file, filename);

			} else {
				printf("Timeout expired\n");
				return close_and_remove_file(file, filename);
			}
		}
	} else {
		printf("Error during file %s opening.\n", filename);
		return -1;
	}

	close(file);
	return 1;
}

int close_and_remove_file(int file, char* filename)
{
	close(file);
	remove(filename);
	return -1;
}

void estabilish_connection_client(int socket, char* address, char* port)
{
	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(port));
	server.sin_addr.s_addr = inet_addr(address);	

	Connect(socket, (const struct sockaddr*) &server, sizeof(server));
}