#ifndef _DISKDRIVER_H
#define _DISKDRIVER_H

#include "VirtualDiskDriver.h"

typedef VirtualDiskDriver DiskDriver;


DiskDriver* getDiskDriver();
void releaseDiskDriver(DiskDriver* driver);

#endif