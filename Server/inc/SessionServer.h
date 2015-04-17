#ifndef _SESSION_H
#define _SESSION_H

#include "types.h"
#include "XHYFileManager.h"
#include "serrordef.h"
//会话,每个连接一个会话
class SessionServer
{
	enum Status {NLOGIN, LOOPING, EXIT, ERROR};	//四种状态:未登陆,运行中,退出,错误
private:
	XHYFileManager* manager;					//文件系统manager
	int socket;									//socket
	Status status;								//当前的状态
	int err;									//最后一次发生错误时的错误码
private:
	uidsize_t userId;							//当前用户id
private:
	void rlogin();								//login,登陆成功则跳到rloop()
	void rloop();								//主循环

	bool readSBuf(char* buf, size_t len);		//在Socket中读出指定字节信息
	bool writeSBuf(const char* buf, size_t len);//在Socket中写入指定字节信息

	bool readFBuf(int fd, char* buf, size_t len);//文件写
	bool writeFBuf(int fd, const char* buf, size_t len);//文件读
	//打印信息
	void printClosedInfo();						
	void printErrorInfo();
	void printConnectionInfo();
	//设置错误码,设置退出状态
	void setErr(int error);
	void setExit();
private:
	bool validHeader();		//验证头
	void createFile();		//创建文件
	void createFolder();	//创建文件夹
	void readFolder();		//读文件夹
	void changeModel();		//改变文件or文件夹的权限模式
	void openFile();		//打开文件
	void readFile();		//读取文件
	void writeFile();		//写入文件
	void seekFile();		//移动文件指针
	void tellFile();		//获取当前文件指针
	void closeFile();		//关闭文件
	void deleteItem();		//删除文件or文件夹
	void getItemSafeInfo();	//获取文件or文件夹详细信息
	//void formatDisk();
	void logout();			//登出
	void cmdError();		//当命令无效时调用
	void addUsr();			//新增用户
	void changePsd();		//改变密码
private:
	uidsize_t validAuth(char* username, char* password);	//验证登陆信息
	bool addableUser(char* username);						//检查能否加入用户名为username的用户(防止重名)
	int pathValidateR(char* path, bool type, SafeInfo* sfInfo);	//读文件权限验证
	int pathValidateW(char* path, bool type, SafeInfo* sfInfo);	//写文件权限验证
public:
	SessionServer(XHYFileManager* mgr, int sock);
	void run();	//主循环
	~SessionServer();
};

#endif