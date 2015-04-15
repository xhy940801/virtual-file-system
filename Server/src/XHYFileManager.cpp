#include "XHYFileManager.h"

#include <ctime>
#include <cstring>

XHYFileManager::XHYFileManager(DiskDriver* dvr)
	: _cmfd(0), _csfd(0), driver(dvr), chunkSize(dvr->getChunkSize())
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
	std::lock_guard<std::recursive_mutex> lck(mtx);
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
	std::lock_guard<std::recursive_mutex> lck(mtx);
	cpos_t parent = loadPath(path, strlen(path));
	if(parent == 0)
		return false;
	return createFile(parent, name, uid);
}

bool XHYFileManager::createFolder(const char* path, const char* name, uidsize_t uid)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	cpos_t parent = loadPath(path, strlen(path));
	if(parent == 0)
		return false;
	return createFolder(parent, name, uid);
}

int XHYFileManager::readFolder(const char* path, size_t start, size_t len, FileNode* result)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
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
	std::lock_guard<std::recursive_mutex> lck(mtx);
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

void XHYFileManager::bindThread(std::thread::id thId)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	_thmap[thId] = std::list<FDSInfo>();
	_errno[std::this_thread::get_id()] = 0;
}

void XHYFileManager::releaseThread(std::thread::id thId)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	auto& list = _thmap[thId];
	for(auto it = list.begin(); it != list.end(); ++it)
		releaseFd(*it);
	_thmap.erase(_thmap.find(thId));
	_errno.erase(_errno.find(std::this_thread::get_id()));
}

bool XHYFileManager::close(int fd)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if(!isOwner(fd))
	{
		setErrno(ERR_PMS_DENY);
		return false;
	}
	auto it = _fdsmap[fd];
	releaseFd(*it);
	_thmap[std::this_thread::get_id()].erase(it);
	return true;
}

int XHYFileManager::read(int fd, void* buf, size_t len)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if(!isOwner(fd))
	{
		setErrno(ERR_PMS_DENY);
		return -1;
	}
	FDSInfo& info = *_fdsmap[fd];
	if(!(info.model & OPTYPE_READ))
	{
		setErrno(ERR_PMS_DENY);
		return -1;
	}
	FDMInfo& mInfo = _fdmmap[info.tfd];
	if(info.abpos >= mInfo.fileSize)
		return 0;
	if(info.abpos + len >= mInfo.fileSize)
		len = mInfo.fileSize - info.abpos;
	size_t cp = 0;
	while(true)
	{
		if(cp == len)
			break;
		if(mInfo.cklistSize == 0)
		{
			if(!load(mInfo.hdpos, 0))
				return -1;
			memcpy(((char*) buf) + cp, bufHandlers[0].buf + sizeof(SafeInfo) + sizeof(INInfo) + info.abpos, len);
			cp += len;
			_seek(mInfo, info, (int) cp, XHYFileManager::cur);
		}
		else
		{
			size_t clcp = info.rlpos / chunkSize;
			size_t clsp = info.rlpos % chunkSize;
			size_t clrd = len - cp < chunkSize - clsp ? len - cp : chunkSize - clsp;
			if(!load(mInfo.cklist[info.ckpos].pos + clcp, 0))
				return -1;
			memcpy(((char*) buf) + cp, bufHandlers[0].buf + clsp, clrd);
			cp += clrd;
			info.rlpos += clrd;
			info.abpos += clrd;
			if(info.abpos == mInfo.fileSize)
			{
				_seek(mInfo, info, 0, XHYFileManager::end);
				break;
			}
			if(info.rlpos == mInfo.cklist[info.ckpos].num * chunkSize)
			{
				while(mInfo.cklist[++info.ckpos].num == 0);
				info.rlpos = 0;
			}
		}
	}
	return cp;
}

