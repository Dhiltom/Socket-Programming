Simple HTTP Proxy

The package for this particular assignment contains the following 4 files:
1.server.c
2.client.c
3.Makefile
4.README.txt

To compile the code, run the Makefile: make
Run the server by using the command line: ./server localhost server_port
Run the client by using the command line: ./client localhost server_port URL
To clean the executables run: make clean

We have implemented the HTTP proxy server and HTTP client. 
The proxy server starts on a user specified port and listens for incoming HTTP requests.
The proxy server behaves in the following fashion:
When the server receives a request from a client, it firsts checks its own cached data . If it has an appropriate cache entry 
then the request is served from the cached data but if does not has a valid cache entry then :
a) the request is proxied to the intended destination 
b) the response is then sent by the proxy server to the client 
c) the response served is also cached by proxy 
The entries in the proxy are not used if they are stale, which in turn is indicated by the Expires header. 
This header spcifies the time and date and after that date and time is passed the cache entry is then considered stale.
