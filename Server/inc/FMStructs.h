#ifndef _FMSTRUCTS_H
#define _FMSTRUCTS_H

#include "types.h"
#include <ctime>
#include <thread>

/**
 * sign:
 * 111 user 111 other 1 write-lock 0 is floder
 * 
 */
//所有者权限模式
 #define PM_U_READ 0x01				//可读
 #define PM_U_WRITE 0x02			//可写
 #define PM_U_EXECUTE 0x04			//可执行

//他人权限模式
 #define PM_O_READ 0x08				//可读
 #define PM_O_WRITE 0x10			//可写
 #define PM_O_EXECUTE 0x20			//可执行

 #define FL_WRITE_MTXLOCK 0x40		//是否有文件写互斥锁
 #define FL_TYPE_FILE 0x80			//是文件还是文件夹(FL_TYPE_FILE 表示是文件)

#pragma pack(push, 1)

//安全信息节点,i节点和文件夹节点公有的一个头
struct SafeInfo
{
	fsign sign;
	ctsize_t mutexRead, shareRead;
	ctsize_t shareWrite;
	time_t created;
	time_t modified;
	uidsize_t uid;
};
//空闲链表
struct FreeNode
{
	size_t size;	//连续多少个块是空闲的
	cpos_t next;	//下一个空闲节点位置
};
//文件系统信息,永远存在块'0'处
struct FSHeader
{
	size_t capacity;		//容量
	size_t remain;			//剩余
	cpos_t freeChunkHead;	//空闲链表头
};
//文件夹内的文件信息节点
struct FileNode
{
	char name[16];	//名字
	cpos_t pos;		//位置
};
//文件夹扩展节点
struct AppendFloderNode
{
	size_t nodeSize;
	cpos_t next;
};
//文件夹专属头节点(紧放在SafeInfo后)
struct FloderInfo
{
	size_t nodeSize;
	cpos_t parent;
	cpos_t next;
};
//文件夹信息汇总,方便管理用,并不对应硬盘上的空间
struct Folder
{
	cpos_t cur;
	SafeInfo* sfInfo;
	FloderInfo* info;
	FileNode* nodes;
};

//num == 0时,表示pos指向的是一个扩展i节点,否则表示pos处开始连续num个块都属于这个文件
struct CKInfo
{
	cksize_t num;
	cpos_t pos;
};
//文件专属头信息
struct INInfo
{
	size_t fileSize;
	cpos_t parent;
};
//为了方便管理的i节点信息结构
struct IndexNode
{
	cpos_t cur;
	SafeInfo* sfInfo;
	INInfo* info;
	size_t nodeSize;
	CKInfo* nodes;
};
//i节点扩展节点,在这个结构后紧挨着nodeSize个CKInfo
struct AppendINode
{
	size_t nodeSize;
};

#pragma pack(pop)
//用来管理一个被打开的信息(比如读入的chunklist文件大小等等)
struct FDMInfo
{
	SafeInfo sfInfo;
	cpos_t hdpos;
	size_t openCount;
	size_t fileSize;
	size_t cklistSize;
	CKInfo* cklist;
};
//用来存放单个用户对单个文件的信息(比如打开模式等)
struct FDSInfo
{
	fdtype_t model;
	int tfd, sfd;
	size_t ckpos;
	size_t rlpos;
	size_t abpos;
	std::thread::id tid;
};



#endif