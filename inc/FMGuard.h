#ifndef _FMGUARD_H
#define _FMGUARD_H

class FMGuard
{
	XHYFileManager* manager;
public:
	FMGuard(XHYFileManager* mgr);
	~FMGuard();
};

#endif