int XHYFileManager::write(int fd, const void* buf, size_t len)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if(!isOwner(fd))
	{
		setErrno(ERR_PMS_DENY);
		return -1;
	}
	FDSInfo& info = *_fdsmap[fd];
	if(!(info.model & OPTYPE_WRITE))
	{
		setErrno(ERR_PMS_DENY);
		return -1;
	}
	FDMInfo& mInfo = _fdmmap[info.tfd];
	if(info.abpos + len > mInfo.fileSize)
	{
		if(!appendFile(mInfo, info.abpos + len - mInfo.fileSize))
			return -1;
		_seek(mInfo, info, info.abpos, XHYFileManager::beg);
		len = mInfo.fileSize - info.abpos;
	}
	size_t cp = 0;
	while(true)
	{
		if(cp == len)
			break;
		if(mInfo.cklistSize == 0)
		{
			if(!load(mInfo.hdpos, 0))
				return -1;
			memcpy(bufHandlers[0].buf + sizeof(SafeInfo) + sizeof(INInfo) + info.abpos, ((char*) buf) + cp, len);
			cp += len;
			_seek(mInfo, info, (int) cp, XHYFileManager::cur);
		}
		else
		{
			size_t clcp = info.rlpos / chunkSize;
			size_t clsp = info.rlpos % chunkSize;
			size_t clrd = len - cp < chunkSize - clsp ? len - cp : chunkSize - clsp;
			if(!load(mInfo.cklist[info.ckpos].pos + clcp, 0))
				return -1;
			memcpy(bufHandlers[0].buf + clsp, ((char*) buf) + cp, clrd);
			cp += clrd;
			info.rlpos += clrd;
			info.abpos += clrd;
			if(info.abpos == mInfo.fileSize)
				_seek(mInfo, info, 0, XHYFileManager::end);
			else if(info.rlpos == mInfo.cklist[info.ckpos].num * chunkSize)
			{
				while(mInfo.cklist[++info.ckpos].num == 0);
				info.rlpos = 0;
			}
		}
		flush();
	}
	return cp;
}

int XHYFileManager::tell(int fd)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if(!isOwner(fd))
	{
		setErrno(ERR_PMS_DENY);
		return -1;
	}
	FDSInfo& info = *_fdsmap[fd];
	return info.abpos;
}

