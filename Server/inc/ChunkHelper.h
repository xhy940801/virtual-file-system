#ifndef _CHUNKHELPER_H
#define _CHUNKHELPER_H

#include "FMStructs.h"
#include <cstring>
#include <cstddef>

//类似unique_ptr,用来自动管理char数组,使其能自动释放,同时,提供一些方便的函数来获取buf中的信息
class ChunkHelper
{
	char* buffer;
public:
	ChunkHelper(const char* buf, size_t chunkSize);
	~ChunkHelper();

	char* getBuffer();

	//在buf中提取SafeInfo
	SafeInfo* getSafeInfo();
	//在buf中提取INInfo
	INInfo* getINInfo();
	//提取长度
	size_t getCKLen();
	//提取节点信息
	CKInfo* getCKInfo();

	//(以扩展块的模式解析)获取扩展i节点的长度
	size_t getAPCKLen();
	//(以扩展块的模式解析)获取扩展i节点信息
	CKInfo* getAPCKInfo();

	FloderInfo* getFloderInfo();
};

inline ChunkHelper::ChunkHelper(const char* buf, size_t chunkSize)
{
	buffer = new char[chunkSize];
	memcpy(buffer, buf, chunkSize);
}

inline ChunkHelper::~ChunkHelper()
{
	delete buffer;
}

inline char* ChunkHelper::getBuffer()
{
	return buffer;
}

inline SafeInfo* ChunkHelper::getSafeInfo()
{
	return (SafeInfo*) buffer;
}

inline INInfo* ChunkHelper::getINInfo()
{
	return (INInfo*) (buffer + sizeof(SafeInfo));
}

inline size_t ChunkHelper::getCKLen()
{
	return * (size_t*) (buffer + sizeof(SafeInfo) + sizeof(INInfo));
}

inline CKInfo* ChunkHelper::getCKInfo()
{
	return (CKInfo*) (buffer + sizeof(SafeInfo) + sizeof(INInfo) + sizeof(size_t));
}

inline FloderInfo* ChunkHelper::getFloderInfo()
{
	return (FloderInfo*) (buffer + sizeof(SafeInfo));
}

inline size_t ChunkHelper::getAPCKLen()
{
	return * (size_t*) buffer;
}

inline CKInfo* ChunkHelper::getAPCKInfo()
{
	return (CKInfo*) (buffer + sizeof(size_t));
}

#endif