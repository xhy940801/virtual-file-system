#include "SandBox.h"
#include "ScriptException.h"
#include "Guard.h"
#include "commondef.h"

#include <iostream>

SandBox::SandBox(Shell* sh, const std::string& program)
	: shell(sh), session(shell->session), analyzer(program), skip(false)
{
}

SandBox::~SandBox()
{
}

void SandBox::run(size_t argc, const std::string* argv)
{
	try
	{
		init(argc, argv);
		stmtBlock(true);
		if(curToken.type != Token::SEOF)
			shell->printShellError(curToken.line, "expect end");
	}
	catch (TypeIncompatibleException e)
	{
		shell->printShellError(e.getLine(), "type  transfer fail -> oldtype: " + e.getOldType() + " newType: " + e.getNewType());
	}
	catch (ScriptException e)
	{
		shell->printShellError(e.getLine(), "key: " + e.getKey() + " " + e.getInfo());
	}
}

void SandBox::init(size_t argc, const std::string* argv)
{
	for(size_t i = 0; i < argc; ++i)
		idTable[std::string("_") + parseString(i)] = Val(argv[i]);
	curToken = analyzer.nextToken();
}

void SandBox::resume()
{
	curToken = analyzer.nextToken();
}

void SandBox::argList(bool exec, std::vector<std::string>& args)
{
	size_t curLine = curToken.line;
	Val rs = stmt(exec);
	if(exec)
		args.push_back(rs.strVal(curLine, exec).sval);
	if(curToken.type == Token::COMMA)
	{
		resume();
		argList(exec, args);
	}
}

void SandBox::stmtBlock(bool exec)
{
	stmt(exec);
	if(curToken.type == Token::SSEMICOLON)
	{
		resume();
		stmtBlock(exec);
	}
}

Val SandBox::stmt(bool exec)
{
	switch(curToken.type)
	{
	case Token::IF:
		return ifstmt(exec);
	case Token::WHILE:
		return whilestmt(exec);
	case Token::CFALSE:
		return falsestmt(exec);
	case Token::CTRUE:
		return truestmt(exec);
	case Token::STRLEN:
		return fstrlen(exec);
	case Token::OPEN:
		return open(exec);
	case Token::SEEK:
		return seek(exec);
	case Token::TELL:
		return tell(exec);
	case Token::WRITE:
		return write(exec);
	case Token::READ:
		return read(exec);
	case Token::CLOSE:
		return close(exec);
	case Token::MKDIR:
		return mkdir(exec);
	case Token::MKFILE:
		return mkfile(exec);
	case Token::RM:
		return rm(exec);
	case Token::CAT:
		return cat(exec);
	case Token::LS:
		return ls(exec);
	case Token::CHMOD:
		return chmod(exec);
	case Token::SEE:
		return see(exec);
	case Token::OCP:
		return ocp(exec);
	case Token::_OPEN:
		return dopen(exec);
	case Token::_SEEK:
		return dseek(exec);
	case Token::_TELL:
		return dtell(exec);
	case Token::_WRITE:
		return dwrite(exec);
	case Token::_READ:
		return dread(exec);
	case Token::_CLOSE:
		return dclose(exec);
	case Token::_MKDIR:
		return dmkdir(exec);
	case Token::_MKFILE:
		return dmkfile(exec);
	case Token::_RM:
		return drm(exec);
	case Token::_CAT:
		return dcat(exec);
	case Token::_LS:
		return dls(exec);
	case Token::_CHMOD:
		return dchmod(exec);
	case Token::_SEE:
		return dsee(exec);
	case Token::_OCP:
		return docp(exec);
	case Token::ECHO:
		return echo(exec);
	default:
		return logicorstmt(exec);
	}
}

Val SandBox::logicorstmt(bool exec)
{
	size_t curLine = curToken.line;
	Val rs1 = logicandstmt(exec);
	if(curToken.type == Token::SOR)
	{
		resume();
		Val rs2 = logicorstmt(exec);
		if(exec)
			return rs1.intVal(curLine, exec).ival || rs2.intVal(curLine, exec).ival;
		return Val();
	}
	return rs1;
}

