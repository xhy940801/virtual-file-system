#ifndef _XHYFILEMANGER_H
#define _XHYFILEMANGER_H

#include "types.h"
#include "FMStructs.h"
#include "DiskDriver.h"
#include "ChunkHelper.h"
#include "functions.h"
#include "commondef.h"

#include <thread>
#include <mutex>
#include <map>
#include <list>
#include <vector>

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
public:
	enum FPos { beg, cur, end }; //文件位置的枚举
private:
	int _cmfd, _csfd;
	DiskDriver* driver;			//虚拟硬盘驱动
	const size_t chunkSize;		//块大小
	std::recursive_mutex mtx;	//互斥锁,防止多人同时使用FileManager的操作引起不必要的麻烦
	FSHeader header;			//超级块信息
	BufHandler bufHandlers[HANDLERSIZE];	//预先分配的缓存
	std::map<std::thread::id, int> _errno;	//每个线程独自拥有的错误码,防止多线程情况下获取不到自己的错误信息
	std::map<int, FDMInfo> _fdmmap;			//整体文件描述符对文件信息的映射
	std::map<int, std::list<FDSInfo>::iterator> _fdsmap;	//整体文件描述符对局部文件描述符的映射
	std::map<cpos_t, int> _cpmap;			//文件位置对整体文件描述符的映射
	std::map<std::thread::id, std::list<FDSInfo> > _thmap;		//线程对自己的局部描述符的映射
private:
	bool createFolder(cpos_t parent, const char* name, uidsize_t uid);	//创建文件
	bool createFile(cpos_t parent, const char* name, uidsize_t uid);	//创建文件夹

	cpos_t getFreeChunk(cksize_t expected, cksize_t& had);				//获取空闲空间,空闲空间的头由函数返回,空闲空间后有多少连续空间空闲
	void releaseChunk(cpos_t pos, size_t size);							//释放空间
	void flush();														//把缓存刷入硬盘中

	bool checkHad(FileNode* fn, size_t len, const char* name);			//检查文件夹内释放有名为name的文件

	bool createBlankFolder(cpos_t parent, cpos_t cur, uidsize_t uid);	//创建空文件夹
	bool createBlankFile(cpos_t parent, cpos_t cur, uidsize_t uid);		//创建空文件
	bool addItemToFolder(Folder folder, cpos_t target, const char* name);		//往文件夹里加东西
	template <typename CheckPmsS>
	bool delItem(cpos_t cur, CheckPmsS pms);							//删除东西 pms是一个伪函数的模板类,要去能接受pms(SafeInfo*)类型

	bool delNode(CKInfo* info);											//删除节点

	bool load(cpos_t pos, size_t hdlid);								//载入一个节点放入编号为hdlid的缓存中
	Folder loadFolder(cpos_t pos, size_t hdlid);						//载入一个文件夹放入编号为hdlid的缓存中
	IndexNode loadINode(cpos_t pos, size_t hdlid);						//载入一个i节点放入编号为hdlid的缓存中
	int checkType(size_t hdlid);										//检测编号为hdlid的缓存里存放的是文件还是文件夹

	cpos_t loadPath(const char* path, size_t len);						//通过路径获取文件/文件夹的硬盘位置

	void setErrno(int err);												//设置错误信息(错误信息多个线程间独立)

	void releaseFd(FDSInfo& info);										//释放文件
	void flushHandler(FDMInfo& info);									//把文件头信息刷入硬盘中
	void flushHandler(FDMInfo& info, time_t modified);					//把文件头信息刷入硬盘中,同时改变修改时间

	int getFreeMFd();													//获取一个自由的主文件描述符
	int getFreeSFd();													//获取一个自由的从文件描述符
	bool fillCKInfo(std::vector<CKInfo>& vec, CKInfo& info);

	bool tryLock(SafeInfo& sfInfo, fdtype_t type);						//尝试锁住文件
	bool isOwner(int fd);												//判断文件描述符是否属于这个线程
	bool appendFile(FDMInfo& mInfo, size_t len);						//扩展文件
	void _seek(FDMInfo& mInfo, FDSInfo& info, int offset, FPos pos);	//移动文件指针