int XHYFileManager::seek(int fd, int offset, FPos pos)
{
	std::lock_guard<std::recursive_mutex> lck(mtx);
	if(!isOwner(fd))
	{
		setErrno(ERR_PMS_DENY);
		return -1;
	}
	FDSInfo& info = *_fdsmap[fd];
	FDMInfo& mInfo = _fdmmap[info.tfd];
	int old = info.abpos;
	_seek(mInfo, info, offset, pos);
	return old;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////              private functions              //////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool XHYFileManager::createFolder(cpos_t parent, const char* name, uidsize_t uid)
{
	cksize_t ts;
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
	cksize_t ts;
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

cpos_t XHYFileManager::getFreeChunk(cksize_t expected, cksize_t& had)
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
	header.remain -= had * chunkSize;
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
	header.remain += size * chunkSize;
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
	sfInfo.sign = PM_U_READ | PM_U_WRITE | PM_O_READ | PM_O_WRITE;
	sfInfo.mutexRead = sfInfo.shareRead = sfInfo.shareWrite = 0;
	sfInfo.modified = sfInfo.created = time(0);
	sfInfo.uid = uid;
	info.nodeSize = 0;
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
	sfInfo.sign = PM_U_READ | PM_U_WRITE | FL_TYPE_FILE;
	sfInfo.mutexRead = sfInfo.shareRead = sfInfo.shareWrite = 0;
	sfInfo.modified = sfInfo.created = time(0);
	sfInfo.uid = uid;
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
	if(strlen(name) > 15)
	{
		setErrno(ERR_NAME_OVERLENGTH);
		return false;
	}
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
		cksize_t ts;
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
				cksize_t ts;
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
	folder.sfInfo->modified = time(0);
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
	return _errno[std::this_thread::get_id()];
}

inline void XHYFileManager::releaseFd(FDSInfo& info)
{
	_fdsmap.erase(_fdsmap.find(info.sfd));
	FDMInfo &mInfo = _fdmmap[info.tfd];
	if(info.model & OPTYPE_WTE_MTX_LOCK)
		mInfo.sfInfo.sign &= ~FL_WRITE_MTXLOCK;
	else if(info.model & OPTYPE_WTE_SHR_LOCK)
		--mInfo.sfInfo.shareWrite;
	else if(info.model & OPTYPE_RAD_MTX_LOCK)
		--mInfo.sfInfo.mutexRead;
	else if(info.model & OPTYPE_RAD_SHR_LOCK)
		--mInfo.sfInfo.shareRead;
	if(--mInfo.openCount == 0)
	{
		flushHandler(mInfo);
		if(mInfo.cklist)
			delete mInfo.cklist;
		_cpmap.erase(_cpmap.find(mInfo.hdpos));
		_fdmmap.erase(_fdmmap.find(info.tfd));
	}
	else
	{
		if(info.model & OPTYPE_WRITE)
			flushHandler(mInfo, time(0));
		else
			flushHandler(mInfo);
	}
}

inline void XHYFileManager::flushHandler(FDMInfo& info)
{
	driver->setChunk(info.hdpos, &info.sfInfo, sizeof(info.sfInfo));
}

inline void XHYFileManager::flushHandler(FDMInfo& info, time_t modified)
{
	char *tmp = new char[sizeof(SafeInfo)];
	driver->getChunk(info.hdpos, tmp, sizeof(SafeInfo));
	SafeInfo* psi = (SafeInfo*) tmp;
	psi->modified = modified;
	driver->setChunk(info.hdpos, tmp, sizeof(SafeInfo));
}

int XHYFileManager::getFreeMFd()
{
	while(true)
	{
		int cr = _cmfd;
		++_cmfd;
		if(cr == 0)
			continue;
		if(_fdmmap.find(cr) == _fdmmap.end())
			return cr;
	}
}

int XHYFileManager::getFreeSFd()
{
	while(true)
	{
		int cr = _csfd;
		++_csfd;
		if(cr == 0)
			continue;
		if(_fdsmap.find(cr) == _fdsmap.end())
			return cr;
	}
}

bool XHYFileManager::fillCKInfo(std::vector<CKInfo>& vec, CKInfo& info)
{
	if(info.num > 0)
	{
		vec.push_back(info);
		return true;
	}
	if(!driver->getChunk(info.pos, bufHandlers[1].buf, chunkSize))
	{
		setErrno(ERR_SYS_LOADFAIL);
		return false;
	}
	ChunkHelper chpr(bufHandlers[1].buf, chunkSize);
	size_t cklen = chpr.getAPCKLen();
	CKInfo* ckInfo = chpr.getAPCKInfo();
	for(size_t i = 0; i < cklen; ++i)
		if(!fillCKInfo(vec, ckInfo[i]))
			return false;
	return true;
}

bool XHYFileManager::delNode(CKInfo* info)
{
	if(info->num == 0)
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
			if(!delNode(&ckInfo[i]))
				return false;
		releaseChunk(info->pos, 1);
	}
	else if(info->num > 0)
		releaseChunk(info->pos, info->num);
	return true;
}

bool XHYFileManager::tryLock(SafeInfo& sfInfo, fdtype_t type)
{
	if(type & OPTYPE_WTE_MTX_LOCK)
	{
		if(sfInfo.sign & FL_WRITE_MTXLOCK || sfInfo.mutexRead || sfInfo.shareRead || sfInfo.shareWrite)
			return false;
		sfInfo.sign |= FL_WRITE_MTXLOCK;
	}
	else if(type & OPTYPE_WTE_SHR_LOCK)
	{
		if(sfInfo.sign & FL_WRITE_MTXLOCK || sfInfo.mutexRead)
			return false;
		++sfInfo.shareWrite;
	}
	else if(type & OPTYPE_RAD_MTX_LOCK)
	{
		if(sfInfo.sign & FL_WRITE_MTXLOCK || sfInfo.shareWrite)
			return false;
		++sfInfo.mutexRead;
	}
	else if(type & OPTYPE_RAD_SHR_LOCK)
	{
		if(sfInfo.sign & FL_WRITE_MTXLOCK)
			return false;
		++sfInfo.shareRead;
	}
	return true;
}

bool XHYFileManager::isOwner(int fd)
{
	auto it = _fdsmap.find(fd);
	if(it == _fdsmap.end())
		return false;
	if(it->second->tid != std::this_thread::get_id())
		return false;
	return true;
}

