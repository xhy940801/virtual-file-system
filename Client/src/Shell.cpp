#include "Shell.h"
#include "Guard.h"
#include "commondef.h"
#include "SandBox.h"
#include "functions.h"

#include <vector>
#include <memory>
#include <iostream>

#include <cstdio>

Shell::Shell(SessionClient* _sn, uidsize_t _uid)
	: session(_sn), uid(_uid)
{
	basePath = "/";
}

Shell::~Shell()
{
}

void Shell::select(const std::string& exec, size_t argc, const std::string* argv)
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
	else if(exec == "close")
		close(argc, argv);
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
	else if(exec == "ls")
		ls(argc, argv);
	else if(exec == "chmod")
		chmod(argc, argv);
	else if(exec == "see")
		see(argc, argv);
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
	bool model = false;
	for(start = 0, end = 0; end < cmd.size(); ++end)
	{
		if(model)
		{
			if(cmd[end] == '\\')
				++end;
			else if(cmd[end] == '\"')
			{
				args.push_back(decode(trim(cmd.substr(start, end - start))));
				++end;
				start = end;
				model = false;
			}
		}
		else
		{
			if(cmd[end] == ' ' || cmd[end] == '\t')
			{
				if(cmd[start] != ' ' && cmd[start] != '\t')
					args.push_back(decode(trim(cmd.substr(start, end - start))));
				start = end + 1;
			}
			else if(cmd[end] == '\"')
			{
				++end;
				start = end;
				model = true;
			}
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
	fdtype_t type = 0;
	//根据参数填写打开的方式type
	for(size_t i = 1; i < argc; ++i)
	{
		const std::string& arg = argv[i];
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
			type |= OPTYPE_WTE_SHR_LOCK;
	}

	if(!(type & OPTYPE_READ) | (type & OPTYPE_WRITE))
		type |= OPTYPE_WRITE | OPTYPE_READ;
	int fd = session->openFile(absPath(argv[0]).c_str(), type);
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
	for(size_t i = 2; i < argc; ++i)
	{
		const std::string& arg = argv[i];
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
		std::cout << "The old pos is " << old << std::endl;
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
	if(fpos)
		std::cout << "The pos is " << fpos << std::endl;
}

void Shell::write(size_t argc, const std::string* argv)
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
	enum Types {STRING, INT8, INT16, INT32, INT64};
	Types type = Types::STRING;
	for(size_t i = 2; i < argc; ++i)
	{
		const std::string& arg = argv[i];
		if(arg == "-s")								//以文字模式插入
			type = Types::STRING;
		else if(arg == "-i8")						//以int8模式插入
			type = Types::INT8;
		else if(arg == "-16")						//以int16模式插入
			type = Types::INT16;
		else if(arg == "-i32")						//输int32模式插入
			type = Types::INT32;
		else if(arg == "-i64")						//输int64模式插入
			type = Types::INT64;
	}
	int result;
	int64_t n64;
	int32_t n32;
	int16_t n16;
	int8_t n8;
	if(type != Types::STRING)
	{
		int64_t n64 = parseInt(argv[1], parseSuc);
		if(!parseSuc)
		{
			printError(ERR_ARGS_INVALID);
			return;
		}
		n32 = n64;
		n16 = n64;
		n8 = n64;
	}

	switch (type)
	{
	case Types::STRING:
		result = session->writeFile(fd, argv[1].c_str(), argv[1].size());
		break;
	case Types::INT8:
		result = session->writeFile(fd, (char*) &n8, sizeof(n8));
		break;
	case Types::INT16:
		result = session->writeFile(fd, (char*) &n16, sizeof(n16));
		break;
	case Types::INT32:
		result = session->writeFile(fd, (char*) &n32, sizeof(n32));
		break;
	case Types::INT64:
		result = session->writeFile(fd, (char*) &n64, sizeof(n64));
		break;
	}

	if(result < 0)
	{
		printError(session->getErrno());
		return;
	}
	std::cout << "write " << result << " bytes" << std::endl;
}

void Shell::read(size_t argc, const std::string* argv)
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
	int len = parseInt(argv[1], parseSuc);
	if(!parseSuc || len <= 0)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	enum Types {STRING, INT8, INT16, INT32, INT64};
	Types type = Types::STRING;
	for(size_t i = 2; i < argc; ++i)
	{
		const std::string& arg = argv[i];
		if(arg == "-s")								//以文字模式插入
			type = Types::STRING;
		else if(arg == "-i8")						//以int8模式插入
			type = Types::INT8;
		else if(arg == "-16")						//以int16模式插入
			type = Types::INT16;
		else if(arg == "-i32")						//输int32模式插入
			type = Types::INT32;
		else if(arg == "-i64")						//输int64模式插入
			type = Types::INT64;
	}
	int result;
	std::unique_ptr<char> buf(new char[len + 1]);
	result = session->readFile(fd, buf.get(), len);
	if(result < 0)
	{
		printError(session->getErrno());
		return;
	}
	if(result > 0)
	{
		buf.get()[result] = '\0';
		int64_t intRs;
		if(type != Types::STRING)
		{
			intRs = parseInt(std::string(buf.get()), parseSuc);
			if(!parseSuc)
			{
				printError(ERR_RS_IS_NOT_NUM);
				return;
			}
		}
		int8_t int8rs = intRs;
		switch (type)
		{
		case Types::STRING:
			std::cout << buf.get() << std::endl;
			break;
		case Types::INT8:
			std::cout << (int) int8rs << std::endl;
			break;
		case Types::INT16:
			std::cout << (int16_t) intRs << std::endl;
			break;
		case Types::INT32:
			std::cout << (int32_t) intRs << std::endl;
			break;
		case Types::INT64:
			std::cout << (int64_t) intRs << std::endl;
			break;
		}
	}
	std::cout << "read " << result << " bytes" << std::endl;
}

