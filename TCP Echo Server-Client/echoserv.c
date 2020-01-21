#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#define MAXLEN 100

int readline(int socket, char* buffer, int size ) {
	
	if (size <= 0 || buffer == NULL) {
		return -1;	
	}
	
	int x,length = 0;
	char s,*b = buffer;
	
	while(1) {
	
		x = (int)read(socket, &s, 1);
		
		if ( x == -1 ) {
			
			if (errno == EINTR) {
				continue;
			} else {
				return -1;
			}
			
		} else if ( x == 0 ) {
		        if (length == 0) 	
		        	return 0;
                        else   
				break; 
		} else {
			
			if ( length < (size - 1) ) {
				length++;
				*b = s;
				b++;
			} 

			if ( s == '\n' ) {
				break;
			}
		}
	
	}
	
	*b = '\0';
	return length;
	
}

int writen (int socket, char* buffer, int length ) {
	
	int num_to_write = length, num_written = 0;
	while (num_to_write != 0 ) {
		
		num_written = write(socket, buffer, num_to_write);
                if (num_written <= 0) { 
		if (num_written < 0 && errno == EINTR) {
			num_written = 0;
			continue;
		} else {
			return 0;
		}
		}
		num_to_write = num_to_write - num_written;
		buffer = buffer + num_written;
		
	}
	return num_written;
}


void main(int argc, char *argv[]) {
 

    // Checking for 1 command-line arguments
        if(argc != 2) {
           fprintf(stderr, "Usage: echos Port\n");
           exit(1);
        }           
 
    //socket inititialization
	
	int sockfd;
	if ( ( sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) {
		fprintf(stderr, "%s\n",strerror(errno));
                exit(1);
	} else {	
		printf("\n Socket created ! \n");
	}
	
	int r = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &r, sizeof(int)) < 0) {
           fprintf(stderr, " %s\n",strerror(errno));
           exit(1);
        }
	
	//binding socket to port & IP
	
	struct sockaddr_in serv;
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = inet_addr("127.0.0.1");
        serv.sin_port = htons(atoi(argv[1]));
    
	
	if ((bind(sockfd , (struct sockaddr*) &serv , sizeof(serv) )) < 0 ) {
                fprintf(stderr, " %s\n",strerror(errno));
                exit(1);
	} else {	
		printf(" Socket binding done ! \n");
	}
 
	//listening for connections
	
	if (listen(sockfd , 10) < 0) {
                fprintf(stderr, " %s\n",strerror(errno));
                exit(1);
	} else {	
		printf(" Listening to client ! \n");
	}
 
	//accept the client connection
	
        int client_sock;

        //loop to accept multiple clients

	while(1) {
		
		struct sockaddr_in client;
		
		socklen_t client_size = sizeof(client);
		
		if ((client_sock = accept(sockfd, (struct sockaddr*) &client , &client_size )) < 0 ) {
                        fprintf(stderr, " %s\n",strerror(errno));
                        exit(1);
		} else {	
			printf(" Client Accepted ! \n");
		}
	
		// forking
		int length,numbytes,n;
                char string[MAXLEN];
                pid_t pid;
                if ( !(pid = fork())) {
                      
                     //loop to accept multiple inputs from each connection
 
                     close(sockfd); 

                     while(1) {

                        length = 0;
                        memset(&string,0,MAXLEN);
			length = readline(client_sock, string, MAXLEN);
                       
                        if (length == -1) {

                           exit(1);
                             
                        } else if (length == 0) {
 
                           close(client_sock);
                           exit(1);

                        } else {

                           printf("\n\n Length of String recieved : %d ",length);
         		   printf("\n String recieved           : %s ",string);

                        }
 
			n = writen(client_sock, string, length);
                        printf("\n Length of String written  : %d \n\n",n);

	   	} //while

            } //if
            
            printf(" Child Process created with PID : %d \n", pid);

	} //while

     close(client_sock);
 
} //main

