
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>


//structure of SBCP Attribute
struct SBCP_attribute{
	uint16_t type;
	uint16_t length;
	char payload[512];
};

//structure of SBCP Message
struct SBCP_message{
        unsigned int vrsn: 9;
        unsigned int type: 7; 
	uint16_t length;
	struct SBCP_attribute attribute[2];
};


//to have a mapping of client fd and name
struct client_data {
	int fd;
	char client_name[16];
};

bool check_username (char name[], char *usernames[], int client_count) {
       
        int i;

	for ( i = 0; i < (client_count); i++ ) {
		
		if (strcmp(name,usernames[i]) == 0 ) {
			return false;
		}
	}

	return true;
}

void print_usernames(char *usernames[], int client_count) {

        printf("\n\n Clients : \n\n");
        int i;
 	for ( i = 0; i < client_count; i++ ) {

             printf(" %d -  %s \n", i+1, usernames[i]); 
        }

}


void NAK(int client_fd, char *r) {
	
	struct SBCP_message nak;
        nak.vrsn = 3;
        nak.type = 5;
        nak.attribute[0].type = 1;	
           
        char reason[200];
        bzero(&reason,sizeof(reason)); 
        strcpy(reason,r);
        
        nak.attribute[0].length = 4 + strlen(reason) + 4 - (strlen(reason) % 4);
        strcpy(nak.attribute[0].payload,reason);   
        nak.length = 4 + nak.attribute[0].length;
	write(client_fd,(void *) &nak,sizeof(nak));
  
}

void ACK(int client_fd, int client_count, char *clientnames[] ) {

	struct SBCP_message ack;
        ack.vrsn = 3;
        ack.type = 7;
        ack.attribute[0].type = 3;
        ack.attribute[1].type = 2;   
        sprintf(ack.attribute[0].payload, "%d",client_count);
        char names[500];
        bzero(&names,sizeof(names));
        int i; 
        for ( i = 0; i < client_count; i++ ) {
             
             strcat(names, clientnames[i]);
             strcat(names, " ");
        }
        ack.attribute[0].length = 4 + strlen(ack.attribute[0].payload) + 4 - (strlen(ack.attribute[0].payload) % 4);
		ack.attribute[1].length = 4 + strlen(names) + 4 - (strlen(names) % 4);
        strcpy(ack.attribute[1].payload, names);
        ack.length = 4 + ack.attribute[0].length + ack.attribute[1].length;
        write(client_fd,(void *) &ack,sizeof(ack));

}

