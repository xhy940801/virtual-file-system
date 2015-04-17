#ifndef _SHELL_H
#define _SHELL_H

#include "SessionClient.h"

#include <string>
#include <cstdint>

#define ERR_CMD_NOT_FOUND 500
#define ERR_ARGS_INVALID 501
#define ERR_RS_IS_NOT_NUM 502
#define ERR_OFILE_OPEN_FAIL 503
#define ERR_OFILE_READ_FAIL 504
#define ERR_FILE_COULD_NOT_EXEC 505
#define ERR_UNKNOW 1

class SandBox;
//壳程序,对Session进行封装,并且与用户交互
class Shell
{
	friend class SandBox;
	SessionClient* session;
	uidsize_t uid;
private:
	std::string basePath;										//当前目录
private:
	static void printError(int err);							//打印错误代码
	std::string absPath(std::string path);						//把路径转换为绝对路径
	std::string evrPath(std::string path);						//把路径转换为环境变量的路径(即在/evr下的目录)
	static std::string parentPath(std::string path);			//获取父路径,如果path格式不正确返回""
private:
	void printShellError(size_t line, std::string info);		//打印shell脚本的错误

	void open(size_t argc, const std::string* argv);			//打开文件
	void seek(size_t argc, const std::string* argv);			//移动文件指针
	void tell(size_t argc, const std::string* argv);			//获取文件的当前指针
	void write(size_t argc, const std::string* argv);			//写文件
	void read(size_t argc, const std::string* argv);			//读文件
	void close(size_t argc, const std::string* argv);			//关闭文件
	void useradd(size_t argc, const std::string* argv);			//增加用户
	void chpsd(size_t argc, const std::string* argv);			//更改密码
	void cd(size_t argc, const std::string* argv);				//进入目录
	void mkdir(size_t argc, const std::string* argv);			//创建目录
	void mkfile(size_t argc, const std::string* argv);			//创建文件
	void rm(size_t argc, const std::string* argv);				//删除文件or文件夹
	void cat(size_t argc, const std::string* argv);				//输出文件内容
	void ls(size_t argc, const std::string* argv);				//列出当前目录
	void chmod(size_t argc, const std::string* argv);			//改变文件权限
	void see(size_t argc, const std::string* argv);				//改变文件权限
	void ocp(size_t argc, const std::string* argv);				//拷贝文件系统外的文件到文件系统内部

	void runshell(const std::string& exec, size_t argc, const std::string* argv);		//运行shell程序
public:
	Shell(SessionClient* _sn, uidsize_t _uid);
	~Shell();
	
	void select(const std::string& exec, size_t argc, const std::string* argv);		//根据exec选择函数运行
	void parse(std::string cmd);													//解析cmd,拆分成exec和参数
	std::string getBasePath();
	uidsize_t getUid();
};

#endif