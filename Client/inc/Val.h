#ifndef _VAL_H
#define _VAL_H

#include "ScriptException.h"
#include "functions.h"

#include <string>
#include <cstdint>
//脚本的变量
struct Val
{
	enum Type {STRING, INT, NONE};
	Type type;
	std::string sval;
	int64_t ival;
	Val() : type(Val::NONE) {}
	Val(std::string val) : type(Val::STRING), sval(val) {}
	Val(int64_t val) : type(Val::INT), ival(val) {}

	Val intVal(size_t line, bool exec)
	{
		if(!exec)
			return *this;
		std::string oldType, newType;
		newType = "int";
		if(type == Val::STRING)
		{
			oldType = "string";
			bool validate;
			int64_t rs = parseInt(sval, validate);
			if(!validate)
				throw TypeIncompatibleException(line, oldType, newType);
			return Val(rs);
		}
		else if(type == Val::INT)
			return *this;
		else
		{
			oldType = "NONE";
			throw TypeIncompatibleException(line, oldType, newType);
		}
	}

	Val strVal(size_t line, bool exec)
	{
		if(!exec)
			return *this;
		if(type == Val::STRING)
			return *this;
		else if(type == Val::INT)
			return Val(parseString(ival));
		else
			throw TypeIncompatibleException(line, "none", "string");
	}
};

#endif