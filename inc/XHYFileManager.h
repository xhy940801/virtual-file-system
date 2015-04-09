#ifndef _XHYFILEMANGER_H
#define _XHYFILEMANGER_H

#include "types.h"
#include "FMStructs.h"
#include "DiskDriver.h"
#include "ChunkHelper.h"
#include "functions.h"

#include <thread>
#include <mutex>
#include <map>

#define ERR_FLOCKED 1000
#define ERR_FLODER_NOTEPT 1001
#define ERR_PATH_NOTEXT 1002
#define ERR_CN_DEL_ROOT 1003
#define ERR_SYS_LOADFAIL 1004
#define ERR_SYS_SAVEFAIL 1005
#define ERR_MEM_FULL 1006
#define ERR_NME_CONFLICT 1007
#define ERR_PATH_IS_NOT_FLODER 1008

#define OPTYPE_READ 0x01
#define OPTYPE_WRITE 0x02
#define OPTYPE_WTE_MTX_LOCK 0x10
#define OPTYPE_WTE_SHR_LOCK 0x20
#define OPTYPE_RAD_MTX_LOCK 0x40
#define OPTYPE_RAD_SHR_LOCK 0x80

class XHYFileManager
{
	struct BufHandler
	{
		cpos_t pos;
		char* buf;
		BufHandler()
			: pos(0), buf(0)
		{
		}
	};
	static const size_t HANDLERSIZE = 4;
private:
	DiskDriver* driver;
	const size_t chunkSize;
	std::mutex mtx;
	FSHeader header;
	BufHandler bufHandlers[HANDLERSIZE];
	std::map<std::thread::id, int> _errno;
	std::map<int, FDMInfo> _fdmap;
	std::map<std::thread::id, std::vector<FDSInfo> > _thmap;
private:
	bool createFolder(cpos_t parent, const char* name, uidsize_t uid);
	bool createFile(cpos_t parent, const char* name, uidsize_t uid);

	cpos_t getFreeChunk(size_t expected, size_t& had);
	void releaseChunk(cpos_t pos, size_t size);
	void flush();

	bool checkHad(FileNode* fn, size_t len, const char* name);

	bool createBlankFolder(cpos_t parent, cpos_t cur, uidsize_t uid);
	bool createBlankFile(cpos_t parent, cpos_t cur, uidsize_t uid);
	bool addItemToFolder(Folder folder, cpos_t target, const char* name);
	template <typename CheckPmsS>
	bool delItem(cpos_t cur, CheckPmsS pms);

	template <typename CheckPms>
	bool delNode(CKInfo* info, CheckPms pms);

	bool load(cpos_t pos, size_t hdlid);
	Folder loadFolder(cpos_t pos, size_t hdlid);
	IndexNode loadINode(cpos_t pos, size_t hdlid);
	int checkType(size_t hdlid);

	cpos_t loadPath(const char* path, size_t len);

	void setErrno(int err);

	void releaseFd(FDSInfo& info);
	void flushHandler(FDMInfo& info);
public:
	enum FPos { beg, cur, end };
public:
	XHYFileManager(DiskDriver* dvr);
	~XHYFileManager();

	bool initFS();
	
	template <typename CheckPmsP, typename CheckPmsS>
	bool deleteItem(const char* path, CheckPmsP pmsp, CheckPmsS pmss);

	bool createFile(const char* path, const char* name, uidsize_t uid);
	bool createFolder(const char* path, const char* name, uidsize_t uid);
	int readFolder(const char* path, size_t start, size_t len, FileNode* result);
	bool getItemSafeInfo(const char* path, SafeInfo* info);

	int getErrno();

	void bindThread(std::thread::id thId);
	void releaseThread(std::thread::id thId);

	template <typename CheckPms>
	int open(const char* path, fdtype_t type, CheckPms pms);

	int read(int fd, void* buf, size_t len);
	int write(int fd, void* buf, size_t len);

	int tell(int fd);
	int seek(int fd, int offset, FPos pos);
};

template <typename CheckPmsS>
bool XHYFileManager::delItem(cpos_t cur, CheckPmsS pms)
{
	if(!driver->getChunk(cur, bufHandlers[3].buf, chunkSize))
	{
		setErrno(ERR_SYS_LOADFAIL);
		return false;
	}
	ChunkHelper chpr(bufHandlers[3].buf, chunkSize);
	SafeInfo *sfInfo = chpr.getSafeInfo();
	if(FL_WRITE_MTXLOCK & sfInfo->sign || sfInfo->mutexRead || sfInfo->shareRead || sfInfo->shareWrite)
	{
		setErrno(ERR_FLOCKED);
		return false;
	}
	int err = pms(sfInfo);
	if(err != 0)
	{
		setErrno(err);
		return false;
	}
	if(sfInfo->sign & FL_TYPE_FILE)
	{
		INInfo* info = chpr.getINInfo();
		if(info->fileSize + sizeof(SafeInfo) + sizeof(INInfo) > chunkSize)
		{
			size_t ckLen = chpr.getCKLen();
			CKInfo* cks = chpr.getCKInfo();
			for(size_t i = 0; i < ckLen; ++i)
				if(!delNode(&cks[i], pms))
					return false;
		}
	}
	else
	{
		FloderInfo* info = chpr.getFloderInfo();
		if(info->nodeSize != 0)
		{
			setErrno(ERR_FLODER_NOTEPT);
			return false;
		}
	}
	releaseChunk(cur, 1);
	return true;
}

