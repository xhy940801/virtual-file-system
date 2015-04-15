#ifndef _SESSION_H
#define _SESSION_H

#include "types.h"
#include "XHYFileManager.h"
#include "serrordef.h"

class SessionServer
{
	enum Status {NLOGIN, LOOPING, EXIT, ERROR};
private:
	XHYFileManager* manager;
	int socket;
	Status status;
	int err;
private:
	uidsize_t userId;
private:
	void rlogin();
	void rloop();

	bool readSBuf(char* buf, size_t len);
	bool writeSBuf(const char* buf, size_t len);

	bool readFBuf(int fd, char* buf, size_t len);
	bool writeFBuf(int fd, const char* buf, size_t len);

	void printClosedInfo();
	void printErrorInfo();
	void printConnectionInfo();

	void setErr(int error);
	void setExit();
private:
	bool validHeader();
	void createFile();
	void createFolder();
	void readFolder();
	void changeModel();
	void openFile();
	void readFile();
	void writeFile();
	void seekFile();
	void tellFile();
	void closeFile();
	void deleteItem();
	void getItemSafeInfo();
	//void formatDisk();
	void logout();
	void cmdError();
	void addUsr();
	void changePsd();
private:
	uidsize_t validAuth(char* username, char* password);
	bool addableUser(char* username);
	int pathValidateR(char* path, bool type, SafeInfo* sfInfo);
	int pathValidateW(char* path, bool type, SafeInfo* sfInfo);
public:
	SessionServer(XHYFileManager* mgr, int sock);
	void run();
	~SessionServer();
};

#endif