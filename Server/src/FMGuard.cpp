#include "FMGuard.h"

#include <thread>

FMGuard::FMGuard(XHYFileManager* mgr)
	: manager(mgr)
{
	manager->bindThread(std::this_thread::get_id());
}

FMGuard::~FMGuard()
{
	manager->releaseThread(std::this_thread::get_id());
}