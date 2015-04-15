#include "SessionServer.h"

#include "sdef.h"
#include "serrordef.h"

#include "FileGuard.h"

#include <memory>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


SessionServer::SessionServer(XHYFileManager* mgr, int sock)
	: manager(mgr), socket(sock), userId(0)
{
	printConnectionInfo();
}

SessionServer::~SessionServer()
{
}

void SessionServer::run()
{
	while(true)
	{
		switch(status)
		{
		case SessionServer::NLOGIN:
			rlogin();
			break;
		case SessionServer::LOOPING:
			rloop();
			break;
		case SessionServer::EXIT:
			printClosedInfo();
			return;
		case SessionServer::ERROR:
			printErrorInfo();
			return;
		}
	}
}

void SessionServer::rlogin()
{
	if(!validHeader())
		return;
	char username[_MAXUNAMESIZE];
	char password[_MAXPSDSIZE];
	if(!readSBuf(username, _MAXUNAMESIZE))
		return;
	if(!readSBuf(password, _MAXPSDSIZE))
		return;
	int rrs;
	uidsize_t uid = validAuth(username, password);
	if(uid)
	{
		rrs = 0;
		status = SessionServer::LOOPING;
		userId = uid;
	}
	else
		rrs = err;
	write(socket, &rrs, sizeof(rrs));
}

void SessionServer::rloop()
{
	if(!validHeader())
		return;
	char cmd[_CMD_SIZE + 1];
	cmd[_CMD_SIZE] = '\0';
	if(!readSBuf(cmd, _CMD_SIZE))
		return;
	if(strcmp(cmd, "cdr") == 0)
		createFolder();
	else if(strcmp(cmd, "cfl") == 0)
		createFile();
	else if(strcmp(cmd, "rdr") == 0)
		readFolder();
	else if(strcmp(cmd, "cmd") == 0)
		changeModel();
	else if(strcmp(cmd, "ofl") == 0)
		openFile();
	else if(strcmp(cmd, "rfl") == 0)
		readFile();
	else if(strcmp(cmd, "wfl") == 0)
		writeFile();
	else if(strcmp(cmd, "skf") == 0)
		seekFile();
	else if(strcmp(cmd, "tlf") == 0)
		tellFile();
	else if(strcmp(cmd, "csf") == 0)
		closeFile();
	else if(strcmp(cmd, "del") == 0)
		deleteItem();
	else if(strcmp(cmd, "gsf") == 0)
		getItemSafeInfo();
	// else if(strcmp(cmd, "fdk") == 0)
	// 	formatDisk(); 
	else if(strcmp(cmd, "lgt") == 0)
		logout();
	else if(strcmp(cmd, "aur") == 0)
		addUsr();
	else if(strcmp(cmd, "cpd") == 0)
		changePsd();
	else
		cmdError();
}

bool SessionServer::validHeader()
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

