#ifndef _SANDBOX_H
#define _SANDBOX_H

#include "Shell.h"
#include "WordAnalyzer.h"
#include "SessionClient.h"
#include "ScriptException.h"
#include "Val.h"

#include <string>
#include <cstdint>
#include <map>
#include <vector>

class SandBox
{
	std::map<std::string, Val> idTable;
	Shell* shell;
	SessionClient* session;
	WordAnalyzer analyzer;
	Token curToken;
	bool skip;
private:
	void init(size_t argc, const std::string* argv);
	void resume();
private:
	void argList(bool exec, std::vector<std::string>& args);

	void stmtBlock(bool exec);
	Val stmt(bool exec);

	Val logicorstmt(bool exec);
	Val logicandstmt(bool exec);
	Val logicstmt(bool exec);
	Val addstmt(bool exec);
	Val mulstmt(bool exec);
	Val atomic(bool exec);

	Val idassignstmt(bool exec);

	Val ifstmt(bool exec);
	Val whilestmt(bool exec);
	Val falsestmt(bool exec);
	Val truestmt(bool exec);

	Val fstrlen(bool exec);

	Val open(bool exec);
	Val seek(bool exec);
	Val tell(bool exec);
	Val write(bool exec);
	Val read(bool exec);
	Val close(bool exec);
	Val mkdir(bool exec);
	Val mkfile(bool exec);
	Val rm(bool exec);
	Val cat(bool exec);
	Val ls(bool exec);
	Val chmod(bool exec);
	Val see(bool exec);
	Val ocp(bool exec);

	Val dopen(bool exec);
	Val dseek(bool exec);
	Val dtell(bool exec);
	Val dwrite(bool exec);
	Val dread(bool exec);
	Val dclose(bool exec);
	Val dmkdir(bool exec);
	Val dmkfile(bool exec);
	Val drm(bool exec);
	Val dcat(bool exec);
	Val dls(bool exec);
	Val dchmod(bool exec);
	Val dsee(bool exec);
	Val docp(bool exec);

	Val echo(bool exec);
public:
	SandBox(Shell* sh, const std::string& program);
	~SandBox();

	void run(size_t argc, const std::string* argv);

};

#endif