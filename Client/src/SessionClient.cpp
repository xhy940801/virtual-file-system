#include "SessionClient.h"

#include "sdef.h"
#include "serrordef.h"

#include <cstring>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

SessionClient::SessionClient(int sock)
	: socket(sock), err(0)
{
}

SessionClient::~SessionClient()
{
	close(socket);
}

bool SessionClient::readSBuf(char* buf, size_t len)
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
			setErr(ERR_SOCKET_CLOSED);
			return false;
		}
		pos += rs;
	}
	return true;
}

bool SessionClient::writeSBuf(const char* buf, size_t len)
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
			setErr(ERR_SOCKET_CLOSED);
			return false;
		}
		pos += rs;
	}
	return true;
}

inline void SessionClient::setErr(int _err)
{
	err = _err;
}

bool SessionClient::sendValidHead()
{
	int rs = write(socket, "X", 1);
	if(rs < 0)
	{
		setErr(ERR_SOCKET_EXCEPTION);
		return false;
	}
	if(rs == 0)
	{
		setErr(ERR_SOCKET_CLOSED);
		return false;
	}
	return true;
}

int SessionClient::login(const char* username, const char* password)
{
	if(!sendValidHead())
		return 0;
	char tun[_MAXUNAMESIZE];
	char tpd[_MAXPSDSIZE];

	memset(tun, 0, _MAXUNAMESIZE);
	memset(tpd, 0, _MAXPSDSIZE);

	if(strlen(username) > 31)
	{
		setErr(ERR_UNAME_OVERLENGTH);
		return 0;
	}

	if(strlen(password) > 31)
	{
		setErr(ERR_PSD_OVERLENGTH);
		return 0;
	}

	strcpy(tun, tpd);
	strcpy(tpd, password);

	if(!writeSBuf(username, _MAXUNAMESIZE))
		return 0;
	if(!writeSBuf(password, _MAXPSDSIZE))
		return 0;

	uidsize_t rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return 0;
	if(rs == 0)
	{
		int rss;
		if(!readSBuf((char*) &rss, sizeof(rss)))
			return 0;
		setErr(rss);
		return 0;
	}
	return rs;
}

bool SessionClient::createFile(const char* path, const char* name)
{
	if(!sendValidHead())
		return false;
	unsigned short pathLen = strlen(path);
	unsigned short nameLen = strlen(name);

	const char* cmd = "cfl";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;

	if(!writeSBuf((char*) &pathLen, sizeof(pathLen)))
		return false;
	if(!writeSBuf(path, pathLen))
		return false;
	if(!writeSBuf((char*) &nameLen, sizeof(nameLen)))
		return false;
	if(!writeSBuf(name, nameLen))
		return false;

	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs != 0)
	{
		setErr(rs);
		return false;
	}
	return true;
}

bool SessionClient::createFolder(const char* path, const char* name)
{
	if(!sendValidHead())
		return false;
	unsigned short pathLen = strlen(path);
	unsigned short nameLen = strlen(name);

	const char* cmd = "cdr";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;

	if(!writeSBuf((char*) &pathLen, sizeof(pathLen)))
		return false;
	if(!writeSBuf(path, pathLen))
		return false;
	if(!writeSBuf((char*) &nameLen, sizeof(nameLen)))
		return false;
	if(!writeSBuf(name, nameLen))
		return false;

	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs != 0)
	{
		setErr(rs);
		return false;
	}
	return true;
}

bool SessionClient::readFolder(const char* path, std::vector<std::string>& fileList)
{
	if(!sendValidHead())
		return false;
	unsigned short pathLen = strlen(path);

	const char* cmd = "rdr";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;

	if(!writeSBuf((char*) &pathLen, sizeof(pathLen)))
		return false;
	if(!writeSBuf(path, pathLen))
		return false;

	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs != 0)
	{
		setErr(rs);
		return false;
	}

	while(true)
	{
		char name[32];
		char nlen;
		if(!readSBuf((char*) &nlen, sizeof(nlen)))
			return false;
		if(nlen == 0)
			break;
		if(!readSBuf(name, nlen))
			return false;
		name[(size_t) nlen] = '\0';
		fileList.push_back(std::string(name));
	}

	return true;
}

