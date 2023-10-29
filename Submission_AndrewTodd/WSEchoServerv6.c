// CS 2690 Program 2 
// Simple Windows Sockets Echo Server (IPv6)
// Last update: 2/12/19
//
// Usage: WSEchoServerv6 <server port>
// Companion client is WSEchoClient.exe
// Server usage: WSEchoClient.exe <server address> <server port> <message to echo>
//
// This program is coded in conventional C programming style, with the 
// exception of the C++ style comments.
//
// I declare that the following source code was written by me or provided
// by the instructor. I understand that copying source code from any other 
// source or posting solutions to programming assignments (code) on public
// Internet sites constitutes cheating, and that I will receive a zero 
// on this project if I violate this policy.
// ----------------------------------------------------------------------------

#pragma comment(lib, "ws2_32.lib")

// Minimum required header files for C Winsock program
#include <stdio.h>       // for print functions
#include <stdlib.h>      // for exit() 
#include <winsock2.h>	 // for Winsock2 functions
#include <ws2tcpip.h>    // adds support for getaddrinfo & getnameinfo for v4+6 name resolution
#include <Ws2ipdef.h>    // optional - needed for MS IP Helper
#include <conio.h>
#include <process.h>     // for _beginthreadex to make ProcessClient async
#include <signal.h>

// #define ALL required constants HERE, not inline 
// #define is a macro, don't terminate with ';'  For example...

int shouldExit = 0;

// declare any functions located in other .c files here
void DisplayFatalErr(char *errMsg); // writes error message before abnormal termination

void ProcessClient(void* clientSocketPtr); //processes the message receaved from the client. Echoes it back

void InterruptSignalHandler(int signal)
{
	if (signal == SIGINT)
	{
		shouldExit = 1; // Set exit flag so the while loop will exit and server will shut down
	}
}

void main(int argc, char *argv[])   // argc is # of strings following command, argv[] is array of ptrs to the strings
{
	signal(SIGINT, InterruptSignalHandler);

	// Declare ALL variables and structures for main() HERE, NOT INLINE (including the following...)
	WSADATA wsaData;                // contains details about WinSock DLL implementation
	//struct sockaddr_in6 serverInfo;	// standard IPv6 structure that holds server socket info

	struct addrinfo hints, *result = NULL, *ptrAddrInfo = NULL;
	SOCKET listenSocket = INVALID_SOCKET, clientSock = INVALID_SOCKET;
	
	// Verify correct number of command line arguments, else do the following:
	if (argc > 2)
	{
		fprintf(stderr, "Too many arguent provided\nExpected: <server port>\n");
		exit(1);	  // ...and terminate with abnormal termination code (1)
	}
	
	const char* port = (argc == 2) ? argv[1] : "5555";

	// Initialize Winsock 2.0 DLL. Returns 0 if ok. If this fails, fprint error message to stderr as above & exit(1).
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DisplayFatalErr("WSAStartup failed");
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &hints, &result) != 0)
	{
		DisplayFatalErr("Failed to generate IP for server");
	}

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (listenSocket == INVALID_SOCKET)
	{
		DisplayFatalErr("Failed to create server's socket");
	}

	u_long mode = 1;
	if (ioctlsocket(listenSocket, FIONBIO, &mode) != 0)
	{
		DisplayFatalErr("Failed to configure listening socket to be non-blocking");
	}

	//set the socket to listen for tcp messages on port
	if (bind(listenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
	{
		DisplayFatalErr("Failed to bind socket  to listen for tcp messages on port: %s", port);
	}

	freeaddrinfo(result);

	//check that we can listen on open socket
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		DisplayFatalErr("Cannot listen on socket");
	}

	printf("Server is ready for client connection to port: %s...\nPress ctrl + d to shut down\n", port);

	fd_set readfds;
	struct timeval timeout;
	int selectResult;

	//wait for and accept client connections
	while (!shouldExit)
	{
		//check for user input on ther server
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 4) // 4 corresponds to Ctrl + D
			{
				shouldExit = 1; //exit the loop and shut down the server
			}
			else
			{
				continue;
			}
		}

		//clear the socket set
		FD_ZERO(&readfds);

		//Add the server socket to the set
		FD_SET(listenSocket, &readfds);

		//Set timeout interval
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;

		selectResult = select(0, &readfds, NULL, NULL, &timeout);
		if (selectResult == SOCKET_ERROR)
		{
			DisplayFatalErr("Error when checking activity on server socket");
		}
		else if (selectResult > 0)
		{
			if (FD_ISSET(listenSocket, &readfds))
			{
				clientSock = accept(listenSocket, NULL, NULL);
				if (clientSock != INVALID_SOCKET)
				{
					struct sockaddr_storage addr;
					int addrLen = sizeof(addr);
					getpeername(clientSock, (struct sockaddr*)&addr, &addrLen);

					char ipString[INET6_ADDRSTRLEN];
					int clientPort;
					if (addr.ss_family == AF_INET)
					{
						struct sockaddr_in* s = (struct sockaddr_in*)&addr;
						inet_ntop(AF_INET, &s->sin_addr, ipString, INET6_ADDRSTRLEN);
						clientPort = ntohs(s->sin_port);
					}
					//AF_INET6
					else
					{
						struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
						inet_ntop(AF_INET6, &s->sin6_addr, ipString, INET6_ADDRSTRLEN);
						clientPort = ntohs(s->sin6_port);
					}

					printf("Processing the client at %s, client port %d, server port %s\n", ipString, clientPort, port);

					_beginthreadex(NULL, 0, ProcessClient, (void*)&clientSock, 0, NULL);
				}
				else
				{
					printf("Could not accept client connection...\n");
					closesocket(clientSock);
					continue;
				}
			}
		}
	}

	printf("Server shutting down...");

	closesocket(listenSocket);

	// Release the Winsock DLL
	WSACleanup();
	
	exit(0);
}
