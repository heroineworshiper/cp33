/*
 * Piano audio processor
 *
 * Copyright (C) 2021 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */



// web server


#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include "piano.h"




#define PORT 80
#define TOTAL_CONNECTIONS 32
#define SOCKET_BUFSIZE 1024
#define MAX_FILESIZE 0x100000
#define SERVER_NAME "Piano"

typedef struct 
{
	int is_busy;
	int fd;
	sem_t lock;
} webserver_connection_t;
webserver_connection_t* connections[TOTAL_CONNECTIONS];

pthread_mutex_t www_mutex;




void send_header(webserver_connection_t *connection, char *content_type)
{
	char header[TEXTLEN];
	sprintf(header, "HTTP/1.0 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Server: %s\r\n\r\n",
			content_type,
            SERVER_NAME);
	write(connection->fd, header, strlen(header));
}

void send_error(webserver_connection_t *connection, const char *text)
{
	char header[TEXTLEN];
	sprintf(header, "HTTP/1.0 404 OK\r\n"
            "Content-Type: text/html\r\n"
            "Server: %s\r\n\r\n",
            SERVER_NAME);
	write(connection->fd, header, strlen(header));
    write(connection->fd, text, strlen(text));
}

void send_string(webserver_connection_t *connection, char *text)
{
	write(connection->fd, text, strlen(text));
}

void send_file(webserver_connection_t *connection, char *getpath, char *mime)
{
	char string[TEXTLEN];

// the home page
	if(!strcmp(getpath, "/"))
	{
		sprintf(string, "index.html");
	}
	else
// strip the .
	{
		sprintf(string, "%s", getpath + 1);
	}

	FILE *fd = fopen(string, "r");
	if(fd)
	{
		fseek(fd, 0, SEEK_END);
		int size = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		
		if(size > MAX_FILESIZE)
		{
			size = MAX_FILESIZE;
		}

// need to pad the buffer or fclose locks up
		unsigned char *buffer2 = (unsigned char*)malloc(size + 16);
		int result = fread(buffer2, size, 1, fd);
		fclose(fd);

printf("send_file %d %s %s\n", __LINE__, string, mime);
		send_header(connection, mime);
		write(connection->fd, buffer2, size);
		free(buffer2);
	}
    else
    {
        printf("send_file %d: couldn't open %s\n", __LINE__, string);
        char string2[TEXTLEN];
        sprintf(string2, "Couldn't open %s\n", string);
        send_error(connection, string2);
    }

}

