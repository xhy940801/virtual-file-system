#include "Session.h"

#include "sdef.h"
#include "serrordef.h"

#include <sys/types.h>
#include <cstdio>
#include <memory>

Session::Session(XHYFileManager* mgr, int sock)
	: manager(mgr), socket(sock), userId(0)
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
	uidsize_t uid = validAuth(username, password);
	if(uid)
	{
		char rrs = 1;
		status = Session::LOOPING;
		userId = uid;
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

int Session::pathValidateR(char* path, bool type, SafeInfo* sfInfo)
{
	SafeInfo sfInfo;
	if(!manager->getItemSafeInfo(path.get(), sfInfo))
		return manager->getErrno();
	else
	{
		if(((bool) sfInfo.sign & FL_TYPE_FILE) == type)
			return 2
		else if(!((sfInfo.uid == userId && sfInfo.sign & PM_U_READ) || (sfInfo.uid != userId && sfInfo.sign & PM_O_READ)))
			return 3;
		return 0;
	}
}

int Session::pathValidateW(char* path, bool type, SafeInfo* sfInfo)
{
	
	if(!manager->getItemSafeInfo(path.get(), sfInfo))
		return manager->getErrno();
	else
	{
		if(((bool) sfInfo.sign & FL_TYPE_FILE) == type)
			return 2
		else if(!((sfInfo.uid == userId && sfInfo.sign & PM_U_WRITE) || (sfInfo.uid != userId && sfInfo.sign & PM_O_WRITE)))
			return 3;
		return 0;
	}
}

void Session::createFile()
{
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	std::auto_ptr<char> path = new char[pathLen + 2];
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	unsigned char nameLen;
	if(!readSBuf(char*) &nameLen, sizeof(nameLen))
		return;
	std::auto_ptr<char> name = new char[nameLen + 1];
	if(!readSBuf(name.get(), nameLen))
		return;
	name.get()[nameLen] = '\0';

	int rss = 0;
	std::lock_guard<XHYFileManager> lck(*manager);
	SafeInfo sfInfo;
	rss = checkPathAuthValidW(path.get(), 0, &sfInfo);
	if(rss == 0)
	{
		if(!manager->createFile(path.get(), name.get(), userId))
			rss = manager->getErrno();
	}

	writeSBuf(&rss, sizeof(rss));
}

void Session::createFloder()
{
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	std::auto_ptr<char> path = new char[pathLen + 1];
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	unsigned char nameLen;
	if(!readSBuf(char*) &nameLen, sizeof(nameLen))
		return;
	std::auto_ptr<char> name = new char[nameLen + 1];
	if(!readSBuf(name.get(), nameLen))
		return;
	name.get()[nameLen] = '\0';

	int rss = 0;
	std::lock_guard<XHYFileManager> lck(*manager);
	SafeInfo sfInfo;
	rss = checkPathAuthValidW(path.get(), 0, &sfInfo);
	if(rss == 0)
	{
		if(!manager->createFloder(path.get(), name.get(), userId))
			rss = manager->getErrno();
	}

	writeSBuf(&rss, sizeof(rss));
}

void Session::readFloder()
{
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	std::auto_ptr<char> path = new char[pathLen + 1];
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	SafeInfo sfInfo;
	int rss = pathValidateR(path.get(), 0, &sfInfo);
	if(rss == 0)
	{
		std::auto_ptr<FileNode> fnodes = new FileNode[20];
		if(!writeSBuf(&rss, sizeof(rss)))
			return;
		int len, start = 0;
		while(true)
		{
			len = manager->readFloder(path, start, 20, fnodes.get());
			if(len == 0)
			{
				char tmp = 0;
				writeSBuf(&tmp, sizeof(tmp));
			}
			else if(len < 0)
				setErr(ERR_SYS_ERROR);
			else
			{
				for(int i = 0; i < len; ++i)
				{
					char nlen = strlen(fnodes.get()[i].name);
					writeSBuf(&nlen, sizeof(nlen));
					writeSBuf(fnodes.get()[i].name, nlen);
				}
				continue;
			}
			break;
		}
	}
	else
		writeSBuf(&rss, sizeof(rss));
}

void Session::changeModel()
{
	fsign sign;
	std::auto_ptr<char> path = new char[pathLen + 1];
	if(!readSBuf(path.get(), pathLen))
		return;
	if(!readSBuf((char*) sign, sizeof(sign)))
		return;
	int rss = pathValidateR(path.get(), 0, &sfInfo);
}

void Session::openFile()
{

}

void Session::readFile()
{

}

void Session::writeFile()
{

}

void Session::seekFile()
{

}

void Session::tellFile()
{

}

void Session::deleteItem()
{

}

void Session::getItemSafeInfo()
{

}

void Session::formatDisk()
{

}

void Session::logout()
{

}