bool XHYFileManager::appendFile(FDMInfo& mInfo, size_t len)
{
	std::vector<CKInfo> apVec;
	while(len)
	{
		if(mInfo.fileSize + sizeof(SafeInfo) + sizeof(INInfo) <= chunkSize)
		{
			if(mInfo.fileSize + sizeof(SafeInfo) + sizeof(INInfo) + len <= chunkSize)
			{
				mInfo.fileSize += len;
				IndexNode inode = loadINode(mInfo.hdpos, 0);
				inode.info->fileSize = mInfo.fileSize;
				flush();
				return true;
			}
			else
			{
				IndexNode inode = loadINode(mInfo.hdpos, 0);
				if(inode.cur == 0)
					return false;
				size_t ndckn = (len + mInfo.fileSize + chunkSize - 1) / chunkSize;
				cksize_t tckn;
				cpos_t st = getFreeChunk(ndckn, tckn);
				if(tckn == 0)
					return false;
				if(!load(st, 1))
					return false;
				memcpy(bufHandlers[1].buf, bufHandlers[0].buf + sizeof(SafeInfo) + sizeof(INInfo), mInfo.fileSize);
				* ((size_t*) (bufHandlers[0].buf + sizeof(SafeInfo) + sizeof(INInfo))) = 1;
				inode.nodes[0] = {tckn, st};
				apVec.push_back(inode.nodes[0]);
				size_t fiy = tckn * chunkSize < mInfo.fileSize + len ? tckn * chunkSize : mInfo.fileSize + len;
				size_t tap = fiy - mInfo.fileSize;
				mInfo.fileSize = fiy;
				len -= tap;
			}
		}
		else if(mInfo.fileSize % chunkSize != 0)
		{
			size_t tap = chunkSize - mInfo.fileSize % chunkSize < len ? chunkSize - mInfo.fileSize % chunkSize : len;
			mInfo.fileSize += tap;
			len -= tap;
		}
		else
		{
			IndexNode inode = loadINode(mInfo.hdpos, 0);
			if(inode.cur == 0)
				return false;
			size_t ndckn = (len + chunkSize - 1) / chunkSize;
			cksize_t tckn;
			cpos_t st = getFreeChunk(ndckn, tckn);
			if(tckn == 0)
				return false;
			if(inode.nodeSize * sizeof(CKInfo) + sizeof(SafeInfo) + sizeof(INInfo) + sizeof(size_t) + sizeof(CKInfo) <= chunkSize)
			{
				++(* ((size_t*) (bufHandlers[0].buf + sizeof(SafeInfo) + sizeof(INInfo))));
				inode.nodes[inode.nodeSize] = {tckn, st};
				apVec.push_back(inode.nodes[inode.nodeSize]);
			}
			else if(inode.nodes[inode.nodeSize - 1].num != 0)
			{
				cksize_t tg;
				cpos_t st2 = getFreeChunk(1, tg);
				if(tg == 0)
					return false;
				(* (AppendINode*) bufHandlers[1].buf).nodeSize = 2;
				bufHandlers[1].pos = st2;
				CKInfo* ckInfos = (CKInfo*) (bufHandlers[1].buf + sizeof(AppendINode));

				ckInfos[0] = inode.nodes[inode.nodeSize - 1];
				ckInfos[1] = {tckn, st};

				inode.nodes[inode.nodeSize - 1] = {0, st2};
				apVec.push_back(ckInfos[1]);
			}
			else
			{
				cpos_t apinPos = inode.nodes[inode.nodeSize - 1].pos;
				while(true)
				{
					if(!load(apinPos, 1))
						return false;
					AppendINode* papin = (AppendINode*) bufHandlers[1].buf;
					CKInfo* ckInfos = (CKInfo*) (bufHandlers[1].buf + sizeof(AppendINode));
					if(sizeof(AppendINode) + (papin->nodeSize + 1) * sizeof(CKInfo) <= chunkSize)
					{
						++papin->nodeSize;
						ckInfos[papin->nodeSize] = {tckn, st};
						apVec.push_back(ckInfos[papin->nodeSize]);
						break;
					}
					else if(ckInfos[papin->nodeSize - 1].num != 0)
					{
						cksize_t tg;
						cpos_t st2 = getFreeChunk(1, tg);
						if(tg == 0)
							return false;
						(* (AppendINode*) bufHandlers[2].buf).nodeSize = 2;
						bufHandlers[2].pos = st2;
						CKInfo* ckInfos = (CKInfo*) (bufHandlers[2].buf + sizeof(AppendINode));

						ckInfos[0] = inode.nodes[inode.nodeSize - 1];
						ckInfos[1] = {tckn, st};

						inode.nodes[inode.nodeSize - 1] = {0, st2};
						apVec.push_back(ckInfos[1]);
						break;
					}
					else
						apinPos = ckInfos[papin->nodeSize - 1].pos;
				}
			}
			size_t fiy = len < tckn * chunkSize ? len : tckn * chunkSize;
			len -= fiy;
			mInfo.fileSize += fiy;
		}
		flush();
	}
	IndexNode inode = loadINode(mInfo.hdpos, 0);
	inode.info->fileSize = mInfo.fileSize;
	flush();
	CKInfo* oldlist = mInfo.cklist;
	mInfo.cklist = new CKInfo[mInfo.cklistSize + apVec.size()];
	memcpy(mInfo.cklist, oldlist, sizeof(CKInfo) * mInfo.cklistSize);
	memcpy(mInfo.cklist + mInfo.cklistSize, apVec.data(), sizeof(CKInfo) * apVec.size());
	mInfo.cklistSize += apVec.size();
	delete[] oldlist;
	return true;
}