bool SessionClient::changeModel(const char* path, fsign sign)
{
	if(!sendValidHead())
		return false;
	unsigned short pathLen = strlen(path);

	const char* cmd = "cmd";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;

	if(!writeSBuf((char*) &pathLen, sizeof(pathLen)))
		return false;
	if(!writeSBuf(path, pathLen))
		return false;
	if(!writeSBuf((char*) &sign, sizeof(sign)))
		return false;

	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs != 0)
	{
		setErr(rs);
		return false;
	}
	return true;
}

int SessionClient::openFile(const char* path, fdtype_t type)
{
	if(!sendValidHead())
		return -1;
	unsigned short pathLen = strlen(path);

	const char* cmd = "ofl";

	if(!writeSBuf(cmd, strlen(cmd)))
		return -1;

	if(!writeSBuf((char*) &pathLen, sizeof(pathLen)))
		return -1;
	if(!writeSBuf(path, pathLen))
		return -1;
	if(!writeSBuf((char*) &type, sizeof(type)))
		return -1;

	int fd;
	if(!readSBuf((char*) &fd, sizeof(fd)))
		return -1;
	if(fd == 0)
	{
		int rs;
		if(!readSBuf((char*) &rs, sizeof(rs)))
			return fd;
		setErr(rs);
		return fd;
	}
	return fd;
}

int SessionClient::readFile(int fd, char* buf, size_t len)
{
	if(!sendValidHead())
		return -1;

	const char* cmd = "rfl";

	if(!writeSBuf(cmd, strlen(cmd)))
		return -1;
	if(!writeSBuf((char*) &fd, sizeof(fd)))
		return -1;

	if(!writeSBuf((char*) &len, sizeof(len)))
		return -1;

	int tl;
	if(!readSBuf((char*) &tl, sizeof(tl)))
		return -1;
	if(tl < 0)
	{
		int rerr;
		if(!readSBuf((char*) &rerr, sizeof(rerr)))
			return tl;
		setErr(rerr);
		return tl;
	}
	if(!readSBuf(buf, tl))
		return -1;
	return tl;
}

int SessionClient::writeFile(int fd, const char* buf, size_t len)
{
	if(!sendValidHead())
		return -1;

	const char* cmd = "wfl";

	if(!writeSBuf(cmd, strlen(cmd)))
		return -1;
	if(!writeSBuf((char*) &fd, sizeof(fd)))
		return -1;

	if(!writeSBuf((char*) &len, sizeof(len)))
		return -1;
	if(!writeSBuf(buf, len))
		return -1;

	int tl;
	if(!readSBuf((char*) &tl, sizeof(tl)))
		return -1;
	if(tl < 0)
	{
		int rerr;
		if(!readSBuf((char*) &rerr, sizeof(rerr)))
			return tl;
		setErr(rerr);
		return tl;
	}
	return tl;
}

int SessionClient::seekFile(int fd, int offset, char pos)
{
	if(!sendValidHead())
		return -1;

	const char* cmd = "skf";

	if(!writeSBuf(cmd, strlen(cmd)))
		return -1;
	if(!writeSBuf((char*) &fd, sizeof(fd)))
		return -1;
	if(!writeSBuf((char*) &offset, sizeof(offset)))
		return -1;
	if(!writeSBuf((char*) &pos, sizeof(pos)))
		return -1;
	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return -1;
	if(rs < 0)
	{
		int rerr;
		if(!readSBuf((char*) &rerr, sizeof(rerr)))
			return rs;
		setErr(rerr);
		return rs;
	}
	return rs;
}

int SessionClient::tellFile(int fd)
{
	if(!sendValidHead())
		return -1;

	const char* cmd = "tlf";

	if(!writeSBuf(cmd, strlen(cmd)))
		return -1;
	if(!writeSBuf((char*) &fd, sizeof(fd)))
		return -1;
	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return -1;
	if(rs < 0)
	{
		int rerr;
		if(!readSBuf((char*) &rerr, sizeof(rerr)))
			return rs;
		setErr(rerr);
		return rs;
	}
	return rs;
}