void Shell::close(size_t argc, const std::string* argv)
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

	bool rs = session->closeFile(fd);		//调用closeFile
	if(!rs)
		printError(session->getErrno());
}

void Shell::useradd(size_t argc, const std::string* argv)
{
	if(argc < 2)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	if(!session->addUsr(argv[0].c_str(), argv[1].c_str()))
		printError(session->getErrno());
}

void Shell::chpsd(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	if(!session->changePsd(argv[0].c_str()))
		printError(session->getErrno());
}

void Shell::cd(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	basePath = absPath(argv[0]);
}

void Shell::mkdir(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	std::string abspth, parent, name;
	abspth = absPath(argv[0]);
	parent = parentPath(abspth);
	if(parent == "/")
		name = abspth.substr(1, abspth.size() - 1);
	else
		name = abspth.substr(parent.size() + 1, abspth.size() - (parent.size() + 1));
	if(parent == "" || name == "")
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	if(!session->createFolder(parent.c_str(), name.c_str()))
		printError(session->getErrno());
}

void Shell::mkfile(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	std::string abspth, parent, name;
	abspth = absPath(argv[0]);
	parent = parentPath(abspth);
	if(parent == "/")
		name = abspth.substr(1, abspth.size() - 1);
	else
		name = abspth.substr(parent.size() + 1, abspth.size() - (parent.size() + 1));
	if(parent == "" || name == "")
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	if(!session->createFile(parent.c_str(), name.c_str()))
		printError(session->getErrno());
}

void Shell::rm(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	if(!session->deleteItem(absPath(argv[0]).c_str()))
		printError(session->getErrno());
}

void Shell::cat(size_t argc, const std::string* argv)
{
	class _FDGuard
	{
		SessionClient* session;
	public:
		_FDGuard(SessionClient* _ss) : session(_ss) {}
		void operator () (int fd) { session->closeFile(fd); }
	};
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	const size_t BUFSIZE = 64;
	std::unique_ptr<char> buf(new char[BUFSIZE + 1]);
	int fd = session->openFile(absPath(argv[0]).c_str(), OPTYPE_READ);
	if(fd <= 0)
	{
		printError(session->getErrno());
		return;
	}
	Guard<int, _FDGuard> gd(fd, _FDGuard(session));
	size_t start = 0;
	while(true)
	{
		int len = session->readFile(fd, buf.get(), BUFSIZE);
		if(len < 0)
		{
			printError(session->getErrno());
			return;
		}
		else if(len == 0)
			break;
		else
		{
			start += len;
			buf.get()[len] = '\0';
			std::cout << buf.get();
		}
	}
	std::cout << std::endl;
	std::cout << "-------END-------" << std::endl;
}

