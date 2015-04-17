#ifndef _FMGUARD_H
#define _FMGUARD_H

#include "XHYFileManager.h"
//文件系统的线程守卫, 确保线程退出时释放掉Filemanager的相关资源
class FMGuard
{
	XHYFileManager* manager;
public:
	FMGuard(XHYFileManager* mgr);
	~FMGuard();
};

#endif