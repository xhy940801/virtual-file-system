#include "XHYFileManager.h"

#include <ctime>

XHYFileManager::XHYFileManager(DiskDriver* dvr)
	: driver(dvr), chunkSize(dvr->getChunkSize())
{
	driver->getChunk(0, &header, sizeof(header));
	for(size_t i = 0; i < HANDLERSIZE; ++i)
	{
		bufHandlers[i].pos = 0;
		bufHandlers[i].buf = new char[chunkSize];
	}
}

XHYFileManager::~XHYFileManager()
{
	flush();
}

bool XHYFileManager::initFS()
{
	std::lock_guard<std::mutex> lck (mtx);
	header.capacity = driver->getTotalChunkNumber() * chunkSize;
	header.remain = header.capacity - 2 * chunkSize;
	header.freeChunkHead = 2;
	FreeNode fn;
	fn.size = driver->getTotalChunkNumber() - 2;
	fn.next = 0;
	if(!driver->setChunk(2, &fn, sizeof(fn)))
	{
		setErrno(ERR_SYS_SAVEFAIL);
		return false;
	}
	if(!createBlankFolder(0, 1, 0))
		return false;
	flush();
	return true;
}

bool XHYFileManager::createFile(const char* path, const char* name, uidsize_t uid)
{
	std::lock_guard<std::mutex> lck (mtx);
	cpos_t parent = loadPath(path, strlen(path));
	if(parent == 0)
		return false;
	return createFile(parent, name, uid);
}

bool XHYFileManager::createFolder(const char* path, const char* name, uidsize_t uid)
{
	std::lock_guard<std::mutex> lck (mtx);
	cpos_t parent = loadPath(path, strlen(path));
	if(parent == 0)
		return false;
	return createFolder(parent, name, uid);
}

int XHYFileManager::readFolder(const char* path, size_t start, size_t len, FileNode* result)
{
	std::lock_guard<std::mutex> lck (mtx);
	if(path[0] != '/')
	{
		setErrno(ERR_PATH_NOTEXT);
		return false;
	}
	cpos_t parent = loadPath(path, strlen(path));
	if(parent == 0)
		return -1;
	Folder folder = loadFolder(parent, 0);
	if(folder.cur == 0)
		return -1;
	if(checkType(0) != 1)
	{
		setErrno(ERR_PATH_IS_NOT_FLODER);
		return -1;
	}
	size_t cp = 0;
	cpos_t nxt = 0;
	if(start < folder.info->nodeSize)
	{
		for(size_t &i = cp, &j = start; i < len && j < folder.info->nodeSize; ++i, ++j)
			result[i] = folder.nodes[j];
		nxt = folder.info->next;
		start = 0;
	}
	else
	{
		if(folder.info->next == 0)
			return 0;
		start -= folder.info->nodeSize;
		nxt = folder.info->next;
		while(true)
		{
			if(nxt == 0)
				break;
			if(!load(nxt, 1))
				return -1;
			AppendFloderNode* paf = (AppendFloderNode*) bufHandlers[1].buf;
			if(start < paf->nodeSize)
				break;
			start -= paf->nodeSize;
			nxt = paf->next;
		}
	}
	while(true)
	{
		if(nxt == 0)
			return cp;
		if(cp >= len)
			return cp;
		if(!load(nxt, 1))
			return -1;
		AppendFloderNode* paf = (AppendFloderNode*) bufHandlers[1].buf;
		FileNode* pfn = (FileNode*) (bufHandlers[1].buf + sizeof(AppendFloderNode));
		for(size_t &i = cp, &j = start; i < len && j < paf->nodeSize; ++i, ++j)
			result[i] = pfn[j];
		start = 0;
		nxt = paf->next;
	}
}

bool XHYFileManager::getItemSafeInfo(const char* path, SafeInfo* info)
{
	std::lock_guard<std::mutex> lck (mtx);
	if(path[0] != '/')
	{
		setErrno(ERR_PATH_NOTEXT);
		return false;
	}
	cpos_t cur = loadPath(path, strlen(path));
	if(cur == 0)
		return false;
	if(!load(cur, 0))
		return false;
	*info = * (SafeInfo*) bufHandlers[0].buf;
	return true;
}

void getItemSafeInfo::bindThread(std::thread::id thId)
{
	std::lock_guard<std::mutex> lck (mtx);
	_thmap[thId] = std::vector<FDSInfo>();
}

