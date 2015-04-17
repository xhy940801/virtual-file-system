#ifndef _FILE_GUARD
#define _FILE_GUARD
//守卫类类似lock_guard
template <typename MGR, typename FILE>
class FileGuard
{
	MGR& manager;
	FILE fd;
public:
	FileGuard(MGR& mgr, FILE _fd) : manager(mgr), fd(_fd) {}
	~FileGuard() { manager.close(fd); }
	
};

#endif