Val SandBox::logicandstmt(bool exec)
{
	size_t curLine = curToken.line;
	Val rs1 = logicstmt(exec);
	if(curToken.type == Token::SAND)
	{
		resume();
		Val rs2 = logicandstmt(exec);
		if(exec)
			return rs1.intVal(curLine, exec).ival && rs2.intVal(curLine, exec).ival;
		return Val();
	}
	return rs1;
}

Val SandBox::logicstmt(bool exec)
{
	size_t curLine = curToken.line;
	Val rs1 = addstmt(exec);
	if(curToken.type == Token::SGTR || curToken.type == Token::SGEQ || curToken.type == Token::SEQU 
		|| curToken.type == Token::SLEQ || curToken.type == Token::SLSS)
	{	
		Token::Type tp = curToken.type;
		resume();
		Val rs2 = logicstmt(exec);
		if(exec)
		{
			if(tp == Token::SGTR)
				return Val((int64_t)(rs1.intVal(curLine, exec).ival > rs2.intVal(curLine, exec).ival));
			else if(tp == Token::SGEQ)
				return Val((int64_t)(rs1.intVal(curLine, exec).ival >= rs2.intVal(curLine, exec).ival));
			else if(tp == Token::SEQU)
			{
				if(rs1.type != rs2.type)
					return Val((int64_t)(rs1.intVal(curLine, exec).ival >= rs2.intVal(curLine, exec).ival));
				if(rs1.type == Val::STRING)
					return Val((int64_t)(rs1.sval == rs2.sval));
				else if(rs1.type == Val::INT)
					return Val((int64_t)(rs1.ival == rs2.ival));
				else
					return Val(0);
			}
			else if(tp == Token::SLEQ)
				return Val((int64_t)(rs1.intVal(curLine, exec).ival <= rs2.intVal(curLine, exec).ival));
			else if(tp == Token::SLSS)
				return Val((int64_t)(rs1.intVal(curLine, exec).ival < rs2.intVal(curLine, exec).ival));
		}
		return Val();
	}
	return rs1;
}

Val SandBox::addstmt(bool exec)
{
	size_t curLine = curToken.line;
	Val rs1 = mulstmt(exec);
	if(curToken.type == Token::SADD)
	{
		resume();
		Val rs2 = addstmt(exec);
		if(exec)
			return rs1.intVal(curLine, exec).ival + rs2.intVal(curLine, exec).ival;
		return Val();
	}
	else if(curToken.type == Token::SSUB)
	{
		resume();
		Val rs2 = addstmt(exec);
		if(exec)
			return rs1.intVal(curLine, exec).ival - rs2.intVal(curLine, exec).ival;
		return Val();
	}
	return rs1;
}

Val SandBox::mulstmt(bool exec)
{
	size_t curLine = curToken.line;
	Val rs1 = atomic(exec);
	if(curToken.type == Token::SMUL)
	{
		resume();
		Val rs2 = mulstmt(exec);
		if(exec)
			return rs1.intVal(curLine, exec).ival * rs2.intVal(curLine, exec).ival;
		return Val();
	}
	else if(curToken.type == Token::SDIV)
	{
		resume();
		Val rs2 = mulstmt(exec);
		if(exec)
			return rs1.intVal(curLine, exec).ival / rs2.intVal(curLine, exec).ival;
		return Val();
	}
	return rs1;
}

Val SandBox::atomic(bool exec)
{
	size_t curLine = curToken.line;
	Val rs;
	if(curToken.type == Token::SLP)
	{
		resume();
		rs = stmt(exec);
		if(curToken.type != Token::SRP)
			throw ScriptException(curToken.sval, curLine, "unexpect the token, expect \")\"");
		resume();
		return rs;
	}
	else if(curToken.type == Token::STRING)
	{
		rs = Val(curToken.sval);
		resume();
		return rs;
	}
	else if(curToken.type == Token::INT)
	{
		rs = Val(curToken.ival);
		resume();
		return rs;
	}
	else if(curToken.type == Token::CFALSE)
		return falsestmt(exec);
	else if(curToken.type == Token::CTRUE)
		return truestmt(exec);
	else if(curToken.type == Token::ID)
		return idassignstmt(exec);
	else if(curToken.type == Token::SNOT)
	{
		resume();
		rs = atomic(exec);
		if(exec)
			rs = Val((int)(!rs.intVal(curLine, exec).ival));
		return rs;
	}
	else
		throw ScriptException(curToken.sval, curLine, "unexpect the token");
}