void getItemSafeInfo::releaseThread(std::thread::id thId)
{
	std::lock_guard<std::mutex> lck (mtx);
	auto& vec = _thmap[thId];
	for(size_t i = 0; i < vec.size(); ++i)
		vec[i].
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////              private functions              //////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool XHYFileManager::createFolder(cpos_t parent, const char* name, uidsize_t uid)
{
	size_t ts;
	cpos_t pos = getFreeChunk(1, ts);
	if(ts == 0)
		return false;
	Folder folder = loadFolder(parent, 0);
	if(folder.cur == 0)
		return false;
	if(folder.sfInfo->sign & FL_WRITE_MTXLOCK && folder.sfInfo->mutexRead)
	{
		setErrno(ERR_FLOCKED);
		return false;
	}
	if(!addItemToFolder(folder, pos, name))
		return false;
	if(!createBlankFolder(parent, pos, uid))
		return false;
	flush();
	return true;
}

bool XHYFileManager::createFile(cpos_t parent, const char* name, uidsize_t uid)
{
	size_t ts;
	cpos_t pos = getFreeChunk(1, ts);
	if(ts == 0)
		return false;
	Folder folder = loadFolder(parent, 0);
	if(folder.cur == 0)
		return false;
	if(folder.sfInfo->sign & FL_WRITE_MTXLOCK && folder.sfInfo->mutexRead)
	{
		setErrno(ERR_FLOCKED);
		return false;
	}
	if(!addItemToFolder(folder, pos, name))
		return false;
	if(!createBlankFile(parent, pos, uid))
		return false;
	flush();
	return true;
}

cpos_t XHYFileManager::getFreeChunk(size_t expected, size_t& had)
{
	if(header.freeChunkHead == 0)
	{
		setErrno(ERR_MEM_FULL);
		return 0;
	}
	FreeNode fn;
	if(!driver->getChunk(header.freeChunkHead, &fn, sizeof(fn)))
	{
		setErrno(ERR_SYS_LOADFAIL);
		return 0;
	}
	cpos_t rs = header.freeChunkHead;
	if(fn.size > expected)
	{
		fn.size -= expected;
		had = expected;
		if(!driver->setChunk(header.freeChunkHead + expected, &fn, sizeof(fn)))
		{
			setErrno(ERR_SYS_SAVEFAIL);
			return 0;
		}
		header.freeChunkHead += expected;
	}
	else
	{
		header.freeChunkHead = fn.next;
		had = fn.size;
	}
	return rs;
}

void XHYFileManager::releaseChunk(cpos_t pos, size_t size)
{
	FreeNode fn;
	if(pos + size == header.freeChunkHead)
	{
		if(!driver->getChunk(header.freeChunkHead, &fn, sizeof(fn)))
		{
			fn.size = size;
			fn.next = header.freeChunkHead;
		}
		fn.size += size;
	}
	else
	{
		fn.size = size;
		fn.next = header.freeChunkHead;
	}
	if(!driver->setChunk(pos, &fn, sizeof(fn)))
		return;
	header.freeChunkHead = pos;
	return;
}

void XHYFileManager::flush()
{
	driver->setChunk(0, &header, sizeof(header));
	for(size_t i = 0; i < HANDLERSIZE; ++i)
	{
		if(bufHandlers[i].pos)
			driver->setChunk(bufHandlers[i].pos, bufHandlers[i].buf, chunkSize);
		bufHandlers[i].pos = 0;
	}
}

bool XHYFileManager::checkHad(FileNode* fn, size_t len, const char* name)
{
	for(size_t i = 0; i < len; ++i)
		if(strcmp(name, fn[i].name) == 0)
			return true;
	return false;
}

bool XHYFileManager::createBlankFolder(cpos_t parent, cpos_t cur, uidsize_t uid)
{
	char tmp[sizeof(FloderInfo) + sizeof(SafeInfo)];
	SafeInfo &sfInfo = * (SafeInfo*) tmp;
	FloderInfo &info = * (FloderInfo*) (tmp + sizeof(SafeInfo));
	sfInfo.sign = PM_U_READ | PM_U_WRITE;
	sfInfo.mutexRead = sfInfo.shareRead = sfInfo.shareWrite = 0;
	sfInfo.uid = uid;
	info.nodeSize = 0;
	info.modified = info.created = time(0);
	info.parent = parent;
	info.next = 0;
	if(!driver->setChunk(cur, tmp, sizeof(tmp)))
	{
		setErrno(ERR_SYS_SAVEFAIL);
		return false;
	}
	return true;
}

bool XHYFileManager::createBlankFile(cpos_t parent, cpos_t cur, uidsize_t uid)
{
	char tmp[sizeof(INInfo) + sizeof(SafeInfo)];
	SafeInfo &sfInfo = * (SafeInfo*) tmp;
	INInfo &info = * (INInfo*) (tmp + sizeof(SafeInfo));
	sfInfo.sign = PM_U_READ | PM_U_WRITE;
	sfInfo.mutexRead = sfInfo.shareRead = sfInfo.shareWrite = 0;
	sfInfo.uid = uid;
	info.modified = info.created = time(0);
	info.fileSize = 0;
	info.parent = parent;
	if(!driver->setChunk(cur, tmp, sizeof(tmp)))
	{
		setErrno(ERR_SYS_SAVEFAIL);
		return false;
	}
	return true;
}

bool XHYFileManager::addItemToFolder(Folder folder, cpos_t target, const char* name)
{
	if(checkHad(folder.nodes, folder.info->nodeSize, name))
	{
		setErrno(ERR_NME_CONFLICT);
		return false;
	}
	if(sizeof(SafeInfo) + sizeof(FloderInfo) + folder.info->nodeSize * sizeof(FileNode) + sizeof(FileNode) <= chunkSize)
	{
		strcpy(folder.nodes[folder.info->nodeSize].name, name);
		folder.nodes[folder.info->nodeSize].pos = target;
		++folder.info->nodeSize;
	}
	else if(folder.info->next == 0)
	{
		size_t ts;
		cpos_t pos = getFreeChunk(1, ts);
		if(ts == 0)
			return false;
		bufHandlers[1].pos = pos;
		AppendFloderNode* afn = (AppendFloderNode*) bufHandlers[1].buf;
		afn->nodeSize = 1;
		afn->next = 0;
		FileNode* pfn = (FileNode*) (((char*) bufHandlers[1].buf) + sizeof(AppendFloderNode));
		strcpy(pfn->name, name);
		pfn->pos = target;
		folder.info->next = pos;
	}
	else
	{
		cpos_t capdFN = folder.info->next;
		while(true)
		{
			AppendFloderNode* tapdFN = (AppendFloderNode*) bufHandlers[1].buf;
			if(capdFN == 0)
			{
				size_t ts;
				cpos_t pos = getFreeChunk(1, ts);
				if(ts == 0)
					return false;
				bufHandlers[2].pos = pos;
				AppendFloderNode* afn = (AppendFloderNode*) bufHandlers[2].buf;
				afn->nodeSize = 1;
				afn->next = 0;
				FileNode* pfn = (FileNode*) (((char*) bufHandlers[2].buf) + sizeof(AppendFloderNode));
				strcpy(pfn->name, name);
				pfn->pos = target;
				tapdFN->next = pos;
				break;
			}
			if(!load(capdFN, 1))
				return false;
			tapdFN = (AppendFloderNode*) bufHandlers[1].buf;
			FileNode* pfn = (FileNode*) (((char*) bufHandlers[1].buf) + sizeof(AppendFloderNode));
			if(checkHad(pfn, tapdFN->nodeSize, name))
			{
				setErrno(ERR_NME_CONFLICT);
				return false;
			}
			if(tapdFN->nodeSize * sizeof(FileNode) + sizeof(FileNode) + sizeof(AppendFloderNode) <= chunkSize)
			{
				strcpy(pfn[tapdFN->nodeSize].name, name);
				pfn[tapdFN->nodeSize].pos = target;
				++tapdFN->nodeSize;
				break;
			}
			capdFN = tapdFN->next;
		}
	}
	folder.modified = time(0);
	return true;
}

bool XHYFileManager::load(cpos_t pos, size_t hdlid)
{
	if(!driver->getChunk(pos, bufHandlers[hdlid].buf, chunkSize))
	{
		setErrno(ERR_SYS_LOADFAIL);
		return false;
	}
	bufHandlers[hdlid].pos = pos;
	return true;
}

Folder XHYFileManager::loadFolder(cpos_t pos, size_t hdlid)
{
	Folder folder;
	if(!load(pos, hdlid))
	{
		folder.cur = 0;
		return folder;
	}
	folder.cur = pos;
	folder.sfInfo = (SafeInfo*) bufHandlers[hdlid].buf;
	folder.info = (FloderInfo*) (bufHandlers[hdlid].buf + sizeof(SafeInfo));
	folder.nodes = (FileNode*) (bufHandlers[hdlid].buf + sizeof(SafeInfo) + sizeof(FloderInfo));
	return folder;
}

IndexNode XHYFileManager::loadINode(cpos_t pos, size_t hdlid)
{
	IndexNode inode;
	if(!load(pos, hdlid))
	{
		inode.cur = 0;
		return inode;
	}
	inode.cur = pos;
	inode.sfInfo = (SafeInfo*) bufHandlers[hdlid].buf;
	inode.info = (INInfo*) (bufHandlers[hdlid].buf + sizeof(SafeInfo));
	inode.nodeSize = * (size_t*) (bufHandlers[hdlid].buf + sizeof(SafeInfo) + sizeof(INInfo));
	inode.nodes = (CKInfo*) (bufHandlers[hdlid].buf + sizeof(SafeInfo) + sizeof(INInfo) + sizeof(size_t));
	return inode;
}

int XHYFileManager::checkType(size_t hdlid)
{
	SafeInfo* sfInfo = (SafeInfo*) bufHandlers[hdlid].buf;
	if(sfInfo->sign & FL_TYPE_FILE)
		return 0;
	return 1;
}

cpos_t XHYFileManager::loadPath(const char* path, size_t len)
{
	Folder folder = loadFolder(1, 0);
	size_t st = 1;
	cpos_t next = 1;
	while(st < len)
	{
		if(folder.cur == 0)
			return false;
		if(folder.sfInfo->sign & FL_TYPE_FILE)
		{
			setErrno(ERR_PATH_NOTEXT);
			return 0;
		}
		next = 0;
		size_t ed;
		for(ed = st; ed < len; ++ed)
			if(path[ed] == '/')
				break;
		size_t sblen = ed - st;
		const char* sbpath = path + st;

		for(size_t i = 0; i < folder.info->nodeSize; ++i)
			if(xstrequ(sbpath, sblen, folder.nodes[i].name))
			{
				next = folder.nodes[i].pos;
				break;
			}
		if(next == 0)
		{
			AppendFloderNode* apdFN;
			FileNode* nodes;
			cpos_t nxtafn = folder.info->next;
			while(nxtafn)
			{
				if(!load(nxtafn, 1))
					return 0;
				apdFN = (AppendFloderNode*) bufHandlers[1].buf;
				nodes = (FileNode*) (bufHandlers[1].buf + sizeof(AppendFloderNode));

				for(size_t i = 0; i < apdFN->nodeSize; ++i)
					if(xstrequ(sbpath, sblen, nodes[i].name))
					{
						next = nodes[i].pos;
						break;
					}

				nxtafn = apdFN->next;
			}
		}

		if(next == 0)
		{
			setErrno(ERR_PATH_NOTEXT);
			return 0;
		}

		folder = loadFolder(next, 0);
		st = ed + 1;
	}
	return next;
}

void XHYFileManager::setErrno(int err)
{
	_errno[std::this_thread::get_id()] = err;
}

int XHYFileManager::getErrno()
{
	std::lock_guard<std::mutex> lck(mtx);
	return _errno[std::this_thread::get_id()];
}

inline void XHYFileManager::releaseFd(FDSInfo& info)
{
	FDMInfo &mInfo = _fdmap[info.fd];
	if(--mInfo.openCoune == 0)
	{
		flushHandler(mInfo);
		_fdmap.erase(_fdmap.find(info.fd));
	}
	else
	{
		if(mInfo.model & OPTYPE_WTE_MTX_LOCK)
			mInfo.sfInfo.sign &= ~FL_WRITE_MTXLOCK;
		else if(mInfo.model & OPTYPE_WTE_SHR_LOCK)
			--mInfo.sfInfo.shareWrite;
		else if(mInfo.model & OPTYPE_RAD_MTX_LOCK)
			--mInfo.sfInfo.mutexRead;
		else if(mInfo.model & OPTYPE_RAD_SHR_LOCK)
			--mInfo.sfInfo.shareRead;
		flushHandler(minfo);
	}
}

inline void XHYFileManager::flushHandler(FDMInfo& info)
{
	driver->setChunk(info.hdpos, &info.sfInfo, sizeof(info.sfInfo));
}