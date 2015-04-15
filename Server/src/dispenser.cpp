#include "dispenser.h"

#include "FileGuard.h"
#include "SessionServer.h"
#include "sdef.h"
#include "types.h"
#include "FMGuard.h"
#include "GlobalVal.h"

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#define LIS_PORT 18542

static bool getCmdYOrN(const char * tips)
{
	std::cout << tips << std::endl;
	std::cout << "y(yes) or n(no)";

	char cmd;
	std::cin >> cmd;
	while(cmd != 'y' && cmd != 'Y' && cmd != 'n' && cmd != 'N')
	{
		std::cout << "Unknow command!" << std::endl;
		std::cout << tips << std::endl;
		std::cout << "y(yes) or n(no)";

		std::cin >> cmd;
	}

	if(cmd == 'y' || cmd == 'Y')
		return true;
	return false;
}

bool writeFBuf(XHYFileManager* manager, int fd, const char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = manager->write(fd, buf + pos, len - pos);
		if(rs < 0)
			return false;
		if(rs == 0)
			return false;
		pos += rs;
	}
	return true;
}

static bool resetOS(XHYFileManager* manager)
{
	class EmptyAuth
	{
	public:
		int operator () (SafeInfo* info, fdtype_t type = 0) { return 0; }
	};

	std::lock_guard<XHYFileManager> lck(*manager);
	manager->initFS();
	manager->createFolder("/", "sys", 0);
	SafeInfo sfInfo;
	manager->getItemSafeInfo("/sys", &sfInfo);
	sfInfo.sign = PM_U_READ | PM_U_WRITE | PM_O_READ;
	manager->changeSafeInfo("/sys", &sfInfo, EmptyAuth());
	manager->createFile("/sys", "user.d", 0);
	int fd = manager->open("/sys/user.d", OPTYPE_WRITE, EmptyAuth());
	if(fd == 0)
		return false;
	FileGuard<XHYFileManager, int> fg(*manager, fd);
	char uname[_MAXUNAMESIZE];
	char psd[_MAXPSDSIZE];
	memset(uname, 0, _MAXUNAMESIZE);
	memset(psd, 0, _MAXPSDSIZE);
	strcpy(uname, "root");
	strcpy(psd, "root");
	uidsize_t id = 1;
	if(!writeFBuf(manager, fd, (char*) &id, sizeof(id)))
		return false;
	if(!writeFBuf(manager, fd, uname, _MAXUNAMESIZE))
		return false;
	if(!writeFBuf(manager, fd, psd, _MAXPSDSIZE))
		return false;
	return true;
}

static void runNew(XHYFileManager* manager, int socket)
{
	FMGuard fmg(manager);
	SessionServer session(manager, socket);
	session.run();
	close(socket);
}

void startServer(XHYFileManager* manager)
{
	//判断是否需要初始化系统
	if(getCmdYOrN("Are you want to format the disk and reset the system?"))
		resetOS(manager);
	int ssocket = socket(AF_INET, SOCK_STREAM, 0);
	if(ssocket == -1)
	{
		std::cout << "Create server socket error!";
		exit(0);
	}

	sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(LIS_PORT);

	//绑定端口
	if(bind(ssocket, (sockaddr*) &servaddr, sizeof(servaddr)) == -1)
	{
		std::cout << "Bind socket error: " << strerror(errno) << "(errno: " << errno << ")" << std::endl;
		exit(0);  
	}

	if(listen(ssocket, 10) == -1)
	{  
		std::cout << "Listen port error: " << strerror(errno) << "(errno: " << errno << ")" << std::endl;
		exit(0);  
	}

	std::cout << "Start listen! port: " << LIS_PORT << std::endl;
	while(true)
	{
		sockaddr_in clientaddr;
		socklen_t socklen = sizeof(clientaddr);
		int csocket = accept(ssocket, (sockaddr*) &clientaddr, &socklen);
		if(csocket != -1)
		{
			std::thread th(runNew, manager, csocket);
			th.detach();
		}
	}
}