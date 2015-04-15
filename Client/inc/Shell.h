#ifndef _SHELL_H
#define _SHELL_H

#include <string>
#include <vector>

class Shell
{
	SessionClient* session;
private:
	std::string basePath;
private:
	void open(const std::vector<std::string>& args);
	void seek(const std::vector<std::string>& args);
	void tell(const std::vector<std::string>& args);
	void write(const std::vector<std::string>& args);
	void read(const std::vector<std::string>& args);
	void useradd(const std::vector<std::string>& args);
	void chpsd(const std::vector<std::string>& args);
	void cd(const std::vector<std::string>& args);
	void mkdir(const std::vector<std::string>& args);
	void mkfile(const std::vector<std::string>& args);
	void rm(const std::vector<std::string>& args);

	void runshell(const std::vector<std::string>& args);
public:
	Shell(SessionClient* _sn);
	~Shell();
	
};

#endif