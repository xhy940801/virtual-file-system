#ifndef _CHUNKHELPER_H
#define _CHUNKHELPER_H

#include "FMStructs.h"
#include <cstring>
#include <cstddef>

class ChunkHelper
{
	char* buffer;
public:
	ChunkHelper(const char* buf, size_t chunkSize);
	~ChunkHelper();

	char* getBuffer();

	SafeInfo* getSafeInfo();
	INInfo* getINInfo();
	size_t getCKLen();
	CKInfo* getCKInfo();

	size_t getAPCKLen();
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