#ifndef _SHELL_H
#define _SHELL_H

#include "SessionClient.h"

#include <string>

#define ERR_CMD_NOT_FOUND 500
#define ERR_ARGS_INVALID 501

class Shell
{
	SessionClient* session;
private:
	std::string basePath;									//当前目录
private:
	static std::string trim(std::string str);				//删除字符串首位空格
	static int parseInt(std::string str, bool& validate);	//把字符串转为int, 如果转换成功validate被赋为true,否则false
	static void printError(int err);						//打印错误代码
	std::string absPath(std::string path);					//把路径转换为绝对路径
	staticstd::string decode(std::string str);				//把参数中的转义字符去掉
private:
	void open(size_t argc, const std::string* argv);		//打开文件
	void seek(size_t argc, const std::string* argv);		//移动文件指针
	void tell(size_t argc, const std::string* argv);		//获取文件的当前指针
	void write(size_t argc, const std::string* argv);		//写文件
	void read(size_t argc, const std::string* argv);		//读文件
	void useradd(size_t argc, const std::string* argv);		//增加用户
	void chpsd(size_t argc, const std::string* argv);		//关闭文件
	void cd(size_t argc, const std::string* argv);			//进入目录
	void mkdir(size_t argc, const std::string* argv);		//创建目录
	void mkfile(size_t argc, const std::string* argv);		//创建文件
	void rm(size_t argc, const std::string* argv);			//删除文件or文件夹
	void cat(size_t argc, const std::string* argv);			//输出文件内容
	void ocp(size_t argc, const std::string* argv);			//拷贝文件系统外的文件到文件系统内部

	void runshell(size_t argc, const std::string* argv);	//运行shell程序
public:
	Shell(SessionClient* _sn);
	~Shell();
	
	void select(const std::string& exec, size_t argc, const std::string* argv);		//根据exec选择函数运行
	void parse(std::string cmd);													//解析cmd,拆分成exec和参数
};

#endif