void web_server_connection(void *ptr)
{
	webserver_connection_t *connection = (webserver_connection_t*)ptr;
	unsigned char buffer[SOCKET_BUFSIZE];
	int i;
	
	while(1)
	{
		sem_wait(&connection->lock);
		
		int done = 0;
		while(!done)
		{
			buffer[0] = 0;
			int bytes_read = read(connection->fd, buffer, SOCKET_BUFSIZE);
			if(bytes_read <= 0)
			{
				break;
			}
			
//printf("web_server_connection %d:\n", __LINE__);
//printf("%s", buffer);

			char *ptr = buffer;
			char getpath[TEXTLEN];
			if(ptr[0] == 'G' &&
				ptr[1] == 'E' &&
				ptr[2] == 'T' &&
				ptr[3] == ' ')
			{
				ptr += 4;
				while(*ptr != 0 && *ptr == ' ')
				{
					ptr++;
				}
				
				if(*ptr != 0)
				{
					char *ptr2 = getpath;
					while(*ptr != 0 && *ptr != ' ')
					{
						*ptr2++ = *ptr++;
					}
					*ptr2 = 0;
				}
				
//printf("web_server_connection %d: requested %s\n", __LINE__, getpath);

// parse commands & get status
				if(!strncasecmp(getpath, "/update", 6))
				{
//printf("web_server_connection %d: requested %s\n", __LINE__, getpath);
					char string[TEXTLEN];
					
					int need_mixer = 0;
					int need_monitor = 0;
					char *ptr = getpath + 6;
					while(*ptr != 0)
					{
						if(*ptr == '?')
						{
							ptr++;
							
							if(!strncmp(ptr, "input", 5))
							{
								ptr += 5;
                                if(*ptr == '=')
                                {
                                    ptr++;
                                    char *ptr2 = string;
                                    while(*ptr != 0 && *ptr != '?')
									{
                                        *ptr2++ = *ptr++;
									}
                                    *ptr2 = 0;
                                    handle_input(string);
                                }
							}
						}
						else
						{
							ptr++;
						}
					}
					

// printf("web_server_connection %d: total_remane=%lld total_written=%lld\n", 
// __LINE__, 
// total_remane,
// total_written);

// send current state of the board
// must write seconds instead of samples because javascript can't handle 64 bits
					pthread_mutex_lock(&www_mutex);
                    handle_update(string);
					pthread_mutex_unlock(&www_mutex);
//printf("web_server_connection %d: update=%s\n", __LINE__, string);
					
					
					
					
					send_header(connection, "text/html");
					write(connection->fd, (unsigned char*)string, strlen(string));
					
					done = 1;
				}
				else
				if(!strcasecmp(getpath, "/favicon.ico"))
				{
					send_file(connection, getpath, "image/x-icon");
					done = 1;
				}
				else
				if(!strcasecmp(getpath, "/") ||
					!strcasecmp(getpath, "/index.html"))
				{
					send_file(connection, getpath, "text/html");
					done = 1;
				}
				else
				if(!strcasecmp(getpath, "/record.png") ||
					!strcasecmp(getpath, "/stop.png") ||
					!strcasecmp(getpath, "/single.gif"))
				{
					send_file(connection, getpath, "image/png");
					done = 1;
				}
				else
				{
					send_header(connection, "text/html");
					send_string(connection, "SHIT\n");
					done = 1;
				}
			}
		}
		
		if(!done)
		{
//			printf("web_server_connection %d: client closed\n", __LINE__);
		}
		else
		{
//			printf("web_server_connection %d: server closed\n", __LINE__);
		}
		close(connection->fd);
		connection->is_busy = 0;
	}
}

webserver_connection_t* new_connection()
{
	webserver_connection_t *result = calloc(1, sizeof(webserver_connection_t));
	sem_init(&result->lock, 0, 0);
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_t tid;
	pthread_create(&tid, 
		&attr, 
		(void*)web_server_connection, 
		result);
	return result;
}

void start_connection(webserver_connection_t *connection, int fd)
{
	connection->is_busy = 1;
	connection->fd = fd;
	sem_post(&connection->lock);
}


void web_server(void *ptr)
{
	int i;
	for(i = 0; i < TOTAL_CONNECTIONS; i++)
	{
		connections[i] = new_connection();
	}
	
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	
	int reuseon = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseon, sizeof(reuseon));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    int result = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
	if(result)
	{
		printf("web_server %d: bind failed\n", __LINE__);
		return;
	}
	
	while(1)
	{
		listen(fd, 256);
		struct sockaddr_in clientname;
		socklen_t size = sizeof(clientname);
		int connection_fd = accept(fd,
                			(struct sockaddr*)&clientname, 
							&size);

//printf("web_server %d: accept\n", __LINE__);

		int got_it = 0;
		for(i = 0; i < TOTAL_CONNECTIONS; i++)
		{
			if(!connections[i]->is_busy)
			{
				start_connection(connections[i], connection_fd);
				got_it = 1;
				break;
			}
		}
		
		if(!got_it)
		{
			printf("web_server %d: out of connections\n", __LINE__);
		}
	}
}




void init_server()
{
	pthread_mutexattr_t attr2;
	pthread_mutexattr_init(&attr2);
	pthread_mutex_init(&www_mutex, &attr2);
    
    pthread_t tid;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
   	pthread_create(&tid, 
		&attr, 
		(void*)web_server, 
		0);
 

}



