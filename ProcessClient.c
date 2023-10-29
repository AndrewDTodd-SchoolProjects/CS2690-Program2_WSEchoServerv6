#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>

#define RCVBUFSIZ 1024

void ProcessClient(void* clientSocketPtr)
{
	SOCKET clientSocket = *(SOCKET*)clientSocketPtr;

	char rcvBuffer[RCVBUFSIZ];
	int bytesReceived = 0;

	do
	{
		bytesReceived = recv(clientSocket, rcvBuffer, RCVBUFSIZ, 0);
		if (bytesReceived > 0)
		{
			send(clientSocket, rcvBuffer, bytesReceived, 0);
		}
		else if (bytesReceived == 0)
		{
			printf("Connection closing...\n");
		}
		else
		{
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
			{
				printf("recv failed with error: %d\n", error);
			}
			else
			{
				printf("Connection closing...\n");
			}
		}
	} while (bytesReceived > 0);

	closesocket(clientSocket);
	_endthreadex(0);
}
