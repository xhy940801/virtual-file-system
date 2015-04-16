#include <thread>
#include <iostream>
#include <unistd.h>
#include <string>
#include <sstream>

#include "DiskDriver.h"
#include "XHYFileManager.h"
#include "dispenser.h"

using namespace std;

int main()
{
	DiskDriver* driver = getDiskDriver();
	XHYFileManager* manager = new XHYFileManager(driver);

	startServer(manager);

	delete manager;
	releaseDiskDriver(driver);
}