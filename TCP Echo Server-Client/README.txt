TCP Echo Server & Client

The package contains 5 files: 
1. echoclnt.c 
2. echoserv.c 
3. README.pdf
4. README.txt
5. makefile  

To compile the code, run the makefile: make 
Run the server by using the command line:/echoserv port 
Run the client by using the command line:/echoclnt localhost port
In this project, the IP address used is 127.0.0.1 
To clean the executables run: make clean


ARCHITECTURE 

Server side:
Server side mainly consists of socket(), bind(), listen(), accept(), fork(), readline() and writen() functions.
First we create a socket, then bind the socket to the assigned IP address and port, and then listen to the port for client connections. When client connects to the server, accept it and then use fork() to create a child process to receive and send message back to client.

Client side:
Client side mainly consists of socket(), connect(), writen() and readline() functions.
First create the socket, then connect the client to server, then using the fgets() to get characters from standard input, and then send the data to server, finally receive the echoed message from server and display it on screen.

Error Handling: 
We have used strerror() function to print out the description of the errors.
Usage : strerror(errno)
