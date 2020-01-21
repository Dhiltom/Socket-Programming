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
#include <time.h>

struct cache_entry {
  char * hostname;
  char * path;
  char * content;
  char * last_access;
  char * last_modified;
  char * expires;
  int valid;
};

int time_cmp(char *date1, char *date2)
{
  struct tm storage1={0,0,0,0,0,0,0,0,0};
  struct tm storage2={0,0,0,0,0,0,0,0,0};

  char *p1;
  char *p2;
  time_t time1=0;
  time_t time2=0;

  p1=(char *)strptime(date1,"%a, %d %b %Y %T %Z",&storage1);
  p2=(char *)strptime(date2,"%a, %d %b %Y %T %Z",&storage2);
  if((p1==NULL) || (p2==NULL))
  {
    return -1;
  } else {
    time1=mktime(&storage1);
    time2=mktime(&storage2);
    if(time1 == time2){
      return 0;
    } else if (time1 > time2){
      return 1;
    } else {
      return 2;
    }
  }
}

time_t time_s(char *date1)
{
  struct tm storage1={0,0,0,0,0,0,0,0,0};

  char *p1;
  time_t time1=0;

  p1=(char *)strptime(date1,"%a, %d %b %Y %T %Z",&storage1);
  if((p1==NULL))
  {
    return -1;
  } else {
    time1=mktime(&storage1);
    return(time1);
  }
}

