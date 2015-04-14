#include "Session.h"

#include "sdef.h"
#include "serrordef.h"

#include "FileGuard.h"

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
	if(!readSBuf(username, _MAXUNAMESIZE))
		return;
	if(!readSBuf(password, _MAXPSDSIZE))
		return;
	int rrs;
	uidsize_t uid = validAuth(username, password);
	if(uid)
	{
		rrs = 1;
		status = Session::LOOPING;
		userId = uid;
	}
	else
		rrs = 0;
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
	else if(strcmp(cmd, "aur") == 0)
		addUsr();
	else if(strcmp(cmd, "cpd") == 0)
		changePsd();
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
	std::auto_ptr<char> path = new char[pathLen + 1];
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	//获取要修改的模式(模式类似linux那套,但没有group)
	fsign sign;
	if(!readSBuf((char*) sign, sizeof(sign)))
		return;
	SafeInfo sfInfo;
	sfInfo.sign = sign;
	rss = manager->changeSafeInfo(path.get(), &sfInfo, changeModel(userId));
	writeSBuf(&rss, sizeof(rss));
}

void Session::openFile()
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
				if(!((type & OPTYPE_READ) && (sfInfo->sign & PM_U_READ)))
					return ERR_OPERATOR_DENY;
				//如果以写的方式打开且没有'所有者写',返回错误码
				if(!((type & OPTYPE_WRITE) && (sfInfo->sign & PM_U_WRITE)))
					return ERR_OPERATOR_DENY;
			}
			else
			{
				//如果以读的方式打开且没有'他人读',返回错误码
				if(!((type & OPTYPE_READ) && (sfInfo->sign & PM_O_READ)))
					return ERR_OPERATOR_DENY;
				//如果以写的方式打开且没有'他人写',返回错误码
				if(!((type & OPTYPE_WRITE) && (sfInfo->sign & PM_O_WRITE)))
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
	std::auto_ptr<char> path = new char[pathLen + 1];
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	fdtype_t type;
	if(!readSBuf((char*) type, sizeof(type)))
		return;
	int fd = manager->open(path, type, OFAuth(userId));
	writeSBuf(&fd, sizeof(fd));
	//如果open失败,追加一个错误码
	if(fd == 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
	}
}

void Session::readFile()
{
	//获取文件描述符
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	//获取期望长度
	size_t want;
	if(!readSBuf((char*) &want, sizeof(want)))
		return;
	std::auto_ptr<char> buf = new char[pathLen + 1];
	int len = manager->read(fd, buf.get(), want);
	writeSBuf(&len, sizeof(len));
	//如果读失败,返回错误码,否则返回内容
	if(len < 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
	}
	else
		writeSBuf(buf.get(), len);
}

void Session::writeFile()
{
	//获取文件描述符
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	//获取期望长度
	size_t want;
	if(!readSBuf((char*) &want, sizeof(want)))
		return;
	std::auto_ptr<char> buf = new char[pathLen + 1];
	if(!readSBuf(buf.get(), want))
		return;
	int len = manager->write(fd, buf.get(), want);
	writeSBuf(&len, sizeof(len));
	//如果写失败,返回错误码
	if(len < 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
	}
}

void Session::seekFile()
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
	writeSBuf(&old, sizeof(old));
	//如果失败,返回错误码
	if(old < 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
	}
}

void Session::tellFile()
{
	//获取文件描述符
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	int old = manager->tell(fd);
	writeSBuf(&old, sizeof(old));
	//如果失败,返回错误码
	if(old < 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
	}
}

void Session::closeFile()
{
	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return;
	bool rs = manager->tell(fd);
	writeSBuf(&rs, sizeof(rs));
	//如果失败,返回错误码
	if(rs == false)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
	}
}

void Session::deleteItem()
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
				if(!((type & OPTYPE_WRITE) && (sfInfo->sign & PM_U_WRITE)))
					return ERR_OPERATOR_DENY;
			}
			else
			{
				//父文件夹要有写的权限
				if(!((type & OPTYPE_WRITE) && (sfInfo->sign & PM_O_WRITE)))
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
	std::auto_ptr<char> path = new char[pathLen + 1];
	if(!readSBuf(path.get(), pathLen))
		return;
	path.get()[pathLen] = '\0';
	bool rs = manager->deleteItem(path, DIPAuth(userId), DISAuth());
	writeSBuf(&rs, sizeof(rs));
}

void Session::getItemSafeInfo()
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
	writeSBuf(&rss, sizeof(rss));
	if(rss == 0)
		writeSBuf(&sfInfo, sizeof(sfInfo));
}

void Session::formatDisk()
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
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	FileGuard<XHYFileManager, int> fg(*manager, fd);
	char uname[_MAXUNAMESIZE];
	char psd[_MAXPSDSIZE];
	memset(uname, 0, _MAXUNAMESIZE);
	memset(psd, 0, _MAXPSDSIZE);
	strcpy(uname, "root");
	strcpy(psd, "root");
	int id = 1;
	if(!writeFBuf(fd, (char*) &id, sizeof(id)))
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, uname, _MAXUNAMESIZE))
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, psd, _MAXPSDSIZE))
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	int rs = 0;
	writeSBuf(&rs, sizeof(rs));
}

void Session::logout()
{
	status = Session::EXIT;
}

void Session::cmdError()
{
	status = Session::ERROR:
	err = ERR_CMDERR;
}

void Session::addUsr()
{
	class EmptyAuth
	{
		int operator () (SafeInfo* info) { return 0; }
	};

	char username[_MAXUNAMESIZE];
	char password[_MAXPSDSIZE];
	if(!readSBuf(username, _MAXUNAMESIZE))
		return;
	if(!readSBuf(password, _MAXPSDSIZE))
		return;
	if(uid != 1)
	{
		int rs = ERR_OPERATOR_DENY;
		writeSBuf((char*) rs, sizeof(rs));
	}
	int fd = manager->open("/sys/user.d", OPTYPE_WRITE, EmptyAuth());
	if(fd == 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	int tx = manager->seek(fd, 0, XHYFileManager::end);
	if(tx < 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	int id = tx / (sizeof(int), _MAXUNAMESIZE + _MAXPSDSIZE);
	if(!writeFBuf(fd, (char*) &id, sizeof(id)))
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, username, _MAXUNAMESIZE))
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, password, _MAXPSDSIZE))
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	int rs = 0;
	writeSBuf(&rs, sizeof(rs));
}

void Session::changePsd()
{
	class EmptyAuth
	{
		int operator () (SafeInfo* info) { return 0; }
	};
	char password[_MAXPSDSIZE];
	if(!readSBuf(password, _MAXPSDSIZE))
		return;
	int fd = manager->open("/sys/user.d", OPTYPE_WRITE, EmptyAuth());
	if(fd == 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	int offset = (userId - 1) * (sizeof(int), _MAXUNAMESIZE + _MAXPSDSIZE) + sizeof(int) + _MAXUNAMESIZE;
	int tx = manager->seek(fd, offset, XHYFileManager::beg);
	if(tx < 0)
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	if(!writeFBuf(fd, password, _MAXPSDSIZE))
	{
		int merr = manager->getErrno();
		writeSBuf(&merr, sizeof(merr));
		return;
	}
	int rs = 0;
	writeSBuf(&rs, sizeof(rs));
}
