#ifndef _VIRTUALDISKDRIVER_H
#define _VIRTUALDISKDRIVER_H

#include "types.h"
#include <cstdio>

class VirtualDiskDriver
{
	FILE* file;
	size_t chunkSize;
	size_t totalChunkNumber;
public:
	size_t getChunkSize();
	size_t getTotalChunkNumber();
	//设置块k;读取块k
	bool setChunk(cpos_t pos, const void* buf);
	bool getChunk(cpos_t pos, void* buf);
	//同上,不过允许传入的buf小于一个chunkSize
	bool setChunk(cpos_t pos, const void* buf, size_t size);
	bool getChunk(cpos_t pos, void* buf, size_t size);

	VirtualDiskDriver(FILE* f, size_t ckSize, size_t tckNumber);
	~VirtualDiskDriver();
};

#endif