int main (int argc, char* argv[]) {

	if(argc != 3) {
    fprintf(stderr, "Usage: ./server <Server_IP> <Server_Port>  \n");
    exit(1);
  }         

  int x,k;
	struct addrinfo hints;
  struct addrinfo *servinfo; 
	struct sockaddr_storage client_addr; //client's address
  struct cache_entry doc_entries[11];
  //int j;
  int num_entries;
  num_entries = 0;
  int cache_full = 0;
  int cache_idx = 0;

  memset(&hints, 0, sizeof hints); // to make sure that the struct is empty
  hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6 address
  hints.ai_socktype = SOCK_STREAM; // for TCP 
	hints.ai_flags = AI_PASSIVE;

  fd_set master;
  fd_set read_set;  
  FD_ZERO (&master); // Clear all entries in master
  FD_ZERO (&read_set); // Clear all entries in read_set
	
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
		printf("\nSocket created ! \n");
	}
	
	int r = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &r, sizeof(int)) < 0) {
    fprintf(stderr, " %s\n",strerror(errno));
    exit(1);
  }

  //binding socket to port & IP

  if( bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		fprintf(stderr, " %s\n",strerror(errno));
    exit(1);
  } else {	
		printf("Socket binding done ! \n");
	}
    
  if (listen(sockfd , 10) < 0) {
    fprintf(stderr, " %s\n",strerror(errno));
    exit(1);
	} else {	
		printf("Listening for clients ! \n");
	}
	
	FD_SET(sockfd,&master);
	int fdmax = sockfd;
  int fd;

	while(1) {
		
		read_set = master;
	  printf("Num of entries in cache : %d\n",num_entries);

    if(num_entries <= 10){
      if (num_entries>0) {
        int z = 0;
        printf("\n================Cache entries=================\n");
        for (z = 0; z<num_entries; z++) {
          printf("Entry number: %d\n", z);
         	printf("Host: %s\n", doc_entries[z].hostname);
         	printf("Path: %s\n", doc_entries[z].path);
         	printf("Expires: %s\n", doc_entries[z].expires);
         	printf("Last-Modified: %s\n", doc_entries[z].last_modified);
         	printf("Last-Access: %s\n", doc_entries[z].last_access);
         	printf("Local filename: %s\n\n", doc_entries[z].content);  
        }   
      }
    }
	
		if(select(fdmax + 1, &read_set, NULL, NULL, NULL) < 0 ){
      fprintf(stderr, "%s\n",strerror(errno));
			exit(1);
    }

    for(fd = 0; fd <= fdmax; fd++) {
      if(FD_ISSET(fd, &master)){
        if(fd == sockfd) {
          socklen_t client_size = sizeof(client_addr);
          int client_fd;

          if ((client_fd = accept(sockfd, (struct sockaddr *) &client_addr, &client_size)) < 0){
            fprintf(stderr, "accept - error: %s\n", strerror(errno));
            exit(1);
          } else {
            printf("Client connected\n");
            FD_SET(client_fd, &master);
            if(client_fd > fdmax)
              fdmax = client_fd;
          }
        } else {   //client fds

          char request[2057];
          bzero(&request, sizeof(request));

          int bytes_read;

          bytes_read = recv(fd, &request, 2056, 0);
          if (bytes_read < 0){
            fprintf(stderr, "recv - error: %s\n", strerror(errno));
            exit(1);
          }
          printf("Bytes read = %d\n", bytes_read);
          printf("Request received:\n%s\n", request);

          // Not - found send request to the SERVER
          // Get the host address
          char * host;
          char * pch; // Pattern pointer
          char * doc_name;
          char * protocol;
          long int fn_bytes;
          char * p1;
          int to_cache = 0;

          pch = strstr(request, "GET ");
          protocol = strstr((pch+4), " HTTP");
          fn_bytes = protocol - (pch+4);

          doc_name = (char *)calloc((fn_bytes + 1), sizeof(char));
          strncpy(doc_name, (pch+4), fn_bytes);
          printf("DOC NAME: %s\n", doc_name);
          pch = strstr(request, "Host: ");
          p1 = strstr(request, "\r\n\r\n");
          host = (char *)malloc(p1-(pch+6)+1);
          bzero(host, p1-(pch+6)+1); 
          strncpy(host, (pch+6), p1-(pch+6));
          printf("host - %s, length: %ld\n", host, strlen(host));
 
          // Lookup here
          int cache_hit = 0;
          int stale = 0;
          int entry_num;
          int i;
          time_t curr_time;
          curr_time = time(NULL);

          if(num_entries > 0){
            for(i=0; i < num_entries; i++){
              if(doc_entries[i].valid != 0){
                if((strcmp(doc_entries[i].hostname, host) == 0) && (strcmp(doc_entries[i].path, doc_name) == 0)){
                  entry_num = i;
                  // TODO - insert condition check for stale data here 
                  if(doc_entries[i].expires == NULL){
                    printf("LAST MOD: %d\n", ((curr_time + 6*3600) - time_s(doc_entries[i].last_modified)));
                    printf("LAST ACC: %d\n", ((curr_time + 6*3600) - time_s(doc_entries[i].last_access)));
                    if((((curr_time + 6*3600) - time_s(doc_entries[i].last_access)) < (24 * 3600)) && (((curr_time+ 6*3600) - time_s(doc_entries[i].last_modified) > (24 * 3600 * 30)))){
                      printf("Data is fresh\n");
                      cache_hit = 1;
                    } else {
                      cache_hit = 1;
                      stale = 1;
                      printf("Data is stale\n");
                    }
                  } else {
                    if((time_s(doc_entries[i].expires) - (curr_time + 6 * 3600)) > 0)
                      cache_hit = 1;
                    else {
                      cache_hit = 1;
                      stale = 1;
                      printf("Data is stale\n");
                    }
                  }
                  break;
                }
              }
            }
          }

          if(cache_hit == 1){
            printf("Cache Hit - cache entry: %d\n", entry_num);
          } else {
            printf("Cache Miss\n");
          }

          if((cache_hit == 0) || ((cache_hit == 1) && (stale == 1))){
            struct addrinfo *httpserv, *serv;

            if ((x = getaddrinfo(host, "80" , &hints, &httpserv)) != 0) {
              fprintf(stderr, " getaddrinfo error : %s\n", gai_strerror(x));
              exit(1);
            }

            int httpfd;

            for(serv = httpserv; serv != NULL; serv = serv->ai_next) {
              void *addr; char *ipver;
              char ipstr[INET6_ADDRSTRLEN];

              if (serv->ai_family == AF_INET) { // IPv4
                ipver = "IPv4"; 
              } else { // IPv6
                ipver = "IPv6";
              }
              // convert the IP to a string and print it: 
              //printf(" %s\n", ipver);

              // HTTP request socket inititialization
             	if ( (httpfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0 ) {
             		fprintf(stderr, "%s\n",strerror(errno));
                 exit(1);
             	} else {
             		printf("HTTP socket created ! \n");
             	}

              if((connect(httpfd, serv->ai_addr, serv->ai_addrlen) < 0)) {
                // Error Condition
                fprintf(stderr, "HTTP connect - %s\n", strerror(errno));
                close(httpfd);
                continue;
              } else {
                printf("HTTP - connected to %s\n", host);
                if ((send(httpfd, request, bytes_read, 0) < 0 )){ 
                  fprintf(stderr, "HTTP request - %s\n",strerror(errno));
                } else {
                  printf("HTTP request sent\n");
                }

                long int total_resp_bytes = 0;
                int resp_bytes, fwd_bytes;
                char * resp = calloc(1025, sizeof(char));
                //bzero(&resp, sizeof resp);
                int header_end = 0;
                char * header_end_delim; // Delimiter for end of header
                char * expires = NULL;
                char * date; // Last accessed
                char * last_modified=NULL;
                char * last_access;
                char * pch1;
                char * filename = (char *) calloc(15, sizeof(char));
                char * entrynum_str = (char *) calloc(3, sizeof(char));
                FILE * fh;
                char * resp_code = (char *) calloc(128, sizeof(char));

                //LRU 
                if(cache_full){  
                  printf("Cache Full\n");
                  int n;
                  int min_idx = 0;
                  int cmp;

                  for(n = 0; n <= 9; n++){
                    if(n == 0){
                      cmp = time_cmp(doc_entries[n].last_access, doc_entries[n+1].last_access);
                    } else if (n < 9){
                      cmp = time_cmp(doc_entries[min_idx].last_access, doc_entries[n+1].last_access);
                    } else {
                      break;
                    }
                    if(cmp == 1){
                      min_idx = n + 1;
                    }
                  }
                  printf("LRU entry is: %d\n", min_idx);
                  cache_idx = min_idx; // Index that is to be replaced
                }

                if(cache_full){
                  sprintf(entrynum_str, "%d", cache_idx);
                } else if (stale){
                  sprintf(entrynum_str, "%d", entry_num);
                } else {
                  sprintf(entrynum_str, "%d", num_entries);
                }
                
                

                while(1){
                  bzero(resp, 1025);
                  resp_bytes = recv(httpfd, (void*) resp, 1024, 0);
                  if(resp_bytes <= 0){
                    if(resp_bytes == 0){
                      if(to_cache)
                        fclose(fh);
                      break;
                    } else if (errno == EINTR){
                      continue;
                    } else {
                      fprintf(stderr, "HTTP response read error: %s\n", strerror(errno));
                    }
                  }
                  if((header_end_delim = strstr(resp, "\r\n\r\n")) != NULL) {
                    // Response code
                    pch = strstr(resp, "HTTP");
                    pch1 = strstr(resp, "\r\n");
                    strncpy(resp_code, (pch + 8), pch1 - (pch + 8));
                    printf("Response code: %s\n", resp_code);

                    if((pch = strstr(resp, "Last-Modified: ")) != NULL)
                    {
                      pch1 = strstr(pch,"GMT");
                      last_modified = (char*) calloc(pch1 - (pch+15) + 1, sizeof(char));
                      strncpy(last_modified, (pch + 15), pch1+3 - (pch+15));
                      to_cache = 1;
                    } else {
                      printf("No last modified\n");
                    } 
                    if((pch = strstr(resp, "Date: ")) != NULL)
                    {
                      pch1 = strstr(pch,"GMT");
                      last_access = (char*) calloc(pch1 - (pch+6) + 1, sizeof(char));
                      strncpy(last_access, (pch + 6), pch1+3 - (pch+6));
                    }
                    if((pch = strstr(resp, "Expires: ")) != NULL)
                    {
                      pch1 = strstr(pch,"GMT");
                      expires = (char*) calloc(pch1 - (pch+9) + 1, sizeof(char));
                      strncpy(expires, (pch + 9), pch1+3 - (pch+9));
                      to_cache = 1;
                    } else {
                      printf("No expires\n");
                    }
                    if(to_cache){
                      strcpy(filename, "cache-entry-");
                      strcat(filename, entrynum_str);
                      printf("File saved as: %s\n", filename);
                      fh = fopen(filename, "w");

                      if(fh == NULL){
                        printf("File open error\n");
                        exit(1);
                      }
                    }
                    header_end = 1;
                  } else {
                  }

                  //printf("Response:\n%s\n", resp);

                  if(send(fd, (void *) resp, 1024, 0) < 0){
                    printf("Send error: %s\n", strerror(errno));
                  } 

                  if(header_end == 1){
                    if(to_cache) 
                      fputs((header_end_delim + 4), fh);
                    header_end = 0;
                    //printf("Response:\n%s\n", resp);
                  } else {
                    if(to_cache) 
                      fputs(resp, fh);
                  }
                }


                if(!cache_full){
                  if(stale){
                    cache_idx = entry_num;
                  } else {
                    cache_idx = num_entries;
                  }
                }

                if(to_cache){
                  bzero(&doc_entries[cache_idx], sizeof(struct cache_entry));
                  doc_entries[cache_idx].valid = 1;
                  doc_entries[cache_idx].hostname = (char *) calloc (strlen(host)+1, sizeof(char));
                  strcpy(doc_entries[cache_idx].hostname, host);
                  doc_entries[cache_idx].path = (char *) calloc (strlen(doc_name)+1, sizeof(char));
                  strcpy(doc_entries[cache_idx].path, doc_name);
                  if(last_modified != NULL){
                    doc_entries[cache_idx].last_modified = (char *) calloc (strlen(last_modified)+1, sizeof(char));
                    strcpy(doc_entries[cache_idx].last_modified, last_modified);
                  } else {
                    printf("No last modified\n");
                  }
                  if(expires != NULL){
                    doc_entries[cache_idx].expires = (char *) calloc (strlen(expires)+1, sizeof(char));
                    strcpy(doc_entries[cache_idx].expires, expires);
                  } else {
                    printf("No expires\n");
                  }
                  doc_entries[cache_idx].last_access = (char *) calloc (strlen(last_access)+1, sizeof(char));
                  strcpy(doc_entries[cache_idx].last_access, last_access);
                  doc_entries[cache_idx].content = (char *) calloc (strlen(filename)+1, sizeof(char));
                  strcpy(doc_entries[cache_idx].content, filename);

                  if(num_entries <= 9){
                    if(!stale){
                      num_entries = num_entries + 1;
                    }
                    if(num_entries == 10){
                      cache_full = 1;
                    }
                  }
                }

                free(last_access);
                free(last_modified);
                free(entrynum_str);
                free(filename);
                free(resp);
                free(doc_name);
                break;
              }
            }
          
            if(serv == NULL) {
              fprintf(stderr, "HTTP connection failed!\n");
              close(httpfd);
              exit(1);
            }
            FD_CLR(fd,&master);
            close(httpfd);
            close(fd);

          } else { // Cache Hit
            FILE * fh;

            char * tx_buf = (char *) calloc(1025, sizeof(char));
            
            char * cache_hit_fn = (char *) calloc(15, sizeof(char));
            char * hitnum_str = (char *) calloc(3, sizeof(char));

            strcpy(cache_hit_fn, "cache-entry-");
            sprintf(hitnum_str, "%d", entry_num);
            strcat(cache_hit_fn, hitnum_str);
            fh = fopen(cache_hit_fn, "r");

            if(fh == NULL){
              printf("File open error\n");
              exit(1);
            }

            while (fgets(tx_buf, 1024, fh) != NULL){
              if(send(fd, (void *) tx_buf, 1024, 0) < 0){
                printf("Send error: %s\n", strerror(errno));
              } 
            }

            fclose(fh);
            FD_CLR(fd, &master);
            close(fd);
            break;
          }
        }
      } //fd_isset
    } //for
   } //while
 close(sockfd);
} //main

