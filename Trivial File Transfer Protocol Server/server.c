#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <signal.h>

#define MAXBUFLEN 100

struct data_pkt {
  uint16_t opcode;
  uint16_t block_num;
  char data[512];
};

//void sighandler (int signum){
//  printf("Timeout occurred\nRetransmitting...\n");
//} 
int readable_timeo(int fd, int sec){
  fd_set rset;
  struct timeval tv;

  FD_ZERO(&rset);
  FD_SET(fd, &rset);

  tv.tv_sec = sec;
  tv.tv_usec = 0;

  return (select(fd + 1, &rset, NULL, NULL, &tv));

}


int main(int argc, char* argv[]) {
  int sockfd,child_socket;

  struct addrinfo hints, *servinfo, *serv;
  struct sockaddr_in clnt_addr,servaddr;
  int getaddrinfo_rv;
  socklen_t addr_len;
  int numbytes_rx;
  int yes = 1;
  char buf[MAXBUFLEN];
  char ip_str[INET6_ADDRSTRLEN];

  // Checking for correct usage
  if(argc != 3) {
    fputs("Usage: ./server <ip> <port>\n", stderr);
    exit(1);
  }

  // Using getaddrinfo() to get server information
  memset(&hints, 0, sizeof(hints));
  // Initializing struct - hints
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;


  if(getaddrinfo_rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) {
    fprintf(stderr, "server - getaddrinfo: %s\n", gai_strerror(getaddrinfo_rv));
  }

  for(serv = servinfo; serv != NULL; serv = serv->ai_next) {
    if((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) < 0) {
      fprintf(stderr, "server - socket: %s\n", strerror(errno));
      continue;
    }

    if((bind(sockfd, serv->ai_addr, serv->ai_addrlen) < 0)) {
      // Error Condition
      fprintf(stderr, "server - bind: %s\n", strerror(errno));
      close(sockfd);
      continue;
    }
    break;
  }

  if(serv == NULL) {
    fprintf(stderr, "server - failed to bind socket\n");
    close(sockfd);
    exit(1);
  }

  freeaddrinfo(servinfo);

  while(1) {
    printf("\nserver - waiting for request...\n");

    void * addr;
    char * ipver;

    addr_len = sizeof(clnt_addr);


    // Receive request
    if((numbytes_rx = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr*) &clnt_addr, &addr_len)) == -1) {
      fprintf(stderr, "server - recvfrom: %s\n", strerror(errno));
      exit(1);
    }

    inet_ntop(clnt_addr.sin_family, (void*) &(clnt_addr.sin_addr), ip_str, sizeof(ip_str));
    printf("server - packet received from: %s\n", ip_str);
    printf("server - packet size: %d\n", numbytes_rx);


    if(!fork()) {
      //printf("\nIn Fork\n");
      servaddr.sin_port = htons(0);  // For ephemeral port
            
	 /* Creatinf socket in child process */
		 if ((child_socket = socket(AF_INET,SOCK_DGRAM,0)) == -1)
		 {
			  perror("server_child_socket error");
			  exit(1);
		 }
		 if (setsockopt(child_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
		 {
			 perror("server_child_setsockopt error");
			 exit(1);
		 }
		 if (bind(child_socket, (struct sockaddr *)&servaddr, sizeof servaddr) == -1)
		 {
			 close(child_socket);
			 perror("server_child_bind error");
			 exit(1);
		 }

    // Decode valid request
    // Packet Format
    //        2 bytes    string   1 byte     string   1 byte
    //        -----------------------------------------------
    // RRQ/  | 01/02 |  Filename  |   0  |    Mode    |   0  |
    // WRQ    -----------------------------------------------

    uint8_t opcode_0, opcode_1;
    opcode_0 = (uint8_t) buf[0];
    opcode_1 = (uint8_t) buf[1];

    int valid_rq = 0;

    // Checking for request type
    if ((opcode_0 == 0) && (opcode_1 == 1)) {
      printf("server - request received: RRQ\n");
      valid_rq = 1;
    } else if ((opcode_0 == 0) && (opcode_1 == 2)) {
      printf("server - request received: WRQ\n");
      valid_rq = 1;
    } else {
      printf("server - request received: invalid\n");
      // TODO - possibly close connection or ask to retry send illegal TFTP operation packet
    }

    if(valid_rq) { 

      char filename[100], mode[100];
      int fn_length, mode_length;
      long int file_size;

      int i = 2;
      int j = 0;

      while(buf[i] != 0) {
        filename[j] = buf[i];
        i++;
        j++;
      }
      
      fn_length = j; // File name length - including '\0'
      filename[fn_length] = '\0';
      i = i + 1; // Start of mode string
      j = 0;

      while(buf[i] != 0) {
        mode[j] = buf[i];
        i++;
        j++;
      }

      mode_length = j;
      mode[mode_length] = '\0';


      buf[numbytes_rx] = '\0'; // NULL terminating the string
      printf("server - filename: %s\n", filename);
      printf("server - mode: %s\n", mode);

    if(opcode_1 == 1) { //RRQ

      if(strcmp(mode,"netascii") == 0) {
        // Check if file exists in the current directory
        struct stat f_info;
        int stat_rv;
        unsigned long int bytes_sent = 0;
        uint16_t blk_num = 1;
        void * tx_buf = malloc(516); // Assumed that biggest packet to be sent will be the data packet
        

        if((stat_rv = stat(filename, &f_info)) < 0){
          fprintf(stderr, "server - stat error: %s\n", strerror(errno));
          // Send error message
          //       2 bytes  2 bytes        string    1 byte
          //       ----------------------------------------
          //ERROR | 05    |  ErrorCode |   ErrMsg   |   0  |
          //       ----------------------------------------
          uint16_t error_opcode = htons(5);
          memcpy(tx_buf, &error_opcode, 2);
          tx_buf += 2;
          uint16_t error_code = htons(1);
          memcpy(tx_buf, &error_code, 2);
          tx_buf += 2;
          char err_msg[513];
          bzero(err_msg, sizeof(err_msg));
          strcpy(err_msg, strerror(errno));
          memcpy(tx_buf, err_msg, strlen(err_msg)+1);
          tx_buf -= 4; // Pointing tx_buf back to starting address
          if(sendto(child_socket, tx_buf, 4 + strlen(err_msg) + 1, 0, (struct sockaddr*) &clnt_addr, addr_len) != (4 + strlen(err_msg) + 1)) {
            fprintf(stderr, "server - sendto: %s\n", strerror(errno));
            exit(1);
          }
          fprintf(stdout, "server - sending error pkt...\n");
          close(child_socket);
	  continue;
        }

        file_size = f_info.st_size;
        printf("server - size of file: %lu bytes\n", file_size);

        // Segment and send file
        //       2 bytes    2 bytes       n bytes
        //       ---------------------------------
        //DATA  | 03    |   Block #  |    Data    |
        //       ---------------------------------
        
        FILE * fh;
        fh = fopen(filename, "r");

        if(fh == NULL) {  
          fprintf(stderr, "server - file open: %s\n", strerror(errno));
        }
        
        struct data_pkt pkt;

        while(bytes_sent < file_size) {
          int bytes_read = 0;
          int extra_chars = 0;
          int pkt_data_len = 0;
          char c, c1; // c1 is a temprary char used for stuffing extra characters
          // Opcode
          uint16_t data_opcode = htons(3);
          uint16_t opcode_temp;
          uint16_t n_blk_num = htons(blk_num);

          memcpy(tx_buf, &data_opcode, 2);
          tx_buf += 2;
          memcpy(tx_buf, &n_blk_num, 2);
          tx_buf += 2;

          while(pkt_data_len != 512){
            c = getc(fh);

            if(c == EOF){ // Either EOF or error
              if(ferror(fh)) {
                fprintf(stderr, "server - file getc: %s\n", strerror(errno));
              }
              break;     
            } else if(c == '\n') {
              c1 = '\r';
              memcpy(tx_buf, &c1, 1);
              tx_buf++;
              memcpy(tx_buf, &c, 1);
              tx_buf++;
              pkt_data_len += 2;
              extra_chars++;
            } else if(c == '\r') {
              memcpy(tx_buf, &c, 1);
              tx_buf++;
              c1 = '\0';
              memcpy(tx_buf, &c1, 1);
              tx_buf++;
              pkt_data_len += 2;
              extra_chars++;
            } else {
              memcpy(tx_buf, &c, 1);
              tx_buf++;
              pkt_data_len += 1;
            }  
          }
           
          int to_cnt = 0; // Timeout count
          uint16_t * ack_blk_num = malloc(2);
          // Resetting tx_buf
          tx_buf -= (4 + pkt_data_len);
          memcpy(&opcode_temp, tx_buf, 2);

          //signal(SIGALRM, sighandler);
          int ack_recvd = 0;
          while(!ack_recvd){
            if(sendto(child_socket, tx_buf, 4 + pkt_data_len, 0, (struct sockaddr*) &clnt_addr, addr_len) != (4 + pkt_data_len)) {
              fprintf(stderr, "server - sendto: %s\n", strerror(errno));
              exit(1);
            }

            //alarm(1);
            // Wait for ACK here
            //          2 bytes    2 bytes
            //          --------------------
            //   ACK   | 04    |   Block #  |
            //          --------------------
            
            int to_retval = 0;
            to_retval = readable_timeo(child_socket, 1);
            if(to_retval > 0){
              if((numbytes_rx = recvfrom(child_socket, buf, MAXBUFLEN-1, 0, (struct sockaddr*) &clnt_addr, &addr_len)) == -1) {
                fprintf(stderr, "server - recvfrom: %s\n", strerror(errno));
                exit(1);
              } else {
                opcode_0 = (uint8_t) buf[0];
                opcode_1 = (uint8_t) buf[1];
                

                if((opcode_0 == 0) && (opcode_1 == 4)) {
                  memcpy(ack_blk_num, buf+2, 2);
                  if(*ack_blk_num == htons(blk_num)){
                    printf("server - received ack for block: %u\n", htons(*ack_blk_num));
                    ack_recvd = 1;
                    //alarm(0); // Cancel timeout
                  }
                }
              }
            } else if(to_retval == 0){
              to_cnt++;
              if(to_cnt <= 10){
                printf("Timeout occurred\nRetransmitting blk:<%d>...\n", blk_num);
                continue;
              } else {
                printf("server - no response from client\n");
                close(child_socket);
                exit(1);
              }
            } else {
              printf("server - timeout error\n");
              exit(1);
            }
          }

          bytes_sent += (pkt_data_len - extra_chars);
          // TODO - Add support for last packet having 512 bytes - send an empty data packet

          blk_num++;
          //printf("A: %lu, B: %d\n", bytes_sent, blk_num);
        }
      } else if (strcmp(mode, "octet") == 0) { // Binary mode of transfer
        // Check if file exists in the current directory
        struct stat f_info;
        int stat_rv;
        unsigned long int bytes_sent = 0;
        uint16_t blk_num = 1;
        void * tx_buf = malloc(516); // Assumed that biggest packet to be sent will be the data packet
        //printf("TX BUF: %p\n", tx_buf);
        
        if((stat_rv = stat(filename, &f_info)) < 0){
          fprintf(stderr, "server - stat error: %s\n", strerror(errno));
          // Send error message
          //       2 bytes  2 bytes        string    1 byte
          //       ----------------------------------------
          //ERROR | 05    |  ErrorCode |   ErrMsg   |   0  |
          //       ----------------------------------------
          uint16_t error_opcode = htons(5);
          memcpy(tx_buf, &error_opcode, 2);
          tx_buf += 2;
          uint16_t error_code = htons(1);
          memcpy(tx_buf, &error_code, 2);
          tx_buf += 2;
          char err_msg[513];
          bzero(err_msg, sizeof(err_msg));
          strcpy(err_msg, strerror(errno));
          memcpy(tx_buf, err_msg, strlen(err_msg)+1);
          tx_buf -= 4; // Pointing tx_buf back to starting address
          if(sendto(child_socket, tx_buf, 4 + strlen(err_msg) + 1, 0, (struct sockaddr*) &clnt_addr, addr_len) != (4 + strlen(err_msg) + 1)) {
            fprintf(stderr, "server - sendto: %s\n", strerror(errno));
            exit(1);
          }
          fprintf(stdout, "server - sending error pkt...\n");
          close(child_socket);
	  continue;
        }

        file_size = f_info.st_size;
        printf("server - size of file: %lu bytes\n", file_size);

        // Segment and send file
        //       2 bytes    2 bytes       n bytes
        //       ---------------------------------
        //DATA  | 03    |   Block #  |    Data    |
        //       ---------------------------------
        
        int fh; // Using open
        fh = open(filename, O_RDONLY);

        if(fh < 0) {  
          fprintf(stderr, "server - file open: %s\n", strerror(errno));
        }
        
        struct data_pkt pkt;


        while(bytes_sent <= file_size) {
          int bytes_read = 0;
          int pkt_data_len = 0;
          uint16_t data_opcode = htons(3);
          uint16_t opcode_temp;
          uint16_t n_blk_num = htons(blk_num);
          int last_pkt = 0;

          memcpy(tx_buf, &data_opcode, 2);
          tx_buf += 2;
          memcpy(tx_buf, &n_blk_num, 2);
          tx_buf += 2;
          
          if(bytes_sent < file_size){
            if(file_size < 512){
              pkt_data_len = file_size;
              bytes_read = read(fh, tx_buf, (size_t) pkt_data_len);
            } else {
              if((file_size - bytes_sent) > 512){
                pkt_data_len = 512;
                bytes_read = read(fh, tx_buf, (size_t) pkt_data_len);
              } else {
                pkt_data_len = file_size - bytes_sent;
                bytes_read = read(fh, tx_buf, (size_t) pkt_data_len);
              }
            }
          } else { // bytes_sent == file_size
            if((file_size % 512) != 0){
              break;
            } else {
              last_pkt = 1;
            }
          }
          
          int to_cnt = 0; // Timeout count
          uint16_t * ack_blk_num = malloc(2);
          // Resetting tx_buf
          tx_buf -= 4;

          //signal(SIGALRM, sighandler);
          int ack_recvd = 0;
          while(!ack_recvd){
            if(sendto(child_socket, tx_buf, 4 + pkt_data_len, 0, (struct sockaddr*) &clnt_addr, addr_len) != (4 + pkt_data_len)) {
              fprintf(stderr, "server - sendto: %s\n", strerror(errno));
              exit(1);
            }

            //alarm(1);
            // Wait for ACK here
            //          2 bytes    2 bytes
            //          --------------------
            //   ACK   | 04    |   Block #  |
            //          --------------------
            int to_retval = 0;
            to_retval = readable_timeo(child_socket, 1);
            if(to_retval > 0){
              if((numbytes_rx = recvfrom(child_socket, buf, MAXBUFLEN-1, 0, (struct sockaddr*) &clnt_addr, &addr_len)) == -1) {
                fprintf(stderr, "server - recvfrom: %s\n", strerror(errno));
                exit(1);
              } else {
                opcode_0 = (uint8_t) buf[0];
                opcode_1 = (uint8_t) buf[1];
                

                if((opcode_0 == 0) && (opcode_1 == 4)) {
                  memcpy(ack_blk_num, buf+2, 2);
                  if(*ack_blk_num == htons(blk_num)){
                    printf("server - received ack for block: %u\n", htons(*ack_blk_num));
                    ack_recvd = 1;
                    alarm(0); // Cancel timeout
                  }
                }
              }
            } else if(to_retval == 0){
              to_cnt++;
              if(to_cnt <= 10){
                printf("Timeout occurred\nRetransmitting blk:<%d>...\n", blk_num);
                continue;
              } else {
                printf("server - no response from client\n");
                close(child_socket);
                exit(1);
              }
            } else {
              printf("server - timeout error\n");
              exit(1);
            }
          }

          bytes_sent += (pkt_data_len);

          if(last_pkt) { // Last packet acked
              close(child_socket);
              break;
          }

          blk_num++;
      
        }
      } else {
        fprintf(stderr, "server - invalid transfer mode\n");
        continue;
      }

      close(child_socket);

    } else if (opcode_1 == 2) { //WRQ
		
			
		 if(strcmp(mode,"netascii") == 0) {
			FILE * fh;
			fh = fopen(filename, "wx");

			if(fh == NULL) {  
			  fprintf(stderr, "server - opening file for write: %s\n", strerror(errno));
			  void * tx_buf = malloc(516);
			  uint16_t error_opcode = htons(5);
			  memcpy(tx_buf, &error_opcode, 2);
			  tx_buf += 2;
			  uint16_t error_code = htons(1);
			  memcpy(tx_buf, &error_code, 2);
			  tx_buf += 2;
			  char err_msg[513];
			  bzero(err_msg, sizeof(err_msg));
			  strcpy(err_msg, strerror(errno));
			  memcpy(tx_buf, err_msg, strlen(err_msg)+1);
			  tx_buf -= 4; // Pointing tx_buf back to starting address
			  if(sendto(child_socket, tx_buf, 4 + strlen(err_msg) + 1, 0, (struct sockaddr*) &clnt_addr, addr_len) != (4 + strlen(err_msg) + 1)) {
				fprintf(stderr, "server - sendto: %s\n", strerror(errno));
				exit(1);
			  }
			  fprintf(stdout, "server - sending error pkt...\n");
                          close(child_socket);
			  continue;
			}

			uint16_t blk_num = 0;
			char buf[516];
			int numbytes_rx = 516;
			uint16_t * ack_blk_num = malloc(2);
	        long num_bytes = 0;		
			
				
				void * tx_buf = malloc(4); //ack packet
				uint16_t data_opcode = ntohs(4);
			  
				uint16_t n_blk_num = ntohs(blk_num);
				int last_pkt = 0;
				
				memcpy(tx_buf, &data_opcode, 2);
				tx_buf += 2;
				memcpy(tx_buf, &n_blk_num, 2);
				tx_buf -=2;
			
			
            if(sendto(child_socket, tx_buf, 4 , 0, (struct sockaddr*) &clnt_addr, addr_len) != 4 ) {
              fprintf(stderr, "server - sendto: %s\n", strerror(errno));
              exit(1);
            }
            
			while(numbytes_rx == 516){
			
            if((numbytes_rx = recvfrom(child_socket, buf, 516, 0, (struct sockaddr*) &clnt_addr, &addr_len)) == -1) {
              fprintf(stderr, "server - recvfrom client: %s\n", strerror(errno));
              exit(1);
            } else {
				
              opcode_0 = (uint8_t) buf[0];
              opcode_1 = (uint8_t) buf[1];
              num_bytes += (numbytes_rx-4);

              char *data = malloc(512);
              if((opcode_0 == 0) && (opcode_1 == 3) ) {
                memcpy(ack_blk_num, buf+2, 2);
                printf("server - received data block : %u\n", htons(*ack_blk_num));
				printf("server - number of bytes recieved : %d\n",numbytes_rx);
				memcpy(data, buf+4, numbytes_rx-4);
				
				char c,ch;
				int count;
				for(count=0;count<numbytes_rx-4;count++) {
					c = *data;
					data++;
					ch = *data;
					
					//printf("\n\n %c %c",c,ch);
					if((c=='\r' )&& (ch=='\n') ) {
						putc(ch,fh);
						data++;
						count++;
					}
					else if((c=='\r') && (ch=='\0')) { 
						putc(ch,fh);
						data++;
						count++;
					}
					else {
					putc(c,fh);
					}
				}
                }
				blk_num = htons(*ack_blk_num);
				uint16_t n_blk_num = ntohs(blk_num);
				
				memcpy(tx_buf, &data_opcode, 2);
				tx_buf += 2;
				memcpy(tx_buf, &n_blk_num, 2);
				tx_buf -=2;
			  if(sendto(child_socket, tx_buf, 4 , 0, (struct sockaddr*) &clnt_addr, addr_len) != 4 ) {
              fprintf(stderr, "server - sendto: %s\n", strerror(errno));
              exit(1);
            }
			  
            }			
          }
		  fclose(fh);
		  printf("server - recieved %li bytes of data\n",num_bytes);
                  close(child_socket);

		 } else if (strcmp(mode, "octet") == 0) {
			
			int fh; // Using open
			fh = open(filename, O_WRONLY|O_CREAT|O_EXCL, 0644);

			if(fh < 0) {  
			  fprintf(stderr, "server - opening file for write: %s\n", strerror(errno));
			  void * tx_buf = malloc(516);
			  uint16_t error_opcode = htons(5);
			  memcpy(tx_buf, &error_opcode, 2);
			  tx_buf += 2;
			  uint16_t error_code = htons(1);
			  memcpy(tx_buf, &error_code, 2);
			  tx_buf += 2;
			  char err_msg[513];
			  bzero(err_msg, sizeof(err_msg));
			  strcpy(err_msg, strerror(errno));
			  memcpy(tx_buf, err_msg, strlen(err_msg)+1);
			  tx_buf -= 4; // Pointing tx_buf back to starting address
			  if(sendto(child_socket, tx_buf, 4 + strlen(err_msg) + 1, 0, (struct sockaddr*) &clnt_addr, addr_len) != (4 + strlen(err_msg) + 1)) {
				fprintf(stderr, "server - sendto: %s\n", strerror(errno));
				exit(1);
			  }
			  fprintf(stdout, "server - sending error pkt...\n");
                          close(child_socket);
			  continue;
			}
			
			uint16_t blk_num = 0;
			char buf[516];
			int numbytes_rx = 516;
			uint16_t * ack_blk_num = malloc(2);
			long num_bytes = 0;
			
				
				void * tx_buf = malloc(4); //ack packet
				uint16_t data_opcode = ntohs(4);
			  
				uint16_t n_blk_num = ntohs(blk_num);
				int last_pkt = 0;
				
				memcpy(tx_buf, &data_opcode, 2);
				tx_buf += 2;
				memcpy(tx_buf, &n_blk_num, 2);
				tx_buf -=2;
			
			
            if(sendto(child_socket, tx_buf, 4 , 0, (struct sockaddr*) &clnt_addr, addr_len) != 4 ) {
              fprintf(stderr, "server - sendto: %s\n", strerror(errno));
              exit(1);
            }
            
			while(numbytes_rx == 516){
			
            if((numbytes_rx = recvfrom(child_socket, buf, 516, 0, (struct sockaddr*) &clnt_addr, &addr_len)) == -1) {
              fprintf(stderr, "server - recvfrom client: %s\n", strerror(errno));
              exit(1);
            } else {
				
              opcode_0 = (uint8_t) buf[0];
              opcode_1 = (uint8_t) buf[1];
              num_bytes += (numbytes_rx-4);
			  
              void *data = malloc(512);
              if((opcode_0 == 0) && (opcode_1 == 3) ) {
                memcpy(ack_blk_num, buf+2, 2);
                printf("server - received data block : %u\n", htons(*ack_blk_num));
				printf("server - number of bytes recieved : %d\n",numbytes_rx);
				memcpy(data, buf+4, numbytes_rx-4);
				
				//fputs(data, fh);
				write(fh,data,numbytes_rx-4);
								
              }
				blk_num = htons(*ack_blk_num);
				uint16_t n_blk_num = ntohs(blk_num);
				
				memcpy(tx_buf, &data_opcode, 2);
				tx_buf += 2;
				memcpy(tx_buf, &n_blk_num, 2);
				tx_buf -=2;
			  if(sendto(child_socket, tx_buf, 4 , 0, (struct sockaddr*) &clnt_addr, addr_len) != 4 ) {
              fprintf(stderr, "server - sendto: %s\n", strerror(errno));
              exit(1);
            }
			  
            }		
          }
		  //fclose(fh);
		  close(child_socket);
		  printf("server - recieved %li bytes of data\n",num_bytes);
		 }
		 
	
	     } //opcode_1==1
		
    	} 
     } //fork
  
  } //while
  close(sockfd);
  return 0;
}