Val SandBox::idassignstmt(bool exec)
{
	std::string idname = curToken.sval;
	resume();
	if(curToken.type == Token::SASSIGN)
	{
		resume();
		Val rs = stmt(exec);
		if(exec)
			idTable[idname] = rs;
	}
	return idTable[idname];
}

Val SandBox::ifstmt(bool exec)
{
	int curLine = curToken.line;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curLine, "unexpect the token, expect \"(\"");
	resume();
	Val rs = stmt(exec);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curLine, "unexpect the token, expect \")\"");
	resume();
	bool mst;
	if(exec)
	{
		if(rs.type == Val::INT)
			mst = (bool) rs.ival;
		else if(rs.type == Val::STRING)
			mst = rs.sval != "";
		else
			mst = false;
		stmtBlock(mst);
	}
	else
		stmtBlock(false);
	if(curToken.type == Token::ELSE)
	{
		resume();
		if(exec)
			stmtBlock(!mst);
	}
	if(curToken.type != Token::SEND)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"end\"");
	resume();
	return Val();
}

Val SandBox::whilestmt(bool exec)
{
	while(true)
	{
		analyzer.push();
		int curLine = curToken.line;
		resume();
		if(curToken.type != Token::SLP)
			throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
		resume();
		Val rs = stmt(exec).intVal(curLine, exec);
		if(curToken.type != Token::SRP)
			throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
		resume();
		if(exec)
		{
			if(rs.ival)
				stmtBlock(true);
			else
				stmtBlock(false);
		}
		else
			stmtBlock(false);
		if(curToken.type != Token::SEND)
			throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"end\"");
		resume();
		if(exec && rs.ival)
		{
			analyzer.top();
			analyzer.pop();
		}
		else
		{
			analyzer.pop();
			break;
		}
	}
	return Val();
}

Val SandBox::falsestmt(bool exec)
{
	resume();
	return Val(0);
}

Val SandBox::truestmt(bool exec)
{
	resume();
	return Val(1);
}

Val SandBox::fstrlen(bool exec)
{
	size_t curLine = curToken.line;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	Val rs = Val(stmt(exec).strVal(curLine, exec).sval.size());
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	return rs;
}

Val SandBox::open(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 1)
		return Val(0);
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
	int fd = session->openFile(shell->absPath(argv[0]).c_str(), type);
	return Val(fd);
}

Val SandBox::seek(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 2)
		return Val(-1);
	bool parseSuc;
	//获取文件描述符
	int fd = parseInt(argv[0], parseSuc);
	if(!parseSuc)
		return Val(-1);
	//获取偏移量
	int offset = parseInt(argv[1], parseSuc);
	if(!parseSuc)
		return Val(-1);
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
	}

	int old = session->seekFile(fd, offset, pos);		//调用seek
	if(old < 0)
		return Val(-1);
	return Val(old);
}

Val SandBox::tell(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 1)
		return Val(-1);
	bool parseSuc;
	//获取文件描述符
	int fd = parseInt(argv[0], parseSuc);
	if(!parseSuc)
		return Val(-1);

	int fpos = session->tellFile(fd);		//调用seek
	if(fpos < 0)
		return Val(-1);
	return Val(fpos);
}

Val SandBox::write(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 2)
		return Val(-1);
	bool parseSuc;
	//获取文件描述符
	int fd = parseInt(argv[0], parseSuc);
	if(!parseSuc)
		return Val(-1);
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
			return Val(-1);
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
		return Val(-1);
	return Val(result);
}

