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

int main(int argc, char* argv[]) {
  int sockfd;

  struct addrinfo hints, *servinfo, *serv;
  int getaddrinfo_rv;

  if(argc != 4) {
    fputs("Usage: ./client <proxy ip> <proxy port> <URL>\n", stderr);
    exit(1);
  }

        char *p1,*p2,*page,*hostname;
        int length=strlen(argv[3]);
       
	p1 = strstr(argv[3], "http://");
        p2 = strstr(argv[3], "https://");
        if (p1) { 
        p1+=7;
        //printf("\n%s",p1);
        page = strstr(p1, "/");
        page+=1;
	//printf("\n%s",page);

        hostname = (char *)calloc(page-p1, sizeof(char));
        sscanf(p1, "%[^/]", hostname);
        //printf("\n%s",hostname);
        } else if (p2) {
          p2+=8;
          page = strstr(p2, "/");
          page+=1;
          hostname = (char *)calloc(page-p2, sizeof(char));
          sscanf(p2, "%[^/]", hostname);
        } else {
        p1 = argv[3];
        page = strstr(p1,"/");
        page+=1;
        hostname = (char *)calloc(page-p1, sizeof(char));
        sscanf(p1, "%[^/]", hostname);
        }       
        char *get_req;
        char *get_req_red;
        get_req = (char *)calloc(strlen(hostname)+strlen(page)+20, sizeof(char));
        get_req_red = (char *)calloc(strlen(hostname)+strlen(page)+20, sizeof(char));
        strcpy(get_req,"GET /");
        strcat(get_req,page);
        strcat(get_req," HTTP/1.0\r\nHost: ");
        strcat(get_req,hostname);
        strcat(get_req,"\r\n\r\n");
        //strcat(get_req,"\nIf-Modified-Since: "); //bonus
        //printf("### %d\n", strlen(get_req));
        printf("\n%s\n",get_req); 
        strcpy(get_req_red, get_req);
     while (1) {

        char *filename,*f;
        filename = page;
        f = strstr(page,"/");
        while (f) {
        filename = f+1;
        f = strstr(page,"/");
        page = f+1;
        } 
        FILE *fh;
        printf("\nFileName : %s\n\n",filename);
        fh = fopen(filename, "w");
        if(fh == NULL) {
                 fprintf(stderr, "client - opening file for write: %s\n", strerror(errno));
                 break;
        }

     memset(&hints, 0, sizeof(hints));
     hints.ai_family = AF_UNSPEC;
     hints.ai_socktype = SOCK_STREAM;

 if(getaddrinfo_rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_rv));
  }

  for(serv = servinfo; serv != NULL; serv = serv->ai_next) {

    if((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) < 0) {
      fprintf(stderr, "socket: %s\n", strerror(errno));
      continue;
    }

  if((connect(sockfd, serv->ai_addr, serv->ai_addrlen) < 0)) {
    fprintf(stderr, "connect: %s\n", strerror(errno));
      close(sockfd);
      continue;
    }

        int num_bytes_sent;
        //printf("### %d\n", strlen(get_req));
        //puts(get_req);
        //printf("### %d\n", strlen(get_req_red));
        //puts(get_req);
        num_bytes_sent = send(sockfd, get_req, strlen(get_req), 0);

        if (num_bytes_sent<0) {
          fprintf(stderr, "Write error: %s\n", strerror(errno));
        }


        int size = 2048, num_bytes;
        int first = 1;

        while(1)
	{
                char *buf = (char *) calloc((size+1),sizeof(char));
		num_bytes = recv(sockfd,buf,2048,0);
                if(num_bytes<=0){
			break;
		}

		if (strstr(buf, "404 Not Found")){
			printf("\n404 Not Found!\n");
			break;
		}

               if (first) {
                char *p = strstr(buf, "\r\n\r\n") ;
		if(p){
			fputs(p+4,fh);
		}
                first = 0;
               } else {
                 fputs(buf,fh);  
               }
 
        } //while 
   
  fclose(fh);
  exit(1);
 }
} //for

  return 0;
} //main
