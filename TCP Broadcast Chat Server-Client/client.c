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

#define ATTR_USERNAME 2
#define ATTR_MESSAGE 4
#define ATTR_REASON 1
#define ATTR_CLIENT_CNT 3

#define MSG_JOIN 2
#define MSG_SEND 4
#define MSG_FWD 3
#define MSG_ACK 7
#define MSG_NAK 5
#define MSG_ONLINE 8
#define MSG_OFFLINE 6
#define MSG_IDLE 9

// Global struct definitions
// SBCP attribute struct
struct SBCP_attribute {
  uint16_t type;
  uint16_t length;
  char payload[512];
};

// SBCP message struct
struct SBCP_message {
  unsigned int version: 9;
  unsigned int type: 7;
  uint16_t length;
  struct SBCP_attribute payload[2];
};


// Readtill eof function - Unused
int read_till_eof(int sockfd, char * recvbuf, int rx_buf_size) {
  char * refptr;
  int read_rtn;

  refptr = recvbuf;

  while(read_rtn != 0) { // not EOF
    if(read_rtn = recv(sockfd, refptr, rx_buf_size, 0) >= 0) {
      if(read_rtn < rx_buf_size) {
        if(errno != EINTR)
          break;
        else
          continue;
      }
    } else {
      if(errno != EINTR){ // If not soft interrupt
        fprintf(stderr, "Read error: %s\n", strerror(errno));
        return(-1); // Error condition
      } else {
        continue;
      }
    }
  }
  return(0);
}

// Function: writen - Unused
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


