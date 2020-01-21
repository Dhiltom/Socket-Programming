#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXLEN 10

// Function: writen
// Writes num_bytes bytes to a socket or -1 on error.
// Usage: writen (int sockfd, char* buffptr, size_t num_bytes)

size_t writen (int sockfd, char* sendbuf, size_t num_bytes) {
  size_t num_bytes_sent, num_bytes_pending;

  num_bytes_pending = num_bytes;

  while(num_bytes_pending > 0) {
    num_bytes_sent = send(sockfd, sendbuf, num_bytes_pending, 0);
    //printf("IN FUNC: %d\n", num_bytes_sent);

    if(num_bytes_sent < 0){ // Error condition check
      if (errno == EINTR) { // Retry write() - soft interrupt
        num_bytes_sent = 0;
      } else {
        return(-1);
      }
    }

    sendbuf = sendbuf + num_bytes_sent;
    num_bytes_pending = num_bytes_pending - num_bytes_sent; // Run loop until pedning bytes = 0
  }
  return(num_bytes-num_bytes_pending);
}


// Function: eff_read
// Efficient read using static variables as suggested in the assignment
// Used UNP book for reference
// Usage: eff_read(int sockfd, char* buffptr)
/*
static char *buf_ptr;
static int buf_cnt;
static char buf[MAXLEN];

int eff_read (int sockfd, char *rx_buf_ptr) {

  if(buf_cnt <= 0) {
  start_again:
    buf_cnt = read(sockfd, buf, sizeof(buf));

    if(buf_cnt < 0) {
      if(errno == EINTR) // Soft error
        goto start_again;
      else {
        printf("Error in eff_read: %s\n", strerror(errno));
        return(-1); // Error condition
      }
    } else if (buf_cnt == 0) {
        return(0);
    }

    buf_ptr = buf;
  }

  buf_cnt = buf_cnt - 1;
  *rx_buf_ptr = *buf_ptr; // Used the static buffer for all reads and only copying it to the string that will be read in the end

  buf_ptr++; // Increment pointer to next position
  return(1);

}*/

// Function: readline
// Returns the number of bytes read from a socket or -1 on error.
// Usage: readline(int sockfd, char* buffptr, size_t maxlen)

int readline (int sockfd, char *recvbuf, int num_bytes) {
  char byte_read; // To write 1 byte at a time
  char *refptr;
  int i;
  int read_rtn;

  refptr = recvbuf; // Using a reference pointer to traverse the array
  i = 0;

  while(i < num_bytes) {

    read_rtn = read(sockfd, &byte_read, 1);
    //printf("Read return: %d, Byte read: %c\n", read_rtn, byte_read);

    if(read_rtn == 1) {
      *refptr = byte_read;
      refptr++;
      i++;
      if(byte_read == '\n')
        break;
    } else if(read_rtn == 0) {
      *refptr = 0;
      fprintf(stdout, "EOF encountered\n");
      return(i-1); // No byte read in this iteration
    } else {
      if(errno != EINTR)
        return(-1); // Error
      else
        continue; 
    }
  }
  
  *refptr = 0;
  return(i);
}

int main (int argc, char *argv[]) {
  int sockfd;
  char tx_buf[100]; // Buffer for reading STDIN
  char rx_buf[MAXLEN]; // Received data buffer
  int bytes_written;
  int bytes_read;

  struct sockaddr_in servinfo;
  // Checking for 2 command-line arguments
  if(argc != 3) {
    fprintf(stderr, "Usage: client IpAdr Port\n");
    exit(1);
  }

  // Creating a socket
  if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    fprintf(stderr, "Socket error: %s\n", strerror(errno));
    exit(1);
  } else {
    printf("Socket created\n");
  };
  
  memset(&servinfo, 0, sizeof(servinfo)); // Initializing struct instance to 0
  servinfo.sin_family = AF_INET; //IPv4
  servinfo.sin_port = htons(atoi(argv[2])); // Network byte order

  // Converting ASCII input into network ad
  if(inet_pton(AF_INET, argv[1], &servinfo.sin_addr) <= 0) {
    fprintf(stderr, "inet_pton error for IP: %s", argv[1]);
    exit(1);
  }

  // Connecting to the server
  if(connect(sockfd, (struct sockaddr *) &servinfo, sizeof(servinfo))) { // Typecasting sockaddr_in to sockaddr
    fprintf(stderr, "Connection error: %s\n", strerror(errno));
    exit(1);
  } else {
    printf("Connected to server on port: %s\n", argv[2]);
  }

  while(1) {
    bzero(&tx_buf, 100); // Initialize array to 0
    bzero(&rx_buf, sizeof(rx_buf)); // Initialize array to 0
    printf("Enter message: ");
    if((fgets(tx_buf, 100, stdin) == NULL) || feof(stdin)) {//Accept input from STDIN
      printf("\nEOF detected: Closing socket!\n");
      close(sockfd);
      break;
    }
    //printf("Input: %s : %d \n", tx_buf, strlen(tx_buf));
    
    if((bytes_written = writen(sockfd, tx_buf, strlen(tx_buf))) < 0) {
      fprintf(stdin, "Write error: %s\n", strerror(errno));
      exit(1);
    } else {
      printf("Bytes written: %d\nString written: %s\n", bytes_written, tx_buf);
    }

    while(1) { 
      bytes_read = readline(sockfd,rx_buf,MAXLEN);
      //printf("Bytes read: %d\n", bytes_read);
      /*fputs(rx_buf,stdout);
      break;*/
      if(bytes_read < 0){
        printf("Error in readline\n");
        break;
      } else {
        if(bytes_read != 0) {
          printf("\nMessage received: ");
          fputs(rx_buf,stdout);
          if(bytes_read < MAXLEN) {
            break;
          }
        } else {
          break;
        }
      }
    }
  }
}
