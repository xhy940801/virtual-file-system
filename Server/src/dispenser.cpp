#include "Dispenser.h"

#include "FileGuard.h"
#include <iostream>

#define LIS_PORT 18542

static bool getCmdYOrN(const char * tips)
{
	cout << tips << endl;
	cout << "y(yes) or n(no)";

	char cmd;
	cin >> cmd;
	while(cmd != 'y' && cmd != 'Y' && cmd != 'n' && cmd != 'N')
	{
		cout << "Unknow command!" << endl;
		cout << tips << endl;
		cout << "y(yes) or n(no)";

		cin >> cmd;
	}

	if(cmd == 'y' || cmd == 'Y')
		return true;
	return false;
}

bool Session::writeFBuf(XHYFileManager* manager, int fd, const char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = manager->write(fd, buf + pos, len - pos)
		if(rs < 0)
		{
			setErr(ERR_SYS_ERROR);
			return false;
		}
		if(rs == 0)
		{
			setErr(ERR_SYS_ERROR);
			return false;
		}
		pos += rs;
	}
	return true;
}

static bool resetOS(XHYFileManager* manager)
{
	class EmptyAuth
	{
		int operator () (SafeInfo* info) { return 0; }
	};

	std::lock_guard<XHYFileManager> lck(*manager);
	manager->initFS();
	manager->createFloder("/", "sys", 0);
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
	int id = 1;
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
	Session session(manager, socket);
	session.run();
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

	sockaddr_in serveraddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(LIS_PORT);

	//绑定端口
	if(bind(socket_fd, (sockaddr*) &servaddr, sizeof(servaddr)) == -1)
	{
		std::cout << "Bind socket error: " << strerror(errno) << "(errno: " << errno << ")" << std::endl
		exit(0);  
	}

	if(listen(socket_fd, 10) == -1)
	{  
		std::cout << "Listen port error: " << strerror(errno) << "(errno: " << errno << ")" << std::endl
		exit(0);  
	}

	while(true)
	{
		sockaddr_in clientaddr;
		int csocket = accept(ssocket, (sockaddr*) &clientaddr, sizeof(clientaddr));
		if(csocket != -1)
		{
			thread th(runNew, manager, csocket);
			th.detach();
		}
	}
}