bool SessionClient::closeFile(int fd)
{
	if(!sendValidHead())
		return false;

	const char* cmd = "csf";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;
	if(!writeSBuf((char*) &fd, sizeof(fd)))
		return false;
	bool rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs == false)
	{
		int rerr;
		if(!readSBuf((char*) &rerr, sizeof(rerr)))
			return false;
		setErr(rerr);
		return false;
	}
	return true;
}

bool SessionClient::deleteItem(const char* path)
{
	if(!sendValidHead())
		return false;
	unsigned short pathLen = strlen(path);

	const char* cmd = "del";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;

	if(!writeSBuf((char*) &pathLen, sizeof(pathLen)))
		return false;
	if(!writeSBuf(path, pathLen))
		return false;

	bool rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs == false)
	{
		int rerr;
		if(!readSBuf((char*) &rerr, sizeof(rerr)))
			return false;
		setErr(rerr);
		return false;
	}
	return true;
}

bool SessionClient::getItemSafeInfo(const char* path, SafeInfo* sfInfo)
{
	if(!sendValidHead())
		return false;
	unsigned short pathLen = strlen(path);

	const char* cmd = "gsf";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;

	if(!writeSBuf((char*) &pathLen, sizeof(pathLen)))
		return false;
	if(!writeSBuf(path, pathLen))
		return false;

	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs != 0)
	{
		setErr(rs);
		return false;
	}
	if(!readSBuf((char*) sfInfo, sizeof(SafeInfo)))
		return false;
	return true;
}

// bool SessionClient::formatDisk()
// {
// 	if(!sendValidHead())
// 		return false;

// 	const char* cmd = "fdk";

// 	if(!writeSBuf(cmd, strlen(cmd)))
// 		return false;

// 	int rs;
// 	if(!readSBuf((char*) &rs, sizeof(rs)))
// 		return false;
// 	if(rs != 0)
// 	{
// 		setErr(rs);
// 		return false;
// 	}
// 	return true;
// }

bool SessionClient::logout()
{
	if(!sendValidHead())
		return false;

	const char* cmd = "lgt";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;
	return true;
}

bool SessionClient::addUsr(const char* username, const char* password)
{
	if(!sendValidHead())
		return false;

	const char* cmd = "aur";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;
	char tun[_MAXUNAMESIZE];
	char tpd[_MAXPSDSIZE];

	memset(tun, 0, _MAXUNAMESIZE);
	memset(tpd, 0, _MAXPSDSIZE);

	if(strlen(username) > 31)
	{
		setErr(ERR_UNAME_OVERLENGTH);
		return false;
	}

	if(strlen(password) > 31)
	{
		setErr(ERR_PSD_OVERLENGTH);
		return false;
	}

	strcpy(tun, tpd);
	strcpy(tpd, password);

	if(!writeSBuf(username, _MAXUNAMESIZE))
		return false;
	if(!writeSBuf(password, _MAXPSDSIZE))
		return false;

	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs != 0)
	{
		setErr(rs);
		return false;
	}
	return true;
}

bool SessionClient::changePsd(const char* password)
{
	if(!sendValidHead())
		return false;

	const char* cmd = "cpd";

	if(!writeSBuf(cmd, strlen(cmd)))
		return false;
	char tpd[_MAXPSDSIZE];
	memset(tpd, 0, _MAXPSDSIZE);

	if(strlen(password) > 31)
	{
		setErr(ERR_PSD_OVERLENGTH);
		return false;
	}

	strcpy(tpd, password);
	if(!writeSBuf(password, _MAXPSDSIZE))
		return false;

	int rs;
	if(!readSBuf((char*) &rs, sizeof(rs)))
		return false;
	if(rs != 0)
	{
		setErr(rs);
		return false;
	}
	return true;
}

int SessionClient::getErrno()
{
	return err;
}