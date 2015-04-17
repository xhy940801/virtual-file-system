#ifndef _WORDANALYZER_H
#define _WORDANALYZER_H

#include <string>
#include <cstdint>
#include <stack>

struct Token
{
	enum Type 
	{
		ID, OPEN, SEEK, TELL, WRITE, READ, CLOSE, MKDIR, MKFILE, RM, CAT, LS, CHMOD, SEE, STRLEN, OCP,
		_OPEN, _SEEK, _TELL, _WRITE, _READ, _CLOSE, _MKDIR, _MKFILE, _RM, _CAT, _LS, _CHMOD, _SEE, _OCP,
		SGTR, SLEQ, SGEQ, SLSS, SEQU, SSEMICOLON, SASSIGN, SADD, SSUB, SMUL, SDIV, SLP, SRP, SOR, SAND, SNOT, COMMA,
		STRING, INT, CFALSE, CTRUE, SEND, SEOF, IF, ELSE, WHILE, ECHO
	};
	Type type;
	std::string sval;
	size_t line;
	int64_t ival;
};

class WordAnalyzer
{
	const std::string& program;
	size_t cpos;
	size_t line;
	std::stack<size_t> stk;
private:
	size_t remain() { return program.size() - cpos; }
	char cchar() { return program[cpos]; }
private:
	void skipBlank();
	void nextChar();
	void moveTo(size_t target);
	bool externStr(std::string str);
	Token nextString();
	Token nextFunc();
	Token nextID();
	Token nextInt();
	Token nextSign();
	Token nextKey();
public:
	WordAnalyzer(const std::string& _pgm);
	~WordAnalyzer();
	
	Token nextToken();
	void push();
	void top();
	void pop();
};

#endif