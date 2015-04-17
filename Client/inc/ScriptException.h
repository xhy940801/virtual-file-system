#ifndef _SCRIPTEXCEPTION_H
#define _SCRIPTEXCEPTION_H

#include <string>

#include <stdexcept>

class ScriptException
{
	std::string key;
	size_t line;
	std::string info;
public:
	ScriptException(std::string _k, size_t _l, std::string _i)
		: key(_k), line(_l), info(_i)
	{
	}

	~ScriptException()
	{
	}

	std::string getKey() { return key; }
	size_t getLine() { return line; }
	std::string getInfo() { return info; }
};

class TypeIncompatibleException
{
	size_t line;
	std::string oldType;
	std::string newType;
public:
	TypeIncompatibleException(size_t _l, std::string _o, std::string _n) : line(_l), oldType(_o), newType(_n) {}
	~TypeIncompatibleException() {}

	size_t getLine() { return line; }
	std::string getOldType() { return oldType; }
	std::string getNewType() { return newType; }
};

#endif