// Main
int main(int argc, char* argv[]) {
  int sockfd;

  struct addrinfo hints, *servinfo, *serv;
  int getaddrinfo_rv;

  struct SBCP_attribute username;
  struct SBCP_message join;
  char name[17];

  struct timeval tv;

  // Checking for correct usage
  if(argc != 4) {
    fputs("Usage: ./client <username> <server ip> <port>\n", stderr);
    exit(1);
  }

  // Checking for the length of the username
  if(strlen(argv[1]) > 16) {
    fputs("Error: Username must not be longer than 16 characters\n", stderr);
    exit(1);
  }

  bzero(&name, sizeof(name));
  memcpy(name, argv[1], strlen(argv[1]));


  // Using getaddrinfo() to get server information
  memset(&hints, 0, sizeof(hints));
  // Initializing struct - hints
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if(getaddrinfo_rv = getaddrinfo(argv[2], argv[3], &hints, &servinfo)) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_rv));
  }

  for(serv = servinfo; serv != NULL; serv = serv->ai_next) {

    if((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) < 0) {
      fprintf(stderr, "socket: %s\n", strerror(errno));
      continue;
    }

    if((connect(sockfd, serv->ai_addr, serv->ai_addrlen) < 0)) {
      // Error Condition
      fprintf(stderr, "connect: %s\n", strerror(errno));
      close(sockfd);
      continue;
    } else {
      // Send join request
      username.type = 2;
      strcpy(username.payload, name);
      username.length = 4 + strlen(username.payload) + (strlen(username.payload) % 4);
      // Pad the attribute struct to 4 byte boundary
      char * ptr;
      int i;
      ptr = username.payload;
      //printf("PTR: %x\n", (unsigned int) ptr);
      ptr += strlen(name);
      //printf("PTR: %x\n", (unsigned int) ptr);
      for(i=0; i < (strlen(name) % 4); i++){
        *ptr = 0;
        ptr++;
      }

      join.version = 3;
      join.type = 2;
      join.length = 4 + username.length;
      
      join.payload[0].type = username.type;
      strcpy(join.payload[0].payload, username.payload);

      if ((write(sockfd,(void *) &join, join.length) < 0 )){ 
        fprintf(stderr, " %s\n",strerror(errno));
      }
    }
    break;
  }

  if(serv == NULL) {
    fprintf(stderr, "connection failed!\n");
    close(sockfd);
    exit(1);
  }

  void * addr;
  char * ipver;
  char ipstr[INET6_ADDRSTRLEN];

  // Debug - Begin - Supports both IPv4 and IPv6
  if(serv->ai_family == AF_INET){
    struct sockaddr_in * inet = (struct sockaddr_in *) serv->ai_addr;
    addr = (void*) &(inet->sin_addr);
    ipver = "IPv4";
  } else {
    struct sockaddr_in6 * inet6 = (struct sockaddr_in6 *) serv->ai_addr;
    addr = (void*) &(inet6->sin6_addr);
    ipver = "IPv6";
  }

  inet_ntop(serv->ai_family, addr, ipstr, serv->ai_addrlen);

  fprintf(stdout, "Connected to: %s on port: %s\n", ipstr, argv[3]);
  // Debug - End

  freeaddrinfo(servinfo);
  fprintf(stdout, "Connected to the chat server with username: %s\n", username.payload);

  // Creating readfds set
  fd_set readfds;
  fd_set master;
  int fdmax;

  FD_ZERO(&readfds); // Initialize to 0
  FD_ZERO(&master); // Initialize to 0

  fdmax = sockfd; //Unnecessary but used it just for the sake of it
  int fd;

  long int t1;
  t1 = 0;

  int init;
  init = 1;


  struct timeval timestamp; // Used for tracking IDLE period 

  struct SBCP_message idle;
  idle.version = 3;
  idle.type = MSG_IDLE;
  idle.length = 4;

  FD_SET(0, &readfds);
  FD_SET(sockfd, &readfds);
  
  int c = 0; 

  for(;;) { //Main loop
    master = readfds; 
    
    tv.tv_sec = 10;
    //tv.tv_usec = 200000;
    
    if(select(fdmax+1, &master, NULL, NULL, &tv) < 0) { 
      fprintf(stderr, "Select error: %s\n", strerror(errno));
      exit(10);
    }
    
    gettimeofday(&timestamp, NULL);
    //printf("Timestamp: %ld\n", timestamp.tv_sec);
    if (((timestamp.tv_sec - t1) >= 10) && (init == 0)){ // Send IDLE message
      write(sockfd, (struct SBCP_message *) &idle, idle.length);
      init = 1;
      //printf("IDLE\n");
    }
  
    if (c == 0) {  
      t1 = timestamp.tv_sec;
      init = 0;  
      c = 1;  
    }
   
    for(fd = 0; fd <= fdmax; fd++){
      if(FD_ISSET(fd, &master)){
        //printf("FD: %d\n", fd);
        if(fd == 0){

          struct SBCP_message send;
          char msg_text[512+1];
          
          bzero(msg_text, sizeof(msg_text));
          read(0, msg_text, 512);
          printf("<SEND> <%s>: %s",name, msg_text);

          send.version = 3;
          send.type = 4;
          //send.length
          //attribute.type and //attribute.length

          strcpy(send.payload[0].payload, msg_text);

          write(sockfd, (void *) &send, sizeof(send));
          t1 = timestamp.tv_sec;
          init = 0;

        } else if(fd == sockfd) {
          struct SBCP_message junk;
          struct SBCP_message rxmsg;
          int readrtn;
          bzero(&rxmsg, sizeof(rxmsg));
          readrtn = read(fd, (struct SBCP_message *) &rxmsg, sizeof(rxmsg));
          //printf("Read rtn: %d", readrtn);
          if(readrtn <= 0){
            printf("Server closed connection!\nClosing Socket!\n");
            close(sockfd);
            exit(1);
          } else if(rxmsg.version == 3){

            if(rxmsg.type == MSG_ACK){ // ACK message received
              printf("<ACK>: Number of clients in the session = %s\n", rxmsg.payload[0].payload);
              printf("List of users: %s\n", rxmsg.payload[1].payload);

            } else if (rxmsg.type == MSG_FWD){ // FWD message received
              char sendername[16+1];
              char msg_text[512+1];
              bzero(sendername, sizeof(sendername));
              bzero(msg_text, sizeof(msg_text));
              strcpy(sendername, rxmsg.payload[1].payload);
              strcpy(msg_text, rxmsg.payload[0].payload);
              printf("<FWD> <%s>: %s\n", sendername, msg_text);

            } else if (rxmsg.type == MSG_NAK){ // NAK message received
              char reason[32+1];
              bzero(reason, sizeof(reason));
              strcpy(reason, rxmsg.payload[0].payload);
              printf("<NAK> %s\n", reason);
              close(fd);
              exit(2);

            } else if (rxmsg.type == MSG_ONLINE){ // ONLINE message received
              char useronline[16+1];
              bzero(useronline, sizeof(useronline));
              strcpy(useronline, rxmsg.payload[0].payload);
              printf("<ONLINE> %s has joined the session\n", useronline);

            } else if (rxmsg.type == MSG_OFFLINE){ // OFFLINE message recieved
              char useroffline[16+1];
              bzero(useroffline, sizeof(useroffline));
              strcpy(useroffline, rxmsg.payload[0].payload);
              printf("<OFFLINE> %s has left the session\n", useroffline);
              
            } else if (rxmsg.type == MSG_IDLE) { // IDLE message recieved
              char useridle[16+1];
              bzero(useridle, sizeof(useridle));
              strcpy(useridle, rxmsg.payload[0].payload);
              printf("<IDLE> %s is idle\n", useridle);
            }

          } else { // Protocol Version Check
            fprintf(stderr, "Received protocol format does not match! Packet Dropped\n");
            bzero(&rxmsg, sizeof(rxmsg));
          }
        }
      }
    }
  }

 printf("\n\n Client out");
 return 0;
}
