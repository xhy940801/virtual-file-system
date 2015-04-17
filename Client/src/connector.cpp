#include "connector.h"

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#define LIS_PORT 18542

SessionClient* initSession(const char* rootaddr)
{
	int cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == cfd)
	{
		std::cout << "socket fail ! " << std::endl;
		return nullptr;
	}

	sockaddr_in s_add;
	memset(&s_add, 0, sizeof(s_add));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr= inet_addr(rootaddr);
	s_add.sin_port=htons(LIS_PORT);

	if(-1 == connect(cfd,(sockaddr *)(&s_add), sizeof(sockaddr)))
	{
		std::cout << "connect fail !" << std::endl;
		return nullptr;
	}
	return new SessionClient(cfd);
}

void releaseSession(SessionClient* session)
{
	if(session)
		delete session;
}