Val SandBox::read(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	Val rsss = Val("");

	if(argc < 2)
		return Val();
	bool parseSuc;
	//获取文件描述符
	int fd = parseInt(argv[0], parseSuc);
	if(!parseSuc)
		return Val();
	int len = parseInt(argv[1], parseSuc);
	if(!parseSuc || len <= 0)
		return Val();
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
		return Val();
	if(result > 0)
	{
		buf.get()[result] = '\0';
		int64_t intRs;
		if(type != Types::STRING)
		{
			intRs = parseInt(std::string(buf.get()), parseSuc);
			if(!parseSuc)
				return Val();
		}
		int8_t int8rs = intRs;
		switch (type)
		{
		case Types::STRING:
			rsss.sval += buf.get();
			break;
		case Types::INT8:
			rsss.sval += parseString((int) int8rs);
			break;
		case Types::INT16:
			rsss.sval += parseString((int16_t) intRs);
			break;
		case Types::INT32:
			rsss.sval += parseString((int32_t) intRs);
			break;
		case Types::INT64:
			rsss.sval += parseString((int64_t) intRs);
			break;
		}
	}
	return Val(result);
}

Val SandBox::close(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 1)
		return Val(0);
	bool parseSuc;
	//获取文件描述符
	int fd = parseInt(argv[0], parseSuc);
	if(!parseSuc)
		return Val(0);

	bool rs = session->closeFile(fd);		//调用closeFile
	if(!rs)
		return Val(0);
	return Val(1);
}

Val SandBox::mkdir(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 1)
		return Val(0);
	std::string abspth, parent, name;
	abspth = shell->absPath(argv[0]);
	parent = shell->parentPath(abspth);
	if(parent == "/")
		name = abspth.substr(1, abspth.size() - 1);
	else
		name = abspth.substr(parent.size() + 1, abspth.size() - (parent.size() + 1));
	if(parent == "" || name == "")
		return Val(0);
	if(!session->createFolder(parent.c_str(), name.c_str()))
		return Val(0);
	return Val(1);
}

Val SandBox::mkfile(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 1)
		return Val(0);
	std::string abspth, parent, name;
	abspth = shell->absPath(argv[0]);
	parent = shell->parentPath(abspth);
	if(parent == "/")
		name = abspth.substr(1, abspth.size() - 1);
	else
		name = abspth.substr(parent.size() + 1, abspth.size() - (parent.size() + 1));
	if(parent == "" || name == "")
		return Val(0);
	if(!session->createFile(parent.c_str(), name.c_str()))
		return Val(0);
	return Val(1);
}

Val SandBox::rm(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 1)
		return Val(0);
	if(!session->deleteItem(shell->absPath(argv[0]).c_str()))
		return Val(0);
	return Val(1);
}

Val SandBox::cat(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	Val rsss = Val("");

	class _FDGuard
	{
		SessionClient* session;
	public:
		_FDGuard(SessionClient* _ss) : session(_ss) {}
		void operator () (int fd) { session->closeFile(fd); }
	};
	if(argc < 1)
		return Val();
	const size_t BUFSIZE = 64;
	std::unique_ptr<char> buf(new char[BUFSIZE + 1]);
	int fd = session->openFile(shell->absPath(argv[0]).c_str(), OPTYPE_READ);
	if(fd <= 0)
		return Val();
	Guard<int, _FDGuard> gd(fd, _FDGuard(session));
	size_t start = 0;
	while(true)
	{
		int len = session->readFile(fd, buf.get(), BUFSIZE);
		if(len < 0)
			return Val();
		else if(len == 0)
			break;
		else
		{
			start += len;
			buf.get()[len] = '\0';
			rsss.sval += buf.get();
		}
	}
	return rsss;
}

Val SandBox::ls(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	Val rsss = Val("");

	std::string path;
	if(argc < 1)
		path = shell->basePath;
	else
		path = shell->absPath(argv[0]);
	std::vector<std::string> files;
	if(!session->readFolder(path.c_str(), files))
		return Val();
	for(size_t i = 0; i < files.size(); ++i)
		rsss.sval += files[i] + " ";
	return rsss;
}

Val SandBox::chmod(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	if(argc < 2)
		return Val(0);
	bool parseSuc;
	int mod = parseInt(argv[1], parseSuc);
	if(!parseSuc || mod / 10 > 7 || mod % 10 > 7)
		return Val(0);
	fsign sign = mod / 10 + (mod % 10) * 8;
	if(!session->changeModel(shell->absPath(argv[0]).c_str(), sign))
		return Val(0);
	return Val(1);
}

