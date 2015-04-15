#ifndef _SESSIONCLIENT_H
#define _SESSIONCLIENT_H

#include "types.h"
#include "FMStructs.h"
#include <cstddef>
#include <vector>
#include <string>

#define ERR_UNAME_OVERLENGTH 10
#define ERR_PSD_OVERLENGTH 11

class SessionClient
{
	int socket;
	int err;
private:
	bool readSBuf(char* buf, size_t len);
	bool writeSBuf(const char* buf, size_t len);

	bool sendValidHead();
	void setErr(int _err);
public:
	SessionClient(int sock);
	~SessionClient();

 	bool login(const char* username, const char* password);
	bool createFile(const char* path, const char* name);
	bool createFolder(const char* path, const char* name);
	bool readFolder(const char* path, std::vector<std::string>& fileList);
	bool changeModel(const char* path, fsign sign);
	int openFile(const char* path, fdtype_t type);
	int readFile(int fd, char* buf, size_t len);
	int writeFile(int fd, const char* buf, size_t len);
	int seekFile(int fd, int offset, char pos);
	int tellFile(int fd);
	bool closeFile(int fd);
	bool deleteItem(const char* path);
	bool getItemSafeInfo(const char* path, SafeInfo* sfInfo);
	//bool formatDisk();
	bool logout();
	bool addUsr(const char* username, const char* password);
	bool changePsd(const char* password);

	int getErrno();
};

#endif