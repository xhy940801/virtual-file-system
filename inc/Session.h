#ifndef _SESSION_H
#define _SESSION_H

#include "types.h"
#include "XHYFileManager.h"

class Session
{
	enum Status {NLOGIN, LOOPING, EXIT, ERROR};
private:
	XHYFileManager* manager;
	int socket;
	Status status;
	int err;
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
	void createFloder();
	void readFloder();
	void changeModel();
	void openFile();
	void readFile();
	void writeFile();
	void deleteItem();
	void getItemSafeInfo();
	void formatDisk();
	void logout();
	void cmdError();
private:
	uidsize_t validAuth(char* username, char* password);
public:
	Session(XHYFileManager* mgr, int sock);
	void run();
	~Session();
};

#endif