#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>

using namespace std;

#define PORT 8080

const int BUFFERSIZE = 2048;

int main(int argc, char const *argv[])
{
	int proxySock;
	struct sockaddr_in proxyAddr;

	// Create a TCP socket
	if ((proxySock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cout << "Error creating socket" << endl;
		exit(1);
	}

	// Clearing up the port
	int option = 1;
	if (setsockopt(proxySock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) < 0)
	{
		cout << "Error in clearing up PORT" << endl;
		exit(1);
	}

	memset(&proxyAddr, 0, sizeof(proxyAddr));
	proxyAddr.sin_family = AF_INET;
	proxyAddr.sin_addr.s_addr = INADDR_ANY; // Initialize IP of server
	proxyAddr.sin_port = htons(PORT);		// Initialize port

	// Binding the TCP socket
	if (bind(proxySock, (struct sockaddr *)&proxyAddr, sizeof(proxyAddr)) < 0)
	{
		cout << "Binding failed" << endl;
		exit(1);
	}

	int clientSock;
	struct sockaddr_in clientAddr;

	// Listening for connection
	if (listen(proxySock, 1) < 0)
	{
		cout << "listen() failed" << endl;
		exit(1);
	}

	unsigned int c_addrSize = sizeof(clientAddr);
	if ((clientSock = accept(proxySock, (struct sockaddr *)&clientAddr, &c_addrSize)) < 0)
	{
		cout << "Could not connect" << endl;
	}

	char recvBuffer[BUFFERSIZE];

	while (true)
	{
		// clearing the buffer
		memset(&recvBuffer, 0, BUFFERSIZE);

		int recvBytes = recv(clientSock, (char *)&recvBuffer, BUFFERSIZE, 0); // receiving the request from a client

		if (recvBytes <= 0)
		{
			cout << "Did not receive any messages" << endl;
			exit(1);
		}

		cout << recvBuffer << endl;

		// parsing client request
		char *no_http = strstr(recvBuffer, "http://") + 7;
		int temp = strlen(strstr(no_http, "/"));
		char *recp_addr = (char *)malloc(strlen(no_http) - strlen(strstr(no_http, "/")));
		memcpy(recp_addr, no_http, strlen(no_http) - strlen(strstr(no_http, "/")));
		
		cout << recp_addr << endl;
		// connect to recepient

	}
}
