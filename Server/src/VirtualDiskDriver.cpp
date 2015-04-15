#include "VirtualDiskDriver.h"

VirtualDiskDriver::VirtualDiskDriver(FILE* f, size_t ckSize, size_t tckNumber)
	: file(f), chunkSize(ckSize), totalChunkNumber(tckNumber)
{
}

VirtualDiskDriver::~VirtualDiskDriver()
{
	fflush(file);
}

size_t VirtualDiskDriver::getChunkSize()
{
	return chunkSize;
}

size_t VirtualDiskDriver::getTotalChunkNumber()
{
	return totalChunkNumber;
}

bool VirtualDiskDriver::setChunk(cpos_t pos, const void* buf)
{
	return setChunk(pos, buf, chunkSize);
}

bool VirtualDiskDriver::getChunk(cpos_t pos, void* buf)
{
	return getChunk(pos, buf, chunkSize);
}

bool VirtualDiskDriver::setChunk(cpos_t pos, const void* buf, size_t size)
{
	return fseek(file, pos * chunkSize, SEEK_SET) == 0
		&& fwrite(buf, size, 1, file) == 1;
}

bool VirtualDiskDriver::getChunk(cpos_t pos, void* buf, size_t size)
{
	return fseek(file, pos * chunkSize, SEEK_SET) == 0
		&& fread(buf, size, 1, file) == 1;
}