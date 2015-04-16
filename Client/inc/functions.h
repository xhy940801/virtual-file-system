#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

#include <string>
#include <cstdint>

std::string trim(std::string str);							//删除字符串首位空格
int64_t parseInt(std::string str, bool& validate);			//把字符串转为int, 如果转换成功validate被赋为true,否则false

#endif
