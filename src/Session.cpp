#include "Session.h"

#include "sdef.h"
#include "serrordef.h"

#include <sys/types.h>
#include <cstdio>

Session::Session(XHYFileManager* mgr, int sock)
	: manager(mgr), socket(sock)
{
	printConnectionInfo();
}

Session::~Session()
{
	close(socket);
}

void Session::run()
{
	while(true)
	{
		switch(status)
		{
		case Session::NLOGIN:
			rlogin();
			break;
		case Session::LOOPING:
			rloop();
			break;
		case Session::EXIT:
			printClosedInfo();
			return;
		case Session::ERROR:
			printErrorInfo();
			return;
		}
	}
}

void Session::rlogin()
{
	if(!validHeader())
		return;
	char username[_MAXUNAMESIZE];
	char password[_MAXPSDSIZE];
	size_t pos = 0;
	if(!readSBuf(username, _MAXUNAMESIZE))
		return;
	if(!readSBuf(password, _MAXPSDSIZE))
		return;
	char rrs;
	if(validAuth(username, password))
	{
		char rrs = 1;
		status = Session::LOOPING;
	}
	else
		char rrs = 0;
	write(socket, &rrs, sizeof(rrs));
}

void Session::rloop()
{
	if(!validHeader())
		return;
	char cmd[_CMD_SIZE + 1];
	cmd[_CMD_SIZE] = '\0';
	if(!readSBuf(cmd, _CMD_SIZE))
		return;
	if(strcmp(cmd, "cdr") == 0)
		createFloder();
	else if(strcmp(cmd, "cfl") == 0)
		createFloder();
	else if(strcmp(cmd, "rdr") == 0)
		readFloder();
	else if(strcmp(cmd, "cmd") == 0)
		changeModel();
	else if(strcmp(cmd, "ofl") == 0)
		openFile();
	else if(strcmp(cmd, "rfl") == 0)
		readFile();
	else if(strcmp(cmd, "wfl") == 0)
		writeFile();
	else if(strcmp(cmd, "del") == 0)
		deleteItem();
	else if(strcmp(cmd, "gsf") == 0)
		getItemSafeInfo();
	else if(strcmp(cmd, "fdk") == 0)
		formatDisk(); 
	else if(strcmp(cmd, "lgt") == 0)
		logout();
	else
		cmdError();
}

bool Session::validHeader()
{
	char tmp;
	int rs = read(socket, &tmp, 1);
	if(rs < 0)
	{
		setErr(ERR_SOCKET_EXCEPTION);
		return false;
	}
	if(rs == 0)
	{
		setExit();
		return false;
	}
	if(tmp != 'X')
	{
		setErr(ERR_VALIDATE_HEAD);
		return false;
	}
	return true;
}

bool Session::readSBuf(char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = read(socket, buf + pos, len - pos)
		if(rs < 0)
		{
			setErr(ERR_SOCKET_EXCEPTION);
			return false;
		}
		if(rs == 0)
		{
			setExit();
			return false;
		}
		pos += rs;
	}
	return true;
}

bool Session::writeSBuf(const char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = write(socket, buf + pos, len - pos)
		if(rs < 0)
		{
			setErr(ERR_SOCKET_EXCEPTION);
			return false;
		}
		if(rs == 0)
		{
			setExit();
			return false;
		}
		pos += rs;
	}
	return true;
}

bool Session::readFBuf(int fd, char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = manager->read(fd, buf + pos, len - pos)
		if(rs < 0)
		{
			setErr(ERR_SYS_ERROR);
			return false;
		}
		if(rs == 0)
			return false;
		pos += rs;
	}
	return true;
}

bool Session::writeFBuf(int fd, const char* buf, size_t len)
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


void Session::printClosedInfo()
{
	struct sockaddr_in sa;
	char tmp[40];
	if(!getpeername(sockfd, (struct sockaddr *)&sa, &len))
	{
		if(inet_ntop(AF_INET, &sa.sin_addr, tmp, sizeof(tmp)) != nullptr)
			printf("Connection closed! ip: %s port: %s", tmp, sa.sin_port);
		else
			printf("Connection closed!");
	}
}

void Session::printErrorInfo()
{
	struct sockaddr_in sa;
	char tmp[40];
	if(!getpeername(sockfd, (struct sockaddr *)&sa, &len))
	{
		if(inet_ntop(AF_INET, &sa.sin_addr, tmp, sizeof(tmp)) != nullptr)
			fprintf(stderr, "Connection error, code: %s! ip: %s port: %s", err, tmp, sa.sin_port);
		else
			fprintf(stderr, "Connection error, code: %s!", err);
	}
}

void Session::printConnectionInfo()
{
	struct sockaddr_in sa;
	char tmp[40];
	if(!getpeername(sockfd, (struct sockaddr *)&sa, &len))
	{
		if(inet_ntop(AF_INET, &sa.sin_addr, tmp, sizeof(tmp)) != nullptr)
			printf("Accept! ip: %s port: %s", tmp, sa.sin_port);
		else
			printf("Accept!");
	}
}

inline void Session::setErr(int error)
{
	err = error;
	status = Session::ERROR;
}

inline void Session::setExit()
{
	status = Session::EXIT;
}

uidsize_t Session::validAuth(char* username, char* password)
{
	int fd = manager->open("/sys/user.d");
	if(fd == 0)
	{
		setErr(ERR_SYS_ERROR);
		return 0;
	}
	char buf[_MAXUNAMESIZE > _MAXPSDSIZE ? _MAXUNAMESIZE : _MAXPSDSIZE];
	while(true)
	{
		uidsize_t uid;
		if(!readFBuf(fd, &uid, sizeof(uid)))
			return 0;
		if(!readFBuf(fd, buf, _MAXUNAMESIZE))
			return 0;
		if(strcmp(buf, username) != 0)
		{
			manager->seek(fd, _MAXPSDSIZE, XHYFileManager::cur);
			continue;
		}
		if(!readFBuf(fd, buf, _MAXPSDSIZE))
			return 0;
		if(strcmp(buf, username) == 0)
			return uid;
	}
}

void Session::createFile()
{
	
}