#include "Shell.h"

#include <vector>

Shell::Shell(SessionClient* _sn)
	: session(_sn)
{
}

Shell::~Shell()
{
}

void Shell::select(const std::string& exec, size_t argc, const std::vector<std::string>& args)
{
	if(exec == "open")
		open(argc, argv);
	else if(exec == "seek")
		seek(argc, argv);
	else if(exec == "tell")
		tell(argc, argv);
	else if(exec == "write")
		write(argc, argv);
	else if(exec == "read")
		read(argc, argv);
	else if(exec == "useradd")
		useradd(argc, argv);
	else if(exec == "chpsd")
		chpsd(argc, argv);
	else if(exec == "cd")
		cd(argc, argv);
	else if(exec == "mkdir")
		mkdir(argc, argv);
	else if(exec == "mkfile")
		mkfile(argc, argv);
	else if(exec == "rm")
		rm(argc, argv);
	else if(exec == "cat")
		cat(argc, argv);
	else if(exec == "ocp")
		ocp(argc, argv);
	else
		printError(ERR_CMD_NOT_FOUND);
}

void Shell::parse(std::string cmd)
{
	cmd = trim(cmd) + ' ';
	std::vector<std::string> args;
	size_t start, end;
	for(start = 0, end = 0; end < cmd.size(); ++end)
	{
		if(cmd[end] == ' ' || cmd[end] == '\t')
		{
			if(cmd[start] != ' ' && cmd[start] != '\t')
				args.push_back(decode(cmd.substr(start, end)));
			start = end;
		}
	}
	if(args.size() == 0)
		return;
	select(args[0], args.size() - 1, args.data() + 1);
}

////////////////////////////////////////////////
////////////////private function////////////////
//////////////// exec functions ////////////////
////////////////////////////////////////////////

void Shell::open(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	std::string
	fdtype_t type;
	//根据参数填写打开的方式type
	for(size_t i = 1; i < argc, ++i)
	{
		std::string& arg = argv[i];
		if(arg == "-r")								//读的方式打开
			type |= OPTYPE_READ;
		else if(arg == "-w")						//写的方式打开
			type |= OPTYPE_WRITE;
		else if(arg == "-rw" || arg == "-wr")		//读写的方式打开
			type |= OPTYPE_WRITE | OPTYPE_READ;
		else if(arg == "-lmr")						//加上读互斥锁
			type |= OPTYPE_RAD_MTX_LOCK;
		else if(arg == "-lsr")						//加上读共享锁
			type |= OPTYPE_RAD_SHR_LOCK;
		else if(arg == "-lmw")						//加上写互斥锁
			type |= OPTYPE_WTE_MTX_LOCK;
		else if(arg == "-lsw")						//加上写共享锁
			type |= OPTYPE_WTE_SHR_LOCK
	}

	if(!(type & OPTYPE_READ) | (type & OPTYPE_WRITE))
		type |= OPTYPE_WRITE | OPTYPE_READ;
	int fd = session->open(argv[0].c_str(), type);
	if(fd == 0)
		printError(session->getErrno());
	else
		std::cout << "fd: " << fd << std::endl;
}

void Shell::seek(size_t argc, const std::string* argv)
{
	if(argc < 2)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	bool parseSuc;
	//获取文件描述符
	int fd = parseInt(argv[0], parseSuc);
	if(!parseSuc)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	//获取偏移量
	int offset = parseInt(argv[1], parseSuc);
	if(!parseSuc)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	bool pod = false;
	char pos = 'c';
	//解析参数
	for(size_t i = 2; i < argc, ++i)
	{
		std::string& arg = argv[i];
		if(arg == "-c")								//以当前位置为偏移点
			pos = 'c';
		else if(arg == "-e")						//以文件结束位置为起点
			pos = 'c';
		else if(arg == "-b")						//以文件起始位置为起点
			pos = 'b';
		else if(arg == "-pod")						//输出之前的位置
			pod = true;
	}

	int old = session->seekFile(fd, offset, pos);		//调用seek
	if(old < 0)
	{
		printError(session->getErrno());
		return;
	}
	if(pod)
		cout << "The old pos is " << old << endl;
}

void Shell::tell(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	bool parseSuc;
	//获取文件描述符
	int fd = parseInt(argv[0], parseSuc);
	if(!parseSuc)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}

	int fpos = session->tellFile(fd);		//调用seek
	if(fpos < 0)
	{
		printError(session->getErrno());
		return;
	}
	if(pod)
		cout << "The pos is " << fpos << endl;
}

void Shell::write(size_t argc, const std::string* argv)
{
}

void Shell::read(size_t argc, const std::string* argv)
{
}

void Shell::useradd(size_t argc, const std::string* argv)
{
}

void Shell::chpsd(size_t argc, const std::string* argv)
{
}

void Shell::cd(size_t argc, const std::string* argv)
{
}

void Shell::mkdir(size_t argc, const std::string* argv)
{
}

void Shell::mkfile(size_t argc, const std::string* argv)
{
}

void Shell::rm(size_t argc, const std::string* argv)
{
}

void Shell::cat(size_t argc, const std::string* argv)
{
}

void ocp(size_t argc, const std::string* argv)
{
}