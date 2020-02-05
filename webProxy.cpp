#include <iostream>

// Networking related libraries
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <cstring>
#include <string.h>

#include <signal.h>

using namespace std;

#define PORT 8080
#define RQST_SIZE 2048
#define HTML_SIZE 10 * RQST_SIZE
#define MAX_PENDING 1

int proxySock;
int clientSock;

char troll_rqst[] = "GET http://pages.cpsc.ucalgary.ca/~carey/CPSC441/trollface.jpg HTTP/1.1\r\nHost: pages.cpsc.uclagary.ca\r\n\r\n";

//Function declarations
void initSock(int &sock, int portnum = PORT, char *addr = NULL, int mode = 0);
char *replaceWord(const char *s, const char *oldW, const char *newW);

void cleanExit(int sig)
{
	close(proxySock);
	close(clientSock);
	exit(0);
}

int main(int argc, char const *argv[])
{
	char client_rqst[RQST_SIZE], proxy_rqst[RQST_SIZE], proxy_res[HTML_SIZE], server_res[HTML_SIZE];
	char url[RQST_SIZE], hostname[RQST_SIZE], file_path[RQST_SIZE];

	int bytesSent;
	int bytesRecv;

	signal(SIGTERM, cleanExit);
	signal(SIGINT, cleanExit);

	//---------- Creating a Listener Socket ----------//
	initSock(proxySock); // Creates a listener socket and is listening if no error

	while (1)
	{
		cout << "Accepting connections..." << endl;
		if ((clientSock = accept(proxySock, NULL, NULL)) < 0)
		{
			cout << "Could not connect to incoming client" << endl;
			exit(-1);
		}

		memset(&client_rqst, 0, RQST_SIZE);
		memset(&proxy_rqst, 0, RQST_SIZE);

		while ((bytesRecv = recv(clientSock, (char *)&client_rqst, RQST_SIZE, 0)) > 0)
		{

			//----------- Checking the for Floppy Images -----------//
			char *pch = strstr(client_rqst, "\r");
			int num = client_rqst - pch;
			char firstLine[(-1 * num) + 1];
			memcpy(firstLine, client_rqst, -1 * num);

			char *ptr = NULL;
			if (ptr = strstr(firstLine, "Floppy"))
			{
				strcpy(proxy_rqst, troll_rqst);
			}
			else
			{
				strcpy(proxy_rqst, client_rqst);
			}

			//----------- Finding the host -----------//
			cout << "Client request:" << endl
				 << client_rqst << endl;

			char *addr = strtok(strstr(client_rqst, "Host: ") + 6, "\r\n");

			//---------- Sending/Receiving from Server ----------//

			cout << "Creating a client socket to connect to " << addr << endl;

			int sendSock;
			initSock(sendSock, 80, addr, 1); // Creating a sender socket

			bytesSent = send(sendSock, proxy_rqst, HTML_SIZE, 0);
			if (bytesSent < 0)
			{
				cout << "Could not pass on request to host" << endl;
				exit(1);
			}

			memset(&server_res, 0, HTML_SIZE);
			while ((bytesRecv = recv(sendSock, server_res, HTML_SIZE, 0)) > 0)
			{
				cout << addr << " responded with:" << endl
					 << server_res << endl;

				//---------- Editing the response ----------//
				char *ptr;
				char trolly[7] = "Trolly";
				while (ptr = strstr(server_res, " Floppy"))
				{
					ptr = ptr + 1;
					int i = 0;
					for (i = 0; i < 6; i++)
					{
						*(ptr + i) = *(trolly + i);
					}
				}

				// Italy -> Japan
				char japan[6] = "Japan";
				while (ptr = strstr(server_res, " Italy"))
				{
					ptr = ptr + 1;
					int i = 0;
					for (i = 0; i < 5; i++)
					{
						*(ptr + i) = *(japan + i);
					}
				}

				//---------- Sending the message back to client ----------//
				bcopy(server_res, proxy_res, bytesRecv);
				bytesSent = send(clientSock, proxy_res, bytesRecv, 0);

				if (bytesSent < 0)
				{
					cout << "Couldn't send message to client" << endl;
					close(clientSock);
					exit(-1);
				}
				memset(&server_res, 0, HTML_SIZE);
				memset(&client_rqst, 0, RQST_SIZE);
				memset(&proxy_rqst, 0, RQST_SIZE);
			}
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

char *replaceWord(const char *s, const char *oldW,
				  const char *newW)
{
	char *result;
	int i, cnt = 0;
	int newWlen = strlen(newW);
	int oldWlen = strlen(oldW);

	// Counting the number of times old word
	// occur in the string
	for (i = 0; s[i] != '\0'; i++)
	{
		if (strstr(&s[i], oldW) == &s[i])
		{
			cnt++;

			// Jumping to index after the old word.
			i += oldWlen - 1;
		}
	}

	// Making new string of enough length
	result = (char *)malloc(i + cnt * (newWlen - oldWlen) + 1);

	i = 0;
	while (*s)
	{
		// compare the substring with the result
		if (strstr(s, oldW) == s)
		{
			strcpy(&result[i], newW);
			i += newWlen;
			s += oldWlen;
		}
		else
			result[i++] = *s++;
	}

	result[i] = '\0';
	return result;
}