bool SessionServer::readSBuf(char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = read(socket, buf + pos, len - pos);
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

bool SessionServer::writeSBuf(const char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = write(socket, buf + pos, len - pos);
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

bool SessionServer::readFBuf(int fd, char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = manager->read(fd, buf + pos, len - pos);
		if(rs < 0)
		{
			setErr(ERR_SYS_ERROR);
			return false;
		}
		if(rs == 0)
		{
			setErr(ERR_FEAD_FILE_EOF);
			return false;
		}
		pos += rs;
	}
	return true;
}

bool SessionServer::writeFBuf(int fd, const char* buf, size_t len)
{
	size_t pos = 0;
	while(pos != len)
	{
		int rs = manager->write(fd, buf + pos, len - pos);
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


void SessionServer::printClosedInfo()
{
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	char tmp[40];
	if(!getpeername(socket, (sockaddr *)&sa, &len))
	{
		if(inet_ntop(AF_INET, &sa.sin_addr, tmp, sizeof(tmp)) != nullptr)
			printf("Connection closed! ip: %s port: %d\n", tmp, sa.sin_port);
		else
			printf("Connection closed!\n");
	}
}

void SessionServer::printErrorInfo()
{
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	char tmp[40];
	if(!getpeername(socket, (struct sockaddr *)&sa, &len))
	{
		if(inet_ntop(AF_INET, &sa.sin_addr, tmp, sizeof(tmp)) != nullptr)
			fprintf(stderr, "Connection error, code: %d! ip: %s port: %d\n", err, tmp, sa.sin_port);
		else
			fprintf(stderr, "Connection error, code: %d!\n", err);
	}
}

void SessionServer::printConnectionInfo()
{
	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	char tmp[40];
	if(!getpeername(socket, (struct sockaddr *)&sa, &len))
	{
		if(inet_ntop(AF_INET, &sa.sin_addr, tmp, sizeof(tmp)) != nullptr)
			printf("Accept! ip: %s port: %d\n", tmp, sa.sin_port);
		else
			printf("Accept!\n");
	}
}

inline void SessionServer::setErr(int error)
{
	err = error;
	status = SessionServer::ERROR;
}

inline void SessionServer::setExit()
{
	status = SessionServer::EXIT;
}

uidsize_t SessionServer::validAuth(char* username, char* password)
{
	class EmptyAuth
	{
	public:
		int operator () (SafeInfo* info, fdtype_t type) { return 0; }
	};

	int fd = manager->open("/sys/user.d", OPTYPE_READ | OPTYPE_RAD_SHR_LOCK, EmptyAuth());
	if(fd == 0)
	{
		setErr(manager->getErrno());
		return 0;
	}
	FileGuard<XHYFileManager, int> fgrd(*manager, fd);
	manager->seek(fd, 0, XHYFileManager::end);
	int fileSize = manager->tell(fd);
	manager->seek(fd, 0, XHYFileManager::beg);
	if(fileSize < 0)
	{
		setErr(manager->getErrno());
		return 0;
	}
	char buf[_MAXUNAMESIZE > _MAXPSDSIZE ? _MAXUNAMESIZE : _MAXPSDSIZE];
	while(true)
	{
		uidsize_t uid;
		int curPos = manager->tell(fd);
		if(curPos < 0)
		{
			setErr(manager->getErrno());
			return 0;
		}
		if(curPos == fileSize)
		{
			setErr(ERR_UNAME_OR_PSD_ERROR);
			return 0;
		}
		if(!readFBuf(fd, (char*) &uid, sizeof(uid)))
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
		if(strcmp(buf, password) == 0)
			return uid;
		else
		{
			setErr(ERR_AUTH_ERR);
			return 0;
		}
	}
}

bool SessionServer::addableUser(char* username)
{
	class EmptyAuth
	{
	public:
		int operator () (SafeInfo* info, fdtype_t type) { return 0; }
	};

	int fd = manager->open("/sys/user.d", OPTYPE_READ | OPTYPE_RAD_SHR_LOCK, EmptyAuth());
	if(fd == 0)
	{
		setErr(manager->getErrno());
		return false;
	}
	FileGuard<XHYFileManager, int> fgrd(*manager, fd);
	manager->seek(fd, 0, XHYFileManager::end);
	int fileSize = manager->tell(fd);
	manager->seek(fd, 0, XHYFileManager::beg);
	if(fileSize < 0)
	{
		setErr(manager->getErrno());
		return false;
	}
	char buf[_MAXUNAMESIZE > _MAXPSDSIZE ? _MAXUNAMESIZE : _MAXPSDSIZE];
	while(true)
	{
		uidsize_t uid;
		int curPos = manager->tell(fd);
		if(curPos < 0)
		{
			setErr(manager->getErrno());
			return false;
		}
		if(curPos == fileSize)
			return true;
		if(!readFBuf(fd, (char*) &uid, sizeof(uid)))
			return false;
		if(!readFBuf(fd, buf, _MAXUNAMESIZE))
			return false;
		if(strcmp(buf, username) != 0)
		{
			manager->seek(fd, _MAXPSDSIZE, XHYFileManager::cur);
			continue;
		}
		else
		{
			setErr(ERR_USER_HAS_EXISTED);
			return false;
		}
	}
}

int SessionServer::pathValidateR(char* path, bool type, SafeInfo* sfInfo)
{
	if(!manager->getItemSafeInfo(path, sfInfo))
		return manager->getErrno();
	else
	{
		if(((bool) sfInfo->sign & FL_TYPE_FILE) != type)
			return 2;
		else if(!((sfInfo->uid == userId && sfInfo->sign & PM_U_READ) || (sfInfo->uid != userId && sfInfo->sign & PM_O_READ)))
			return 3;
		return 0;
	}
}

int SessionServer::pathValidateW(char* path, bool type, SafeInfo* sfInfo)
{
	
	if(!manager->getItemSafeInfo(path, sfInfo))
		return manager->getErrno();
	else
	{
		if(((bool) sfInfo->sign & FL_TYPE_FILE) != type)
			return 2;
		else if(!((sfInfo->uid == userId && sfInfo->sign & PM_U_WRITE) || (sfInfo->uid != userId && sfInfo->sign & PM_O_WRITE)))
			return 3;
		return 0;
	}
}

void SessionServer::createFile()
{
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	std::unique_ptr<char> path(new char[pathLen + 2]);
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	unsigned short nameLen;
	if(!readSBuf((char*) &nameLen, sizeof(nameLen)))
		return;
	std::unique_ptr<char> name(new char[nameLen + 1]);
	if(!readSBuf(name.get(), nameLen))
		return;
	name.get()[nameLen] = '\0';

	int rss = 0;
	std::lock_guard<XHYFileManager> lck(*manager);
	SafeInfo sfInfo;
	rss = pathValidateW(path.get(), false, &sfInfo);
	if(rss == 0)
	{
		if(!manager->createFile(path.get(), name.get(), userId))
			rss = manager->getErrno();
	}

	writeSBuf((char*) &rss, sizeof(rss));
}

void SessionServer::createFolder()
{
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	std::unique_ptr<char> path(new char[pathLen + 1]);
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	unsigned short nameLen;
	if(!readSBuf((char*) &nameLen, sizeof(nameLen)))
		return;
	std::unique_ptr<char> name(new char[nameLen + 1]);
	if(!readSBuf(name.get(), nameLen))
		return;
	name.get()[nameLen] = '\0';

	int rss = 0;
	std::lock_guard<XHYFileManager> lck(*manager);
	SafeInfo sfInfo;
	rss = pathValidateW(path.get(), false, &sfInfo);
	if(rss == 0)
	{
		if(!manager->createFolder(path.get(), name.get(), userId))
			rss = manager->getErrno();
	}

	writeSBuf((char*) &rss, sizeof(rss));
}

void SessionServer::readFolder()
{
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	std::unique_ptr<char> path(new char[pathLen + 1]);
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	SafeInfo sfInfo;
	int rss = pathValidateR(path.get(), 0, &sfInfo);
	if(rss == 0)
	{
		std::unique_ptr<FileNode> fnodes(new FileNode[20]);
		if(!writeSBuf((char*) &rss, sizeof(rss)))
			return;
		int len, start = 0;
		while(true)
		{
			len = manager->readFolder(path.get(), start, 20, fnodes.get());
			if(len == 0)
			{
				char tmp = 0;
				writeSBuf(&tmp, sizeof(tmp));
			}
			else if(len < 0)
				setErr(ERR_SYS_ERROR);
			else
			{
				start += len;
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
		writeSBuf((char*) &rss, sizeof(rss));
}

void SessionServer::changeModel()
{
	/**
	 * 验证权限的伪函数
	 * 只有文件的拥有者才能修改文件权限
	 */
	class CMAuth
	{
		uidsize_t userId;
	public:
		CMAuth(uidsize_t uid) : userId(uid) {}
		int operator () (SafeInfo* sfInfo)
		{
			if(userId == sfInfo->uid)
				return 0;
			return ERR_OPERATOR_DENY;
		}
	};

	//获取路径长度
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	//获取路径
	std::unique_ptr<char> path(new char[pathLen + 1]);
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	//获取要修改的模式(模式类似linux那套,但没有group)
	fsign sign;
	if(!readSBuf((char*) &sign, sizeof(sign)))
		return;
	SafeInfo sfInfo;
	sfInfo.sign = sign;
	int rss = 0;
	if(!manager->changeSafeInfo(path.get(), &sfInfo, CMAuth(userId)))
		rss = manager->getErrno();
	writeSBuf((char*) &rss, sizeof(rss));
}

void SessionServer::openFile()
{
	/**
	 * 验证权限的伪函数
	 * 根据当前用户和文件所有者及文件权限模式确定有没有权限
	 */
	class OFAuth
	{
		uidsize_t userId;
	public:
		OFAuth(uidsize_t uid) : userId(uid) {}
		int operator () (SafeInfo* sfInfo, fdtype_t type)
		{
			if(userId == sfInfo->uid)
			{
				//如果以读的方式打开且没有'所有者读',返回错误码
				if((type & OPTYPE_READ) && !(sfInfo->sign & PM_U_READ))
					return ERR_OPERATOR_DENY;
				//如果以写的方式打开且没有'所有者写',返回错误码
				if((type & OPTYPE_WRITE) && !(sfInfo->sign & PM_U_WRITE))
					return ERR_OPERATOR_DENY;
			}
			else
			{
				//如果以读的方式打开且没有'他人读',返回错误码
				if((type & OPTYPE_READ) && !(sfInfo->sign & PM_O_READ))
					return ERR_OPERATOR_DENY;
				//如果以写的方式打开且没有'他人写',返回错误码
				if((type & OPTYPE_WRITE) && !(sfInfo->sign & PM_O_WRITE))
					return ERR_OPERATOR_DENY;
			}
			return 0;
		}
	};

	//获取路径长度
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	//获取路径
	std::unique_ptr<char> path(new char[pathLen + 1]);
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	fdtype_t type;
	if(!readSBuf((char*) &type, sizeof(type)))
		return;
	int fd = manager->open(path.get(), type, OFAuth(userId));
	writeSBuf((char*) &fd, sizeof(fd));
	//如果open失败,追加一个错误码
	if(fd == 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
	}
}

void SessionServer::readFile()
{
	//获取文件描述符
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	//获取期望长度
	size_t want;
	if(!readSBuf((char*) &want, sizeof(want)))
		return;
	std::unique_ptr<char> buf(new char[want]);
	int len = manager->read(fd, buf.get(), want);
	writeSBuf((char*) &len, sizeof(len));
	//如果读失败,返回错误码,否则返回内容
	if(len < 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
	}
	else
		writeSBuf(buf.get(), len);
}

void SessionServer::writeFile()
{
	//获取文件描述符
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	//获取期望长度
	size_t want;
	if(!readSBuf((char*) &want, sizeof(want)))
		return;
	std::unique_ptr<char> buf(new char[want]);
	if(!readSBuf(buf.get(), want))
		return;
	int len = manager->write(fd, buf.get(), want);
	writeSBuf((char*) &len, sizeof(len));
	//如果写失败,返回错误码
	if(len < 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
	}
}

void SessionServer::seekFile()
{
	//获取文件描述符
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	int offset;
	if(!readSBuf((char*) &offset, sizeof(offset)))
		return;
	char type;
	if(!readSBuf(&type, sizeof(type)))
		return;
	XHYFileManager::FPos lpos;
	switch(type)
	{
	case 'c':
		lpos = XHYFileManager::cur;
		break;
	case 'b':
		lpos = XHYFileManager::beg;
		break;
	case 'e':
		lpos = XHYFileManager::end;
		break;
	default:
		lpos = XHYFileManager::cur;
		break;
	}
	int old = manager->seek(fd, offset, lpos);
	writeSBuf((char*) &old, sizeof(old));
	//如果失败,返回错误码
	if(old < 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
	}
}

void SessionServer::tellFile()
{
	//获取文件描述符
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	int old = manager->tell(fd);
	writeSBuf((char*) &old, sizeof(old));
	//如果失败,返回错误码
	if(old < 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
	}
}

void SessionServer::closeFile()
{
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	bool rs = manager->close(fd);
	writeSBuf((char*) &rs, sizeof(rs));
	//如果失败,返回错误码
	if(rs == false)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
	}
}

void SessionServer::deleteItem()
{
	class DIPAuth
	{
		uidsize_t userId;
	public:
		DIPAuth(uidsize_t uid) : userId(uid) {}
		int operator () (SafeInfo* sfInfo)
		{
			if(userId == sfInfo->uid)
			{
				//父文件夹要有写的权限
				if(!(sfInfo->sign & PM_U_WRITE))
					return ERR_OPERATOR_DENY;
			}
			else
			{
				//父文件夹要有写的权限
				if(!(sfInfo->sign & PM_O_WRITE))
					return ERR_OPERATOR_DENY;
			}
			return 0;
		}
	};

	class DISAuth
	{
	public:
		int operator () (SafeInfo* sfInfo)
		{
			//删除文件与文件本身的权限无关
			return 0;
		}
	};

	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	//获取路径
	std::unique_ptr<char> path(new char[pathLen + 1]);
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	bool rs = manager->deleteItem(path.get(), DIPAuth(userId), DISAuth());
	writeSBuf((char*) &rs, sizeof(rs));
	if(rs == false)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
	}
}

void SessionServer::getItemSafeInfo()
{
	unsigned short pathLen;
	if(!readSBuf((char*) &pathLen, sizeof(pathLen)))
		return;
	std::unique_ptr<char> path(new char[pathLen + 1]);
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	SafeInfo sfInfo;
	int rss = pathValidateR(path.get(), 0, &sfInfo);
	writeSBuf((char*) &rss, sizeof(rss));
	if(rss == 0)
		writeSBuf((char*) &sfInfo, sizeof(sfInfo));
}

// void SessionServer::formatDisk()
// {
// 	class EmptyAuth
// 	{
// 	public:
// 		int operator () (SafeInfo* info, fdtype_t type) { return 0; }
// 	};

// 	std::lock_guard<XHYFileManager> lck(*manager);
// 	manager->initFS();
// 	manager->createFolder("/", "sys", 0);
// 	manager->createFile("/sys", "user.d", 0);
// 	int fd = manager->open("/sys/user.d", OPTYPE_WRITE, EmptyAuth());
// 	if(fd == 0)
// 	{
// 		int merr = manager->getErrno();
// 		writeSBuf((char*) &merr, sizeof(merr));
// 		return;
// 	}
// 	FileGuard<XHYFileManager, int> fg(*manager, fd);
// 	char uname[_MAXUNAMESIZE];
// 	char psd[_MAXPSDSIZE];
// 	memset(uname, 0, _MAXUNAMESIZE);
// 	memset(psd, 0, _MAXPSDSIZE);
// 	strcpy(uname, "root");
// 	strcpy(psd, "root");
// 	int id = 1;
// 	if(!writeFBuf(fd, (char*) &id, sizeof(id)))
// 	{
// 		int merr = manager->getErrno();
// 		writeSBuf((char*) &merr, sizeof(merr));
// 		return;
// 	}
// 	if(!writeFBuf(fd, uname, _MAXUNAMESIZE))
// 	{
// 		int merr = manager->getErrno();
// 		writeSBuf((char*) &merr, sizeof(merr));
// 		return;
// 	}
// 	if(!writeFBuf(fd, psd, _MAXPSDSIZE))
// 	{
// 		int merr = manager->getErrno();
// 		writeSBuf((char*) &merr, sizeof(merr));
// 		return;
// 	}
// 	int rs = 0;
// 	writeSBuf((char*) &rs, sizeof(rs));
// }

void SessionServer::logout()
{
	status = SessionServer::EXIT;
}

void SessionServer::cmdError()
{
	status = SessionServer::ERROR;
	err = ERR_CMDERR;
}

void SessionServer::addUsr()
{
	class EmptyAuth
	{
	public:
		int operator () (SafeInfo* info, fdtype_t type) { return 0; }
	};

	char username[_MAXUNAMESIZE];
	char password[_MAXPSDSIZE];
	if(!readSBuf(username, _MAXUNAMESIZE))
		return;
	if(!readSBuf(password, _MAXPSDSIZE))
		return;
	if(userId != 1)
	{
		int rs = ERR_OPERATOR_DENY;
		writeSBuf((char*) &rs, sizeof(rs));
		return;
	}
	std::lock_guard<XHYFileManager> flg(*manager);
	if(!addableUser(username))
	{
		int merr = err;
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	int fd = manager->open("/sys/user.d", OPTYPE_READ | OPTYPE_WRITE, EmptyAuth());
	if(fd == 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	FileGuard<XHYFileManager, int> fgrd(*manager, fd);
	int tx = manager->seek(fd, 0, XHYFileManager::end);
	if(tx < 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	tx = manager->tell(fd);
	if(tx < 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	uidsize_t id = tx / (sizeof(uidsize_t) + _MAXUNAMESIZE + _MAXPSDSIZE) + 1;
	if(!writeFBuf(fd, (char*) &id, sizeof(id)))
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, username, _MAXUNAMESIZE))
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, password, _MAXPSDSIZE))
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	int rs = 0;
	writeSBuf((char*) &rs, sizeof(rs));
}

void SessionServer::changePsd()
{
	class EmptyAuth
	{
	public:
		int operator () (SafeInfo* info, fdtype_t type) { return 0; }
	};
	char password[_MAXPSDSIZE];
	if(!readSBuf(password, _MAXPSDSIZE))
		return;
	int fd = manager->open("/sys/user.d", OPTYPE_WRITE, EmptyAuth());
	if(fd == 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	FileGuard<XHYFileManager, int> fgrd(*manager, fd);
	int offset = (userId - 1) * (sizeof(uidsize_t)+ _MAXUNAMESIZE + _MAXPSDSIZE) + sizeof(uidsize_t) + _MAXUNAMESIZE;
	int tx = manager->seek(fd, offset, XHYFileManager::beg);
	if(tx < 0)
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, password, _MAXPSDSIZE))
	{
		int merr = manager->getErrno();
		writeSBuf((char*) &merr, sizeof(merr));
		return;
	}
	int rs = 0;
	writeSBuf((char*) &rs, sizeof(rs));
}