void main (int argc, char* argv[]) {
	
	// Checking for 3 command-line arguments
	
    if(argc != 4) {
        fprintf(stderr, "Usage: ./server Server_IP Server_Port Max_Clients \n");
        exit(1);
    }           
	
	int max_clients = atoi(argv[3]);
	int client_count = 0;
	char *usernames[max_clients];
	int x,k;
	struct client_data client[max_clients];
	struct addrinfo hints;
        struct addrinfo *servinfo; 
        memset(&hints, 0, sizeof hints); // to make sure that the struct is empty
        hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6 address
        hints.ai_socktype = SOCK_STREAM; // for TCP 
	hints.ai_flags = AI_PASSIVE; 
	
	fd_set master;
        fd_set read_set;  
        FD_ZERO (&master);// Clear all entries in master
        FD_ZERO (&read_set);// Clear all entries in read_set
	
	if ((x = getaddrinfo(argv[1], argv[2] , &hints, &servinfo)) != 0) {
        fprintf(stderr, " getaddrinfo error : %s\n", gai_strerror(x));
        exit(1);
    }
    
	//socket inititialization
	
	int sockfd;
	if ( (sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0 ) {
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
	
	if( bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0){

		fprintf(stderr, " %s\n",strerror(errno));
                exit(1);

        }  else {	

		printf(" Socket binding done ! \n");

	}
    
	//listening for connections
	
	if (listen(sockfd , max_clients) < 0) {
        fprintf(stderr, " %s\n",strerror(errno));
        exit(1);
	} else {	
		printf(" Listening for clients ! \n");
	}
	
	
	FD_SET(sockfd,&master);
	int fdmax = sockfd;
	
	struct sockaddr_storage client_addr; //client's address
	
	while(1) {
		
		read_set = master;
		
		if(select(fdmax + 1, &read_set, NULL, NULL, NULL) < 0 ){
       		        fprintf(stderr, " %s\n",strerror(errno));
			exit(1);
        }
	
		int t,i;
		
		for ( i = 0; i <= fdmax; i++) { // looping through all the file descriptors
			if(FD_ISSET(i, &read_set)) {  // getting one file descriptor
				
				if ( i == sockfd ) {  // server socket, check for new connections
					socklen_t client_size = sizeof client_addr;
					int client_fd;
					
					if ((client_fd = accept(sockfd, (struct sockaddr *)&client_addr , &client_size )) < 0 ) {
                  			        fprintf(stderr, " %s\n",strerror(errno));
                                                exit(1);
						
					} else {	
						FD_SET(client_fd, &master);
                        
						t = fdmax;
						
                      			  if(client_fd > fdmax) { 
                       			     fdmax = client_fd;
                       			  }
						
						client_count++;
					//	printf("\n client_count: %d Max : %d\n\n\n",client_count, max_clients);
						
						if ( client_count > max_clients ) {  //reject client connection when count exceeded
							fdmax = t;
							printf(" Maximum client limit (%d) reached ! \n", max_clients);
							FD_CLR(client_fd, &master);
							NAK(client_fd,"Max client limit reached !!!");
                                                        close(client_fd);
							client_count--;
							print_usernames(usernames, client_count);							
						} else {
							
							struct SBCP_message join;
							char name[16];
							bzero(&join,sizeof(join));
							read(client_fd,(struct SBCP_message *) &join,sizeof(join));
							
							if ( join.type == 2 ) { //checking if it is Join message
								strcpy(name,join.attribute[0].payload);  //copy username to name
							
							}
							if (check_username(name, usernames, client_count-1)) {
							
								printf("\n\n Client Accepted : %s\n\n", name);
                                                                usernames[client_count - 1] = (char*) malloc(strlen(name) );
								strcpy(usernames[client_count - 1],name);
								printf(" Client count: %d\n", client_count);
								ACK(client_fd,client_count,usernames);
								print_usernames(usernames, client_count);
                                                                strcpy(client[client_count-1].client_name,name);
                                                                client[client_count-1].fd = client_fd;
								struct SBCP_message online;
								online.vrsn = 3;
								online.type = 8;
								online.attribute[0].type = 2;
                                online.attribute[0].length = 4 + strlen(name) + 4 - (strlen(name) % 4);
								strcpy(online.attribute[0].payload, name);
								online.length = 4 + online.attribute[0].length;
								//sending online message to all clients except current one and server
								
								for (k = 0; k <= fdmax; k++) {
								
									if (FD_ISSET(k,&master)) {
										
										if (k != client_fd && k != sockfd) {
										
											if ((write(k,(void *) &online,sizeof(online))) < 0 ){
												
						            	                            	fprintf(stderr, " %s\n",strerror(errno));
												
            	                						        }
										
										}
										
									}
								
								}
							
							
							} else {
							
								printf(" User with name : %s already exists, use another name...", name);
								NAK(client_fd,"Pls use another name...");
                                                                close(client_fd);
                                                                FD_CLR(client_fd, &master);
								client_count--;
								print_usernames(usernames, client_count);							
							}
												
						
						}
						
					
					}
					
				} else {  // one of the client sockets, handle the data from it 
					struct SBCP_message reads;
					int num_bytes;
					num_bytes = read(i,(struct SBCP_message *) &reads,sizeof(reads));
					
                                         if (num_bytes <= 0) {  					
						struct SBCP_message offline;
						offline.vrsn = 3;
						offline.type = 6;
						offline.attribute[0].type = 2;
					        	
						for (k = 0; k <= client_count; k++) {
							
							if (i == client[k].fd) {
								
								strcpy(offline.attribute[0].payload, client[k].client_name);
						        offline.attribute[0].length = 4 + strlen(client[k].client_name) + 4 - (strlen(client[k].client_name) % 4);	
							}
							
						}
						
						printf("\nUser : %s has left the chat \n\n",offline.attribute[0].payload);
						
						//sending offline message to all clients except current one and server
						offline.length = 4 + offline.attribute[0].length;		
						for (k = 0; k <= fdmax; k++) {
					
							if (FD_ISSET(k,&master)) {
										
								if (k != i && k != sockfd) {
										
									if ((write(k,(void *) &offline,sizeof(offline))) < 0 ){
												
                                 					      	fprintf(stderr, " %s\n",strerror(errno));
												
									}
										
								}
										
							}
								
						}
				                int index,index2;
                                                for (k = 0; k < client_count; k++) {
							if (i == client[k].fd) {
                                                        	index2 = k;
							}	
						}	

                                                for (k = 0; k < client_count; k++) {
							if (strcmp(client[index2].client_name,usernames[k]) == 0) {
								index = k;
                                                                break;	
							}
						}
	
						for (k = index; k < client_count-1; k++) {
							strcpy(usernames[k],usernames[k+1]);
							client[k] = client[k+1];
							
						}
						
						FD_CLR(i,&master);
						close(i);
						client_count--;
						print_usernames(usernames, client_count);	
						
					} else { //num_bytes > 0, so we forward it to all other clients
						if(reads.type == 4){	
							struct SBCP_message fwd;
							fwd.vrsn = 3;
							fwd.type = 3;
							fwd.attribute[0].type = 4;
							fwd.attribute[1].type = 2;
						
							strcpy(fwd.attribute[0].payload, reads.attribute[0].payload);
					                fwd.attribute[0].length = 4 + strlen(reads.attribute[0].payload) + 4 - (strlen(reads.attribute[0].payload) % 4); 	
						
							for (k = 0; k <= client_count; k++) {

								if (i == client[k].fd) {

									strcpy(fwd.attribute[1].payload, client[k].client_name);
							        fwd.attribute[1].length = 4 + strlen(client[k].client_name) + 4 - (strlen(client[k].client_name) % 4);

								}

							}
							fwd.length = 4 + fwd.attribute[0].length + fwd.attribute[1].length;
						
							//forwarding message to all clients except current one and server
						
							for (k = 0; k <= fdmax; k++) {
					
								if (FD_ISSET(k,&master)) {

									if (k != i && k != sockfd) {

										if ((write(k,(void *) &fwd,sizeof(fwd))) < 0 ){

      	                                                                   	fprintf(stderr, " %s\n",strerror(errno));

										}

									}

								}

							}
						} else {
							struct SBCP_message idle;
							idle.vrsn = 3;
							idle.type = 9;
							idle.attribute[0].type = 2;
						
							for (k = 0; k <= client_count; k++) {

								if (i == client[k].fd) {

									strcpy(idle.attribute[0].payload, client[k].client_name);

								}

							}
						
							//forwarding idle message to all clients except current one and server

							for (k = 0; k <= fdmax; k++) {
					
								if (FD_ISSET(k,&master)) {

									if (k != i && k != sockfd) {

										if ((write(k,(void *) &idle,sizeof(idle))) < 0 ){

      	                                                                   	fprintf(stderr, " %s\n",strerror(errno));

										}

									}

								}

							}
						}
						
					} //end of forwarding data from client
					
				} //end of handling data from client
				
			} //end of getting file descriptor
			
		} // end of for loop 
			
	} //end of while
	
	
	close(sockfd);
	
} //main
