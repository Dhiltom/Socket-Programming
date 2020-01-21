TCP Simple Broadcast Chat Server and Client

The package contains 4 files: 
1.client.c 
2.server.c 
3.README.txt 
4.makefile 
 
To compile the code, run the makefile: make 

Run the server by using the command line: ./server <Server_IP> <Server_Port> <Max_Clients> 
Run the client by using the command line: ./client <Username> <Server_IP> <Server_Port>
To clean the executables run: make clean

Bonus features in this code include :- 
1. Server and Client side IPv4/IPv6 implementation. 
2. ACK, NAK, ONLINE, OFFLINE messages. 
3. Client side IDLE (>10 seconds) feature


SERVER: 

Server side mainly consists of socket(), bind(), listen(), accept(), select() functions.
First create socket, and then bind the socket to the assigned IP address and port, and then listen to the port. Loop through the file descriptors array and make appropriate response to different file descriptors.

The server sends following message types:- 

1. NAK     - If client limit is reached or the username already exists, a NAK message is sent to the client and connection is closed. 
2. ACK     - Contains the list of usernames in the chat session and total no of clients is sent to the client. 
3. ONLINE  - When a new client is acknowledged in the chatroom, ONLINE message goes to all other clients. 
4. OFFLINE - When a client leaves chatroom, OFFLINE message goes to all other clients.
5. IDLE    - When a client is idle more than 10 seconds after sending message, it sends an IDLE message to server.The server then sends the message to all other clients. 
6. FWD     - Server forwards message sent by a client to all other clients. 


CLIENT: 

Client side mainly consists of socket(), connect(), select() functions.
First create socket, then connect the client to server, then select from standard input and received messages from server.

The Client begins by sending the JOIN message to the server, with the appropriate header values chosen. This section is followed by the prompt to send a message. 
However, if the channels pertaining to receiving a message trigger instead, the select() function chooses to enable the receive functionality, at which the header values are compared against one another to determine what type of message was received. These messages are then processed into terminal messages in readable format. 
The optional features (ACK, NAK, etc.) were included and accounted for in this code. 
In addition, the IDLE feature was included. When a client is idle for more than 10 seconds, it sends an idle message to server and server in turn forwards it to everyone. 


Error Handling: 
We have used strerror() function to print out the description of the errors.
Usage : strerror(errno)