Val SandBox::see(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	Val rsss = Val("");

	if(argc < 1)
		return Val();
	SafeInfo sfInfo;
	if(!session->getItemSafeInfo(shell->absPath(argv[0]).c_str(), &sfInfo))
		return Val();
	if(sfInfo.sign & FL_TYPE_FILE)
		rsss.sval += "file: " + shell->absPath(argv[0]);
	else
		rsss.sval += "folder: " + shell->absPath(argv[0]);
	rsss.sval += " mode: " + parseString(sfInfo.sign % 8) + parseString((sfInfo.sign / 8) % 8);
	rsss.sval += " owner: " + parseString(sfInfo.uid);
	rsss.sval += " created: " + parseString(sfInfo.created);
	rsss.sval += " modified: " + parseString(sfInfo.modified);
	return rsss;
}

Val SandBox::ocp(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

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
		return Val(0);

	FILE* ofd = fopen(argv[0].c_str(), "r");
	if(ofd == nullptr)
		return Val(0);
	Guard<FILE*, _OFDGuard> ogd(ofd, _OFDGuard());
	std::string abspth, parent, name;
	abspth = shell->absPath(argv[1]);
	parent = shell->parentPath(abspth);
	if(parent == "/")
		name = abspth.substr(1, abspth.size() - 1);
	else
		name = abspth.substr(parent.size() + 1, abspth.size() - (parent.size() + 1));
	if(parent == "" || name == "")
		return Val(0);
	if(!session->createFile(parent.c_str(), name.c_str()))
		return Val(0);
	int fd = session->openFile(shell->absPath(argv[1]).c_str(), OPTYPE_WRITE);
	if(fd <= 0)
		return Val(0);
	Guard<int, _FDGuard> gd(fd, _FDGuard(session));
	size_t start = 0;
	const size_t BUFSIZE = 64;
	std::unique_ptr<char> buf(new char[BUFSIZE]);
	while(true)
	{
		int len = fread(buf.get() + start, sizeof(char), BUFSIZE - start, ofd);
		if(len < 0)
			return Val(0);
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
					return Val(0);
				if(cl == 0)
					return Val(0);
				has += cl;
			}
		}
	}
	return Val(1);
}

Val SandBox::dopen(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->open(argc, argv);
	return Val();
}

Val SandBox::dseek(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->seek(argc, argv);
	return Val();
}

Val SandBox::dtell(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->tell(argc, argv);
	return Val();
}

Val SandBox::dwrite(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->write(argc, argv);
	return Val();
}

Val SandBox::dread(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->read(argc, argv);
	return Val();
}

Val SandBox::dclose(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->close(argc, argv);
	return Val();
}

Val SandBox::dmkdir(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->mkdir(argc, argv);
	return Val();
}

Val SandBox::dmkfile(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->mkfile(argc, argv);
	return Val();
}

Val SandBox::drm(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->rm(argc, argv);
	return Val();
}

Val SandBox::dcat(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->cat(argc, argv);
	return Val();
}

Val SandBox::dls(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->ls(argc, argv);
	return Val();
}

Val SandBox::dchmod(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->chmod(argc, argv);
	return Val();
}

Val SandBox::dsee(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->see(argc, argv);
	return Val();
}

Val SandBox::docp(bool exec)
{
	std::vector<std::string> args;
	resume();
	if(curToken.type != Token::SLP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \"(\"");
	resume();
	argList(exec, args);
	if(curToken.type != Token::SRP)
		throw ScriptException(curToken.sval, curToken.line, "unexpect the token, expect \")\"");
	resume();
	if(!exec)
		return Val();

	size_t argc = args.size();
	const std::string* argv = args.data();

	shell->ocp(argc, argv);
	return Val();
}

Val SandBox::echo(bool exec)
{
	resume();
	Val rs = stmt(exec);
	if(exec)
	{
		if(rs.type == Val::NONE)
			std::cout << "NULL" << std::endl;
		if(rs.type == Val::INT)
			std::cout << rs.ival << std::endl;
		else
			std::cout << "\"" << decode(rs.sval) << "\"" << std::endl;
	}
	return Val();
}