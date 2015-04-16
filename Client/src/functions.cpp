#include "functions.h"

std::string trim(std::string str)
{
	size_t start = 0, end = str.size();
	while(start < str.size())
	{
		char c = str[start];
		if(c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\a' || c == '\b' || c == '\v' || c == '\f' || c == '\0')
			++start;
		else
			break;
	}
	while(end > 0)
	{
		char c = str[end - 1];
		if(c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\a' || c == '\b' || c == '\v' || c == '\f' || c == '\0')
			--end;
		else
			break;
	}
	if(start < end)
		return str.substr(start, end - start);
	return std::string("");
}

int64_t parseInt(std::string str, bool& validate)
{
	str = trim(str);
	if(str.size() == 0)
	{
		validate = false;
		return 0;
	}
	int64_t rs = 0;
	for(size_t i = 0; i < str.size(); ++i)
	{
		if(str[i] < '0' || str[i] > '9')
		{
			validate = false;
			return 0;
		}
		rs *= 10;
		rs += str[i] - '0';
	}
	validate = true;
	return rs;
}