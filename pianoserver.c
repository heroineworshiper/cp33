/*
 * Piano audio processor
 *
 * Copyright (C) 2021-2024 Adam Williams <broadcast at earthling dot net>
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



// UDP or HTTP server

#define USE_UDP

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



#define UDP_RECV_PORT 1234
#define UDP_SEND_PORT 1235
#define BUFSIZE 1024



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
                                    handle_input(string, 0);
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
                    handle_update(string, 0);
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
#ifdef USE_UDP

    uint8_t packet[BUFSIZE];
    int read_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in read_addr;
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(struct sockaddr_in);
	read_addr.sin_family = AF_INET;
	read_addr.sin_port = htons((unsigned short)UDP_RECV_PORT);
	read_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(read_socket, 
		(struct sockaddr*)&read_addr, 
		sizeof(read_addr)) < 0)
	{
		perror("web_server: bind");
        exit(1);
	}

    while(1)
    {
        int bytes_read = recvfrom(read_socket,
            packet, 
            BUFSIZE, 
            0,
            (struct sockaddr *) &peer_addr, 
            &peer_addr_len);

//        printf("web_server %d: bytes_read=%d\n", __LINE__, bytes_read);
        uint8_t code[] = { 
            0xd0, 0x21, 0x39, 0xed, 0x63, 0x62, 0x47, 0x24, 
            0x9b, 0xed, 0x54, 0x0d, 0x24, 0xe2, 0x61, 0xf1 
        };
        int i;
        int got_it = 1;
        if(bytes_read < sizeof(code) + 1) got_it = 0;
        for(i = 0; i < sizeof(code) && got_it; i++)
        {
            if(packet[i] != code[i]) 
            {
                got_it = 0;
                break;
            }
        }
        if(got_it)
        {
            packet[bytes_read] = 0;
            int offset = sizeof(code);
            printf("web_server %d: got \"%s\"\n", 
                __LINE__, 
                packet + offset);
// contains user input
            if(packet[offset] == '1')
                offset += handle_input(packet + offset + 1, 1);
            offset++;

// extract hostname since peer_addr just contains the router
// The router might be masquerading
            char *peer = packet + offset;
            printf("web_server %d: peer=%s peer_addr=%s\n", 
                __LINE__, 
                peer,
                inet_ntoa(peer_addr.sin_addr));
            struct hostent *hostinfo = gethostbyname(peer);

            memcpy(packet, code, sizeof(code));
            handle_update(packet + sizeof(code), 1);
// create write socket for IPV4
	        int write_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	        struct sockaddr_in write_addr;
	        write_addr.sin_family = AF_INET;
	        write_addr.sin_port = htons((unsigned short)UDP_SEND_PORT);
            write_addr.sin_addr = *(struct in_addr *)hostinfo->h_addr;
//            write_addr.sin_addr = peer_addr.sin_addr;

// symbolic connection to the peer
	        if(connect(write_socket, 
		        (struct sockaddr*)&write_addr, 
		        sizeof(write_addr)) < 0)
	        {
		        perror("web_server: connect");
	        }

//printf("web_server %d: write_socket=%d\n", __LINE__, write_socket);
            write(write_socket, 
                packet, 
                sizeof(code) + strlen(packet + sizeof(code)));
            close(write_socket);
        }
    }


#else // USE_UDP

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

#endif // USE_UDP
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