public:
	XHYFileManager(DiskDriver* dvr);
	~XHYFileManager();

	bool initFS();
	
	template <typename CheckPmsP, typename CheckPmsS>
	bool deleteItem(const char* path, CheckPmsP pmsp, CheckPmsS pmss);	//删除项目:pmsp父目录权限验证,pmss自身权限验证

	bool createFile(const char* path, const char* name, uidsize_t uid);	//创建文件
	bool createFolder(const char* path, const char* name, uidsize_t uid);		//创建文件夹
	int readFolder(const char* path, size_t start, size_t len, FileNode* result);//读取文件夹
	bool getItemSafeInfo(const char* path, SafeInfo* info);				//获取文件/文件夹详细信息

	int getErrno();								//获取错误码

	void bindThread(std::thread::id thId);		//绑定线程
	void releaseThread(std::thread::id thId);	//释放线程

	template <typename CheckPms>
	int open(const char* path, fdtype_t type, CheckPms pms);	//打开文件:pms权限验证的伪函数模板类pms(SafeInfo*, fdtype_t)

	bool close(int fd);							//关闭文件

	int read(int fd, void* buf, size_t len);	//读
	int write(int fd, const void* buf, size_t len);	//写

	int tell(int fd);	//获取文件指针
	int seek(int fd, int offset, FPos pos);	//改变文件指针

	template <typename CheckPms>
	bool changeSafeInfo(const char* path, SafeInfo* newInfo, CheckPms pms);	//改变安全信息,pms权限

	void lock() { mtx.lock(); }	//手动获得锁(每个调用自身会获得一次锁,这个锁的作用是实现事务操作)
	void unlock() { mtx.unlock(); }	//手动释放锁
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
				if(!delNode(&cks[i]))
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

template <typename CheckPmsP, typename CheckPmsS>
bool XHYFileManager::deleteItem(const char* path, CheckPmsP pmsp, CheckPmsS pmss)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
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
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if((type & OPTYPE_READ) && !(type & OPTYPE_RAD_SHR_LOCK))
		type |= OPTYPE_RAD_MTX_LOCK;
	if((type & OPTYPE_WRITE) && !(type & OPTYPE_WTE_SHR_LOCK))
		type |= OPTYPE_WTE_MTX_LOCK;
	size_t len = strlen(path);
	if(path[0] != '/')
	{
		setErrno(ERR_PATH_NOTEXT);
		return 0;
	}
	if(len == 1)
	{
		setErrno(ERR_PATH_IS_NOT_FILE);
		return 0;
	}
	cpos_t cur = loadPath(path, len);
	auto it = _cpmap.find(cur);
	int rsfd;
	if(it == _cpmap.end())
	{
		IndexNode inode = loadINode(cur, 0);
		if(inode.cur == 0)
			return false;
		if(checkType(0) != 0)
		{
			setErrno(ERR_PATH_IS_NOT_FILE);
			return 0;
		}
		int err = pms(inode.sfInfo, type);
		if(err != 0)
		{
			setErrno(err);
			return 0;
		}
		FDMInfo info;
		info.sfInfo = *inode.sfInfo;
		info.hdpos = cur;
		info.openCount = 1;
		info.fileSize = inode.info->fileSize;

		if(info.fileSize + sizeof(SafeInfo) + sizeof(INInfo) > chunkSize)
		{
			std::vector<CKInfo> vckInfos;
			for(size_t i = 0; i < inode.nodeSize; ++i)
				if(!fillCKInfo(vckInfos, inode.nodes[i]))
					return 0;
			info.cklistSize = vckInfos.size();
			info.cklist = new  CKInfo[vckInfos.size()];
			for(size_t i = 0; i < vckInfos.size(); ++i)
				info.cklist[i] = vckInfos[i];
		}
		else
		{
			info.cklistSize = 0;
			info.cklist = 0;
		}
		if(!tryLock(info.sfInfo, type))
		{
			setErrno(ERR_FLOCKED);
			return 0;
		}
		rsfd = getFreeMFd();
		_fdmmap[rsfd] = info;
		_cpmap[cur] = rsfd;
	}
	else
	{
		rsfd = it->second;
		if(!tryLock(_fdmmap[rsfd].sfInfo, type))
		{
			setErrno(ERR_FLOCKED);
			return 0;
		}
		++_fdmmap[rsfd].openCount;
	}
	FDSInfo sinfo;
	sinfo.model = type;
	sinfo.tfd = rsfd;
	sinfo.sfd = getFreeSFd();
	sinfo.ckpos = 0;
	sinfo.rlpos = 0;
	sinfo.abpos = 0;
	sinfo.tid = std::this_thread::get_id();
	_thmap[std::this_thread::get_id()].push_back(sinfo);
	_fdsmap[sinfo.sfd] = (--_thmap[std::this_thread::get_id()].end());
	flushHandler(_fdmmap[rsfd]);
	return sinfo.sfd;
}

template <typename CheckPms>
bool XHYFileManager::changeSafeInfo(const char* path, SafeInfo* newInfo, CheckPms pms)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	size_t len = strlen(path);
	if(path[0] != '/')
	{
		setErrno(ERR_PATH_NOTEXT);
		return 0;
	}
	cpos_t cur = loadPath(path, len);
	if(!load(cur, 0))
		return false;
	SafeInfo* sfInfo = (SafeInfo*) bufHandlers[0].buf;
	int err = pms(sfInfo);
	if(err != 0)
	{
		setErrno(err);
		return false;
	}
	xswap(sfInfo->sign, newInfo->sign);
	sfInfo->sign &= ~(FL_WRITE_MTXLOCK | FL_TYPE_FILE);
	sfInfo->sign |= newInfo->sign & (FL_WRITE_MTXLOCK | FL_TYPE_FILE);
	flush();
	return true;
}

#endif