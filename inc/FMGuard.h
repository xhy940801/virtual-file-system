#ifndef _FMGUARD_H
#define _FMGUARD_H

#include "XHYFileManager.h"

class FMGuard
{
	XHYFileManager* manager;
public:
	FMGuard(XHYFileManager* mgr);
	~FMGuard();
};

#endif