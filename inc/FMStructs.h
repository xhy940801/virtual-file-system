#ifndef _FMSTRUCTS_H
#define _FMSTRUCTS_H

#include "types.h"
#include <ctime>

/**
 * sign:
 * 111 user 111 other 1 write-lock 0 is floder
 * 
 */

 #define PM_U_READ 0x01
 #define PM_U_WRITE 0x02
 #define PM_U_EXECUTE 0x04

 #define PM_O_READ 0x08
 #define PM_O_WRITE 0x10
 #define PM_O_EXECUTE 0x20

 #define FL_WRITE_MTXLOCK 0x40
 #define FL_TYPE_FILE 0x80

#pragma pack(push, 1)

struct SafeInfo
{
	fsign sign;
	ctsize_t mutexRead, shareRead;
	ctsize_t shareWrite;
	uidsize_t uid;
};

struct FreeNode
{
	size_t size;
	cpos_t next;
};

struct FSHeader
{
	size_t capacity;
	size_t remain;
	cpos_t freeChunkHead;
};

struct FileNode
{
	char name[16];
	cpos_t pos;
};

struct AppendFloderNode
{
	size_t nodeSize;
	cpos_t next;
};

struct FloderInfo
{
	size_t nodeSize;
	time_t created;
	time_t modified;
	cpos_t parent;
	cpos_t next;
};

struct Folder
{
	cpos_t cur;
	SafeInfo* sfInfo;
	FloderInfo* info;
	FileNode* nodes;
};

/**
 * 1 or 0 is sign the append-inode or data
 */
struct CKInfo
{
	cksize_t num;
	cpos_t pos;
};

struct INInfo
{
	size_t nodeSize;
	time_t created;
	time_t modified;
	size_t fileSize;
	cpos_t parent;
};

struct IndexNode
{
	cpos_t cur;
	SafeInfo* sfInfo;
	INInfo* info;
	size_t nodeSize;
	CKInfo* nodes;
};

struct AppendINode
{
	size_t nodeSize;
};

#pragma pack(pop)

struct FDMInfo
{
	SafeInfo sfInfo;
	cpos_t hdpos;
	size_t openCount;
	size_t fileSize;
	size_t cklistSize;
	CKInfo* cklist;
};

struct FDSInfo
{
	fdtype_t model;
	int fd;
	size_t ckpos;
	size_t rlpos;
	size_t abpos;
};

#endif