void Shell::ls(size_t argc, const std::string* argv)
{
	std::string path;
	if(argc < 1)
		path = basePath;
	else
		path = absPath(argv[0]);
	std::vector<std::string> files;
	if(!session->readFolder(path.c_str(), files))
	{
		printError(session->getErrno());
		return;
	}
	if (files.size() == 0)
	{
		std::cout << "-------empty folder-------";
	}
	for(size_t i = 0; i < files.size(); ++i)
		std::cout << files[i] << "\t\t";
	std::cout << std::endl;
}

void Shell::chmod(size_t argc, const std::string* argv)
{
	if(argc < 2)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	bool parseSuc;
	int mod = parseInt(argv[1], parseSuc);
	if(!parseSuc || mod / 10 > 7 || mod % 10 > 7)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	fsign sign = mod / 10 + (mod % 10) * 8;
	if(!session->changeModel(absPath(argv[0]).c_str(), sign))
		printError(session->getErrno());
}

void Shell::see(size_t argc, const std::string* argv)
{
	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	SafeInfo sfInfo;
	if(!session->getItemSafeInfo(absPath(argv[0]).c_str(), &sfInfo))
		printError(session->getErrno());
	if(sfInfo.sign & FL_TYPE_FILE)
		std::cout << "file:\t" << absPath(argv[0]);
	else
		std::cout << "folder:\t" << absPath(argv[0]);
	std::cout << "\tmode:\t" << sfInfo.sign % 8 << (sfInfo.sign / 8) % 8;
	std::cout << "\towner:\t" << sfInfo.uid;
	std::cout << "\tcreated:\t" << sfInfo.created;
	std::cout << "\tmodified:\t" << sfInfo.modified;
	std::cout << std::endl;
}

void Shell::ocp(size_t argc, const std::string* argv)
{
	class _FDGuard
	{
		SessionClient* session;
	public:
		_FDGuard(SessionClient* _ss) : session(_ss) {}
		void operator () (int fd) { session->closeFile(fd); }
	};

	class _OFDGuard
	{
	public:
		void operator () (FILE* fd) { fclose(fd); }
	};
	if(argc < 2)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}

	FILE* ofd = fopen(argv[0].c_str(), "r");
	if(ofd == nullptr)
		printError(ERR_OFILE_OPEN_FAIL);
	Guard<FILE*, _OFDGuard> ogd(ofd, _OFDGuard());
	std::string abspth, parent, name;
	abspth = absPath(argv[1]);
	parent = parentPath(abspth);
	if(parent == "/")
		name = abspth.substr(1, abspth.size() - 1);
	else
		name = abspth.substr(parent.size() + 1, abspth.size() - (parent.size() + 1));
	if(parent == "" || name == "")
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	if(!session->createFile(parent.c_str(), name.c_str()))
		printError(session->getErrno());
	int fd = session->openFile(absPath(argv[1]).c_str(), OPTYPE_WRITE);
	if(fd <= 0)
	{
		printError(session->getErrno());
		return;
	}
	Guard<int, _FDGuard> gd(fd, _FDGuard(session));
	size_t start = 0;
	const size_t BUFSIZE = 64;
	std::unique_ptr<char> buf(new char[BUFSIZE]);
	while(true)
	{
		int len = fread(buf.get() + start, sizeof(char), BUFSIZE - start, ofd);
		if(len < 0)
		{
			printError(ERR_OFILE_READ_FAIL);
			return;
		}
		else if(len == 0)
			break;
		else
		{
			start += len;
			int has = 0;
			while(has != len)
			{
				int cl = session->writeFile(fd, buf.get() + has, len - has);
				if(cl < 0)
				{
					printError(session->getErrno());
					return;
				}
				if(cl == 0)
				{
					printError(ERR_UNKNOW);
					return;
				}
				has += cl;
			}
		}
	}
}