template <typename CheckPms>
bool XHYFileManager::delNode(CKInfo* info, CheckPms pms)
{
	if(info->pos != 0)
	{
		if(info->num < 0)
		{
			if(!driver->getChunk(info->pos, bufHandlers[3].buf, chunkSize))
			{
				setErrno(ERR_SYS_LOADFAIL);
				return false;
			}
			ChunkHelper chpr(bufHandlers[3].buf, chunkSize);
			size_t cklen = chpr.getAPCKLen();
			CKInfo* ckInfo = chpr.getAPCKInfo();
			for(size_t i = 0; i < cklen; ++i)
				delNode(&ckInfo[i], pms);
			releaseChunk(info->pos, 1);
		}
		else if(info->num > 0)
			releaseChunk(info->pos, info->num);
	}
	return true;
}

template <typename CheckPmsP, typename CheckPmsS>
bool XHYFileManager::deleteItem(const char* path, CheckPmsP pmsp, CheckPmsS pmss)
{
	std::lock_guard<std::mutex> lck (mtx);
	if(path[0] != '/')
	{
		setErrno(ERR_PATH_NOTEXT);
		return false;
	}
	size_t len = strlen(path);
	if(len == 1)
	{
		setErrno(ERR_CN_DEL_ROOT);
		return false;
	}
	size_t ptlen = len;
	while(true)
	{
		if(path[--ptlen] == '/')
			break;
	}
	cpos_t parent = loadPath(path, ptlen);
	if(parent == 0)
		return false;
	Folder folder = loadFolder(parent, 0);
	if(folder.cur == 0)
		return false;
	int err = pmsp(folder.sfInfo);
	if(err != 0)
	{
		setErrno(err);
		return false;
	}
	size_t* pns;
	FileNode* cnode = 0;
	FileNode* lnode = 0;
	for(size_t i = 0; i < folder.info->nodeSize; ++i)
		if(strcmp(path + ptlen + 1, folder.nodes[i].name) == 0)
		{
			cnode = &folder.nodes[i];
			break;
		}
	AppendFloderNode* apdFN = 0;
	FileNode* nodes = 0;
	cpos_t nxtpos = folder.info->next;
	bool mustSort = false;
	if(cnode == 0)
	{
		if(folder.info->next == 0)
		{
			setErrno(ERR_PATH_NOTEXT);
			return false;
		}
		while(true)
		{
			if(nxtpos == 0)
			{
				setErrno(ERR_PATH_NOTEXT);
				return false;
			}
			if(!load(nxtpos, 1))
				return false;
			apdFN = (AppendFloderNode*) bufHandlers[1].buf;
			nodes = (FileNode*) (bufHandlers[1].buf + sizeof(AppendFloderNode));
			for(size_t i = 0; i < apdFN->nodeSize; ++i)
				if(strcmp(path + ptlen + 1, nodes[i].name) == 0)
				{
					cnode = &nodes[i];
					break;
				}
			if(cnode == 0)
				nxtpos = apdFN->next;
			else
				break;
		}
	}

	if(folder.info->next == 0)
	{
		lnode = &folder.nodes[folder.info->nodeSize - 1];
		pns = &folder.info->nodeSize;
	}
	else
	{
		while(nxtpos)
		{
			if(!load(nxtpos, 2))
				return false;
			apdFN = (AppendFloderNode*) bufHandlers[2].buf;
			nodes = (FileNode*) (bufHandlers[2].buf + sizeof(AppendFloderNode));
			nxtpos = apdFN->next;
		}
		lnode = &nodes[apdFN->nodeSize - 1];
		pns = &apdFN->nodeSize;
		if(*pns == 1)
			mustSort = true;
	}
	--(*pns);
	cpos_t ndpos = cnode->pos;
	*cnode = *lnode;
	if(!delItem(ndpos, pmss))
		return false;
	flush();
	if(!mustSort)
		return true;
	size_t pt = 0, sn = 1;
	folder = loadFolder(parent, sn);
	if(folder.cur == 0)
		return false;
	cpos_t* ndcg = &folder.info->next;
	cpos_t* pdcg = ndcg;
	while(*ndcg)
	{
		pdcg = ndcg;
		xswap(pt, sn);
		if(!load(*ndcg, sn))
			return false;
		apdFN = (AppendFloderNode*) bufHandlers[sn].buf;
		ndcg = &apdFN->next;
	}
	*pdcg = 0;
	cpos_t nf = bufHandlers[sn].pos;
	bufHandlers[sn].pos = 0;
	releaseChunk(nf, 1);
	flush();
	return true;
}

template <typename CheckPms>
int XHYFileManager::open(const char* path, fdtype_t type, CheckPms pms)
{
	
}

#endif