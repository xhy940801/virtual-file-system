#ifndef _FILE_GUARD
#define _FILE_GUARD

//文件守卫, RAII相关, 用来确保文件描述符最终都会被关闭
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