void XHYFileManager::_seek(FDMInfo& mInfo, FDSInfo& info, int offset, FPos pos)
{
	if(pos == XHYFileManager::cur && offset < 0)
	{
		size_t aboft = -offset;
		if(aboft >= info.abpos)
		{
			info.ckpos = 0;
			info.rlpos = 0;
			info.abpos = 0;
			return;
		}
		info.abpos -= aboft;
		if(mInfo.cklistSize == 0)
		{
			info.rlpos = info.abpos;
			return;
		}
		while(aboft)
		{
			if(aboft > info.rlpos)
			{
				aboft -= info.rlpos - 1;
				--info.ckpos;
				info.rlpos = chunkSize * mInfo.cklist[info.ckpos].num;
			}
			else
			{
				info.rlpos -= aboft;
				aboft = 0;
			}
		}
	}
	else if(pos == XHYFileManager::cur && offset >= 0)
	{
		size_t aboft = offset;
		if(mInfo.cklistSize == 0)
		{
			info.abpos += aboft;
			info.rlpos = info.abpos;
			return;
		}
		if(aboft + info.abpos >= mInfo.fileSize)
		{
			info.ckpos = mInfo.cklistSize - 1;
			info.abpos = mInfo.fileSize;
			info.rlpos = (mInfo.cklist[mInfo.cklistSize - 1].num - 1) * chunkSize
					+ mInfo.fileSize % chunkSize == 0 ? chunkSize : mInfo.fileSize % chunkSize;
			return;
		}
		info.abpos += aboft;
		while(aboft)
		{
			if(aboft + info.rlpos < mInfo.cklist[info.ckpos].num * chunkSize)
			{
				info.rlpos += aboft;
				aboft = 0;
			}
			else
			{
				aboft -= (mInfo.cklist[info.ckpos].num * chunkSize - info.rlpos );
				++info.ckpos;
				info.rlpos = 0;
			}
		}
	}
	else if(pos == XHYFileManager::beg)
	{
		info.ckpos = 0;
		info.rlpos = 0;
		info.abpos = 0;
		_seek(mInfo, info, offset, XHYFileManager::cur);
	}
	else
	{
		if(mInfo.cklistSize == 0)
		{
			info.ckpos = 0;
			info.rlpos = mInfo.fileSize;
			info.abpos = mInfo.fileSize;
			_seek(mInfo, info, offset, XHYFileManager::cur);
			return;
		}
		info.ckpos = mInfo.cklistSize - 1;
		info.abpos = mInfo.fileSize;
		info.rlpos = (mInfo.cklist[mInfo.cklistSize - 1].num - 1) * chunkSize
				+ mInfo.fileSize % chunkSize == 0 ? chunkSize : mInfo.fileSize % chunkSize;
		_seek(mInfo, info, offset, XHYFileManager::cur);
	}
}