void Shell::runshell(size_t argc, const std::string* argv)
{
	class _FDGuard
	{
		SessionClient* session;
	public:
		_FDGuard(SessionClient* _ss) : session(_ss) {}
		void operator () (int fd) { session->closeFile(fd); }
	};

	//检查是否有可读权限
	SafeInfo sfInfo;
	if(!session->getItemSafeInfo(evrPath(argv[0]).c_str(), &sfInfo))
		printError(session->getErrno());
	if((sfInfo.uid == uid && !(sfInfo.sign & PM_U_EXECUTE)) || (sfInfo.uid != uid && !(sfInfo.sign & PM_O_EXECUTE)))
	{
		printError(ERR_FILE_COULD_NOT_EXEC);
		return;
	}

	std::string sh;

	if(argc < 1)
	{
		printError(ERR_ARGS_INVALID);
		return;
	}
	const size_t BUFSIZE = 64;
	std::unique_ptr<char> buf(new char[BUFSIZE + 1]);
	//打开文件
	int fd = session->openFile(evrPath(argv[0]).c_str(), OPTYPE_READ);
	if(fd <= 0)
	{
		printError(session->getErrno());
		return;
	}
	Guard<int, _FDGuard> gd(fd, _FDGuard(session));
	size_t start = 0;
	//把文件内容写入sh中
	while(true)
	{
		int len = session->readFile(fd, buf.get(), BUFSIZE);
		if(len < 0)
		{
			printError(session->getErrno());
			return;
		}
		else if(len == 0)
			break;
		else
		{
			start += len;
			buf.get()[len] = '\0';
			sh += buf.get();
		}
	}

	//开启个新的沙盒运行之
	SandBox sd(this);
	sd.run(sh, argc - 1, argv + 1);
}

void Shell::printError(int err)
{
	std::cout << "error code: " << err << std::endl;
}

std::string Shell::absPath(std::string path)
{
	if(path.size() < 1)
		return "";
	std::string tpath;
	size_t start = 0, end = 0;
	if(path == "..")
	{
		tpath = parentPath(basePath);
		return tpath;
	}
	if(path == ".")
		return basePath;
	if(path == "/")
		return "/";
	if(path[0] == '/')
	{
		start = 1;
		tpath = "";
	}
	else if(basePath == "/")
		tpath = "";
	else
		tpath = basePath;
	for(end = start; end < path.size(); ++end)
	{
		if(path[end] == '/')
		{
			if(start == end)
				tpath = "/";
			if(end - start == 2 && path[start] == '.' && path[start + 1] == '.')
				tpath = parentPath(tpath);
			else if(end - start == 1 && path[start] == '.');
			else
			{
				tpath += '/';
				tpath += path.substr(start, end - start);
			}
			start = end + 1;
		}
	}
	if(path[path.size() - 1] != '/')
	{
		tpath += '/';
		tpath += path.substr(start, path.size() - start);
	}
	return tpath;
}

std::string Shell::evrPath(std::string path)
{
	return absPath("/evr/" + path);
}

std::string Shell::decode(std::string str)
{
	if(str.size() == 0)
		return str;
	std::string rs;
	for(size_t i = 0; i + 1 < str.size(); ++i)
	{
		if(str[i] == '\\')
		{
			std::string tmp;
			switch(str[i + 1])
			{
			case 'a':
				tmp = "\a";
				break;
			case 'b':
				tmp = "\b";
				break;
			case 'f':
				tmp = "\f";
				break;
			case 'n':
				tmp = "\n";
				break;
			case 'r':
				tmp = "\r";
				break;
			case 't':
				tmp = "\t";
				break;
			case 'v':
				tmp = "\v";
				break;
			case '\\':
				tmp = "\\";
				break;
			case '\'':
				tmp = "\'";
				break;
			case '\"':
				tmp = "\"";
				break;
			case '\0':
				tmp = "\0";
				break;
			default:
				tmp = "\\" + str[i + 1];
				break;
			}
			++i;
			rs += tmp;
		}
		else
			rs += str[i];
	}
	rs += str[str.size() - 1];
	return rs;
}

std::string Shell::parentPath(std::string path)
{
	for(size_t i = path.size(); i > 0; --i)
		if(path[i - 1] == '/')
		{
			std::string tmp = path.substr(0, i - 1);
			if(tmp == "")
				tmp = "/";
			return tmp;
		}
	return "";
}

std::string Shell::getBasePath()
{
	return basePath;
}

uidsize_t Shell::getUid()
{
	return uid;
}