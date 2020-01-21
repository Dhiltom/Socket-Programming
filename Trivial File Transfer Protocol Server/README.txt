TFTP Server implementation

The package contains 3 files:
1.server.c
2.README.txt
3.makefile

To compile the code, run the makefile: make

Run the server by using the command line: ./server <Server_IP> <Server_Port>
To clean the executables run: make clean
Connecting native client to server: tftp <Server_IP> <Server_Port>

Bonus features in this code include : WRQ feature

CLIENT:
We used native linux tftp client for the testing of our tftp server.

SERVER:
We have implemented the server code in C. All the processes including the binding to a port and the socket creation were done here.
The main function included the listening through the specifc port and after getting the request from the client, fork() command
forks a child process and awaits another connection.
This tftp server supports two types of file formats- NETASCII and OCTET.
The file is read 512 bytes at one time. When we receive the final ACK, the new socket gets closed and the server cleans up the child.
If file can not be read, the server will have error alert and send a message to client.
If transmission was interrupted, the server will wait for the client for 1 second, and try to resend packet.
If no ACK from server after 10 resending attempts, then server will think client timed out and terminate this tranmission.

Error Handling:
We have used strerror() function to print out the description of the errors.
Usage : strerror(errno)

