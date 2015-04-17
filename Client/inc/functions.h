#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H

#include <string>
#include <cstdint>

std::string trim(std::string str);							//删除字符串首位空格
int64_t parseInt(std::string str, bool& validate);			//把字符串转为int, 如果转换成功validate被赋为true,否则false
std::string parseString(int64_t num);						//把int转为字符串
std::string decode(std::string str);						//把参数中的转义字符去掉

#endif
