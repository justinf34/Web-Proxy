#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <cstring>
#include <signal.h>

using namespace std;

#define PORT 8080
#define RQST_SIZE 2028
#define HTML_SIZE 10 * RQST_SIZE
#define MAX_PENDING 1

int proxySock;
int clientSock;

//Function declarations
void initSock(int &sock, int portnum = PORT, char *addr = NULL, int mode = 0);

void cleanExit(int sig)
{
	close(proxySock);
	close(clientSock);
	exit(0);
}

int main(int argc, char const *argv[])
{
	char client_rqstBuffer[RQST_SIZE], server_rqstBuffer[RQST_SIZE], reply[HTML_SIZE], svr_resBuffer[HTML_SIZE];
	char url[RQST_SIZE], hostname[RQST_SIZE], file_path[RQST_SIZE];

	int bytesSent;
	int bytesRecv;

	signal(SIGTERM, cleanExit);
	signal(SIGINT, cleanExit);

	//---------- Creating a Listener Socket ----------//
	initSock(proxySock); // Creates a listener socket and is listening if no error

	while (true)
	{
		cout << "Accepting connections..." << endl;
		if ((clientSock = accept(proxySock, NULL, NULL)) < 0)
		{
			cout << "Could not connect to client" << endl;
			exit(-1);
		}

		memset(&client_rqstBuffer, 0, RQST_SIZE); // clearing the buffer
		memset(&server_rqstBuffer, 0, RQST_SIZE);

		int totalMsg = 0;
		while ((bytesRecv = recv(clientSock, (char *)&client_rqstBuffer, RQST_SIZE, 0)) > 0)
		{
			if (bytesRecv < 1)
			{
				cout << "Did not receive any message" << endl;
				exit(1);
			}

			//----------- Parsing Client Request -----------//

			char tmp[RQST_SIZE];
			strcpy(tmp, client_rqstBuffer);

			cout << "Client request:" << endl
				 << client_rqstBuffer << endl;

			char *addr = strtok(strstr(tmp, "Host: ") + 6, "\r\n");

			//---------- Sending/Receiving from Server ----------//

			cout << "Creating a client socket to connect to " << addr << endl;
			int sendSock;
			initSock(sendSock, 80, addr, 1); // Creating a sender socket

			strcpy(server_rqstBuffer, client_rqstBuffer);
			bytesSent = send(sendSock, server_rqstBuffer, HTML_SIZE, 0);
			if (bytesSent < 0)
			{
				cout << "Could not pass on request to host" << endl;
				exit(1);
			}

			memset(&svr_resBuffer, 0, HTML_SIZE);
			while ((bytesRecv = recv(sendSock, svr_resBuffer, HTML_SIZE, 0)) > 0)
			{
				if (bytesRecv < 1)
				{
					cout << "Did not get a response from the server" << endl;
					exit(0);
				}

				cout << addr << " responded with:" << endl
					 << svr_resBuffer << endl;

				//---------- Editing the response ----------//
				//	Get the byte size of the message body
				//	bytesRecv - 390, should point back the start of the message body
				char editRes[bytesRecv];
				strcpy(editRes, svr_resBuffer);

				char *msgBody = strstr(editRes, "\r\n\r\n") + 4;
				cout << "The message body is:" << endl
					 << msgBody << endl;

				// Floppy -> Trolly
				char *ptr;
				char trolly[7] = "Trolly";
				while (ptr = strstr(msgBody, "Floppy"))
				{
					int i = 0;
					for (i = 0; i < 6; i++)
					{
						*(ptr + i) = *(trolly + i);
					}
				}

				// Italy -> Japan
				char japan[6] = "Japan";
				while (ptr = strstr(msgBody, "Italy"))
				{
					int i = 0;
					for (i = 0; i < 5; i++)
					{
						*(ptr + i) = *(japan + i);
					}
				}

				//---------- Sending the message back to client ----------//
				bytesSent = send(clientSock, editRes, HTML_SIZE, 0);

				if (bytesSent < 1)
				{
					cout << "Couldn't send message to client" << endl;
					close(clientSock);
					exit(0);
				}
				memset(&svr_resBuffer, 0, HTML_SIZE);
			}

			memset(&client_rqstBuffer, 0, RQST_SIZE);
			memset(&server_rqstBuffer, 0, RQST_SIZE);
		}

		close(clientSock);
	}
}

/*
*
*
*/
void initSock(int &sock, int portnum, char *addr, int mode)
{
	struct sockaddr_in sockAddr;
	int option;

	// Creating a TCP socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cout << "Error creating socket" << endl;
		exit(-1);
	}

	// Freeing up the port before binding
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0)
	{
		cout << "Failed to free up the port" << endl;
		exit(-1);
	}

	//Initialize server info
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(portnum); // Initialize port

	if (mode && addr) // When we are building a client socket
	{
		struct hostent *server;
		server = gethostbyname(addr);
		bcopy((char *)server->h_addr, (char *)&sockAddr.sin_addr.s_addr, server->h_length); // Set the id address to the host we are connecting to
	}
	else // When creating a server/listener socket
	{
		sockAddr.sin_addr.s_addr = INADDR_ANY; // Initialize IP of webProxy to any address
	}

	if (mode) // Connect to server; Mode: client
	{
		cout << "Connecting to " << addr << "..." << endl;
		if (connect(sock, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) < 0)
		{
			cout << "Cannot connect to " << addr << endl;
			exit(-1);
		}

		cout << "Connected to " << addr << "..." << endl;
	}
	else
	{
		cout << "Binding the listener socket..." << endl;
		// Bind socket
		if (bind(sock, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) < 0)
		{
			cout << "Binding failed" << endl;
			exit(-1);
		}

		cout << "Listening for connections..." << endl;
		// Listening for connection
		if (listen(sock, MAX_PENDING) < 0)
		{
			cout << "listen() failed" << endl;
			exit(-1);
		}
	}
}
