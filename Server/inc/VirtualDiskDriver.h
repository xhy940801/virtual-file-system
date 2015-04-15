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

	bool setChunk(cpos_t pos, const void* buf);
	bool getChunk(cpos_t pos, void* buf);

	bool setChunk(cpos_t pos, const void* buf, size_t size);
	bool getChunk(cpos_t pos, void* buf, size_t size);

	VirtualDiskDriver(FILE* f, size_t ckSize, size_t tckNumber);
	~VirtualDiskDriver();
};

#endif