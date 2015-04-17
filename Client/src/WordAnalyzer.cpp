#include "WordAnalyzer.h"

#include "ScriptException.h"
#include "functions.h"

WordAnalyzer::WordAnalyzer(const std::string& _pgm)
	: program(_pgm), cpos(0), line(1)
{
	if(program[0] == '\n')
		++line;
}

WordAnalyzer::~WordAnalyzer()
{
}

Token WordAnalyzer::nextToken()
{
	Token token;
	skipBlank();
	if(cpos == program.size())
	{
		token.type = Token::SEOF;
		token.line = line;
		return token;
	}
	if(program[cpos] == '$')
		return nextID();
	if(program[cpos] == '\"')
		return nextString();
	if(program[cpos] == '@')
		return nextFunc();
	if(program[cpos] >= '0' && program[cpos] <= '9')
		return nextInt();
	if((program[cpos] >= 'A' && program[cpos] <= 'Z') || (program[cpos] >= 'a' && program[cpos] <= 'z'))
		return nextKey();
	return nextSign();
}

void WordAnalyzer::push()
{
	stk.push(cpos);
}

void WordAnalyzer::top()
{
	cpos = stk.top();
}

void WordAnalyzer::pop()
{
	stk.pop();
}

void WordAnalyzer::skipBlank()
{
	for(; cpos < program.size(); nextChar())
	{
		char c = program[cpos];
		if(c == ' ' || c == '\a' || c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
			continue;
		break;
	}
}

void WordAnalyzer::nextChar()
{
	++cpos;
	if(remain() > 0 && program[cpos] == '\n')
		++line;
}

void WordAnalyzer::moveTo(size_t target)
{
	while(cpos < target)
	{
		++cpos;
		if(program[cpos] == '\n')
			++line;
	}
}

bool WordAnalyzer::externStr(std::string str)
{
	if(remain() < str.size())
		return false;
	for(size_t i = 0; i < str.size(); ++i)
	{
		if(cchar() != str[i])
			return false;
		nextChar();
	}
	return true;
}

Token WordAnalyzer::nextString()
{
	Token token;
	token.type = Token::STRING;
	token.line = line;
	size_t end;
	nextChar();
	bool hasEnd = false;
	for(end = cpos; end < program.size(); ++end)
		if(program[end] == '\\')
			++end;
		else if(program[end] == '\"')
		{
			hasEnd = true;
			break;
		}
	if(!hasEnd)
		throw ScriptException(program.substr(cpos, end - cpos), line, "token error!");
	token.sval = program.substr(cpos, end - cpos);
	moveTo(end + 1);
	return token;
}

Token WordAnalyzer::nextFunc()
{
	Token token;
	token.line = line;
	nextChar();
	if(remain() < 2)
		throw ScriptException(program.substr(cpos, remain()), line, "token error!");
	std::string key;
	key += cchar();
	if(cchar() == 'c')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'l')
		{
			token.type = Token::_CLOSE;
			if(externStr("lose"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'a')
		{
			token.type = Token::_CAT;
			if(externStr("at"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'h')
		{
			token.type = Token::_CHMOD;
			if(externStr("hmod"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'e')
	{
		token.type = Token::ECHO;
		if(externStr("echo"))
			return token;
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'm')
	{
		nextChar();
		key += cchar();
		if(cchar() != 'k')
			throw ScriptException(key, line, "token error!");
		nextChar();
		key += cchar();
		if(cchar() == 'f')
		{
			token.type = Token::_MKFILE;
			if(externStr("file"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'd')
		{
			token.type = Token::_MKDIR;
			if(externStr("dir"))
				return token;
			else
				throw ScriptException(key, line, "token error!");

		}
	}
	else if(cchar() == 'r')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'e')
		{
			token.type = Token::_READ;
			if(externStr("ead"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'm')
		{
			nextChar();
			token.type = Token::_RM;
			return token;
		}
	}
	else if(cchar() == 'o')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'p')
		{
			token.type = Token::_OPEN;
			if(externStr("pen"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'c')
		{
			token.type = Token::_OCP;
			if(externStr("cp"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'l')
	{
		token.type = Token::_LS;
		if(externStr("ls"))
			return token;
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 's')
	{
		if(externStr("see"))
		{
			if(remain() > 0 && cchar() == 'k')
			{
				nextChar();
				token.type = Token::_SEEK;
			}
			else
				token.type = Token::_SEE;
			return token;
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 't')
	{
		token.type = Token::_LS;
		if(externStr("tell"))
			return token;
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'w')
	{
		token.type = Token::_LS;
		if(externStr("write"))
			return token;
		else
			throw ScriptException(key, line, "token error!");
	}
	return token;
}

Token WordAnalyzer::nextID()
{
	Token token;
	token.type = Token::ID;
	nextChar();
	std::string key;
	key += cchar();
	if((cchar() >= 'a' && cchar() <= 'z') || (cchar() >= 'A' && cchar() <= 'Z') || cchar() == '_')
	{
		token.sval = cchar();
		while(true)
		{
			nextChar();
			if((cchar() >= 'a' && cchar() <= 'z') || (cchar() >= 'A' && cchar() <= 'Z')
				|| (cchar() >= '0' && cchar() <= '9') || cchar() == '_')
				token.sval += cchar();
			else
				break;
		}
		return token;
	}
	else
		throw ScriptException(key, line, "token error!");
}

Token WordAnalyzer::nextInt()
{
	Token token;
	token.type = Token::INT;
	std::string key;
	key += cchar();
	token.sval = cchar();
	while(true)
	{
		nextChar();
		if(cchar() >= '0' && cchar() <= '9')
			token.sval += cchar();
		else
			break;
	}
	bool validate;
	token.ival = parseInt(token.sval, validate);
	if(!validate)
		throw ScriptException(token.sval, line, "token error!");
	return token;
}

Token WordAnalyzer::nextSign()
{
	Token token;
	token.line = line;
	if(cchar() == ',')
	{
		nextChar();
		token.type = Token::COMMA;
		return token;
	}
	else if(cchar() == '>')
	{
		nextChar();
		token.type = Token::SGTR;
		if(remain() < 1)
			return token;
		if(cchar() == '=')
		{
			nextChar();
			token.type = Token::SGEQ;
			return token;
		}
	}
	else if(cchar() == '<')
	{
		nextChar();
		token.type = Token::SLSS;
		if(remain() < 1)
			return token;
		if(cchar() == '=')
		{
			nextChar();
			token.type = Token::SLEQ;
			return token;
		}
	}
	else if(cchar() == '=')
	{
		nextChar();
		token.type = Token::SASSIGN;
		if(remain() < 1)
			return token;
		if(cchar() == '=')
		{
			nextChar();
			token.type = Token::SEQU;
			return token;
		}
	}
	else if(cchar() == ';')
	{
		nextChar();
		token.type = Token::SSEMICOLON;
		return token;
	}
	else if(cchar() == '+')
	{
		nextChar();
		token.type = Token::SADD;
		return token;
	}
	else if(cchar() == '-')
	{
		nextChar();
		token.type = Token::SSUB;
		return token;
	}
	else if(cchar() == '*')
	{
		nextChar();
		token.type = Token::SMUL;
		return token;
	}
	else if(cchar() == '/')
	{
		nextChar();
		token.type = Token::SDIV;
		return token;
	}
	else if(cchar() == '(')
	{
		nextChar();
		token.type = Token::SLP;
		return token;
	}
	else if(cchar() == ')')
	{
		nextChar();
		token.type = Token::SRP;
		return token;
	}
	else if(cchar() == '!')
	{
		nextChar();
		token.type = Token::SNOT;
		return token;
	}
	else if(cchar() == '|')
	{
		nextChar();
		token.type = Token::SOR;
		return token;
	}
	else if(cchar() == '&')
	{
		nextChar();
		token.type = Token::SAND;
		return token;
	}
	else
		throw ScriptException(std::string() + cchar(), line, "token error!");
	return token;
}

Token WordAnalyzer::nextKey()
{
	Token token;
	token.line = line;
	if(remain() < 2)
		throw ScriptException(program.substr(cpos, remain()), line, "token error!");
	std::string key;
	key += cchar();
	if(cchar() == 'c')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'l')
		{
			token.type = Token::CLOSE;
			if(externStr("lose"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'a')
		{
			token.type = Token::CAT;
			if(externStr("at"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'h')
		{
			token.type = Token::CHMOD;
			if(externStr("hmod"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'e')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'n')
		{
			token.type = Token::SEND;
			if(externStr("nd"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'l')
		{
			token.type = Token::ELSE;
			if(externStr("lse"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'f')
	{
		token.type = Token::CFALSE;
		if(externStr("false"))
			return token;
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'i')
	{
		token.type = Token::IF;
		if(externStr("if"))
			return token;
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'm')
	{
		nextChar();
		key += cchar();
		if(cchar() != 'k')
			throw ScriptException(key, line, "token error!");
		nextChar();
		key += cchar();
		if(cchar() == 'f')
		{
			token.type = Token::MKFILE;
			if(externStr("file"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'd')
		{
			token.type = Token::MKDIR;
			if(externStr("dir"))
				return token;
			else
				throw ScriptException(key, line, "token error!");

		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'r')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'e')
		{
			token.type = Token::READ;
			if(externStr("ead"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'm')
		{
			nextChar();
			token.type = Token::RM;
			return token;
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'o')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'p')
		{
			token.type = Token::OPEN;
			if(externStr("pen"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'c')
		{
			token.type = Token::OCP;
			if(externStr("cp"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 'l')
	{
		token.type = Token::LS;
		if(externStr("ls"))
			return token;
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 's')
	{
		nextChar();
		key += cchar();
		if(cchar() == 't')
		{
			token.type = Token::STRLEN;
			if(externStr("trlen"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'e')
		{
			if(externStr("ee"))
			{
				if(remain() > 0 && cchar() == 'k')
				{
					nextChar();
					token.type = Token::SEEK;
				}
				else
					token.type = Token::SEE;
				return token;
			}
			else
				throw ScriptException(key, line, "token error!");
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	else if(cchar() == 't')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'e')
		{
			token.type = Token::TELL;
			if(externStr("ell"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'r')
		{
			token.type = Token::CTRUE;
			if(externStr("rue"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else
			throw ScriptException(key, line, "token error!");
		
	}
	else if(cchar() == 'w')
	{
		nextChar();
		key += cchar();
		if(cchar() == 'r')
		{
			token.type = Token::WRITE;
			if(externStr("rite"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else if(cchar() == 'h')
		{
			token.type = Token::WHILE;
			if(externStr("hile"))
				return token;
			else
				throw ScriptException(key, line, "token error!");
		}
		else
			throw ScriptException(key, line, "token error!");
	}
	return token;
}
