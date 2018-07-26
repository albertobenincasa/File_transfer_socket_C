#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "sockwrap.h"
#include "errlib.h"
#include "send.h"

#define BUFF_LENGTH 1500
#define OK_MSG "+OK\r\n"
#define ERROR_MSG "-ERR\r\n"
#define QUIT_MSG "QUIT"
#define GET_MSG "GET"

char *prog_name;

int send_file_to_client(int socket1)
{
	char filename[BUFF_LENGTH], buffer[BUFF_LENGTH];

	while(1){
		memset(buffer, 0, BUFF_LENGTH);
		int read_char = 0, i;
		char c = ' ';
		char msg[5];

		while(c != '\n'){
			if(read(socket1, &c, 1) != 1) return -1;
			buffer[read_char++] = c;
		}

		sscanf(buffer, "%s %s", msg, filename);

		if(strcmp(msg, GET_MSG) == 0){
			int file;
			struct stat st;
			int ret = stat(filename, &st);
			printf("-- (%d) File requested: %s ", getpid(), filename);
			
			if(ret == 0){
				if((file=open(filename, O_RDONLY)) != -1){

					//checking stat's file
					int size = st.st_size;
					int last_modify = st.st_mtime;
					uint32_t dim = htonl(size);
					uint32_t last = htonl(last_modify);
					printf("size: %u byte\n", size);

					//sending ok msg, dimension and timestamp
					if(write(socket1, OK_MSG, strlen(OK_MSG)) != strlen(OK_MSG)) return -1;
					if(write(socket1, &dim, 4) != 4) return -1;
					if(write(socket1, &last, 4) != 4) return -1;

					int sent_char = 0;
					int read_return;
					while(sent_char < size){
						memset(buffer, 0, BUFF_LENGTH);
						if(BUFF_LENGTH > size - sent_char){
							read_return =	read(file, buffer, size - sent_char);
							if(read_return != size - sent_char) return -1;
						} else{
							read_return = read(file, buffer, BUFF_LENGTH);
							if(read_return != BUFF_LENGTH) return -1;
						}
					
						sent_char += read_return;
						if(write(socket1, buffer, read_return) != read_return) return -1;
					}

					printf("\n--- (%d) File succesfully sent, %u byte sent.\n\n", getpid(), sent_char);
					close(file);
				} else {
					printf("\n-- (%d) The file %s does not exist in working directory\n\n", getpid(), filename);
					//Write(socket1, ERROR_MSG, strlen(ERROR_MSG));
					if(write(socket1, ERROR_MSG, strlen(ERROR_MSG)) != strlen(ERROR_MSG)) return -1;
					return -1;
				}
			} else {
				printf("\n-- (%d) The file %s does not exist in working directory\n\n", getpid(), filename);
				//Write(socket1, ERROR_MSG, strlen(ERROR_MSG));
				if(write(socket1, ERROR_MSG, strlen(ERROR_MSG)) != strlen(ERROR_MSG)) return -1;
				return -1;
			} 
		} else if(strcmp(msg, QUIT_MSG) == 0){
			printf("\n-- (%d) QUIT_MSG arrived, closing connection..\n", getpid());
			break;
		} else {
			printf("-- (%d) No GET or QUIT messages arrived\n", getpid());
			if(write(socket1, ERROR_MSG, strlen(ERROR_MSG)) != strlen(ERROR_MSG)) return -1;
			return -1;
		}
	}

	return 1;
}

void estabilish_connection_server(int socket, char* port)
{
	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(port));
	server.sin_addr.s_addr = htonl(INADDR_ANY);	

	Bind(socket, (const struct sockaddr*) &server, sizeof(server));
	Listen(socket, 2);
}