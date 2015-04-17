#ifndef _DISKDRIVER_H
#define _DISKDRIVER_H

#include "VirtualDiskDriver.h"

typedef VirtualDiskDriver DiskDriver;

//获取虚拟硬盘驱动
DiskDriver* getDiskDriver();
//释放虚拟硬盘
void releaseDiskDriver(DiskDriver* driver);

#endif