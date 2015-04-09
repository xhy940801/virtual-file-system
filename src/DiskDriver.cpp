#include "VirtualDiskDriver.h"
#include "DiskDriver.h"

#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>
using namespace std;

static bool createFile(const char* path, size_t size)
{
	ofstream of(path, ios::binary);
	if(!of)
		return false;
	char c = '\0';
	while(size--)
	{
		of.write(&c, 1);
		if(!of.good())
			return false;
	}
	of.close();
	return true;
}

static bool getCmdYOrN(const char * tips)
{
	cout << tips << endl;
	cout << "y(yes) or n(no)";

	char cmd;
	cin >> cmd;
	while(cmd != 'y' && cmd != 'Y' && cmd != 'n' && cmd != 'N')
	{
		cout << "Unknow command!" << endl;
		cout << tips << endl;
		cout << "y(yes) or n(no)";

		cin >> cmd;
	}

	if(cmd == 'y' || cmd == 'Y')
		return true;
	return false;
}

static size_t getFileSize(FILE* fp)
{
	if(fp == NULL)
		return -1; 
	fseek(fp, 0L, SEEK_END); 
	size_t len = (size_t)ftell(fp);
	fclose(fp);
	return len;
}

DiskDriver* getDiskDriver()
{
	ifstream cf("default.conf");

	if(!cf)
	{
		cout << "Please input the config file path" << endl;
		string path;
		cin >> path;
		cf.open(path);
		if(!cf)
		{
			cout << "File path error!" << endl;
			exit(0);
		}
	}

	string vspath;
	cf >> vspath;
	size_t chunkSize, totalChunkNumber;
	cf >> chunkSize >> totalChunkNumber;
	FILE* fd = fopen(vspath.c_str(), "rb+");
	if(fd == NULL)
	{
		if(!getCmdYOrN("File is not exit, create it?"))
			exit(0);
		createFile(vspath.c_str(), chunkSize * totalChunkNumber);
		fd = fopen(vspath.c_str(), "rb+");
	}
	if(getFileSize(fd) < chunkSize * totalChunkNumber)
	{
		cout << "Virtual Disk error!" << endl;
		exit(0);
	}
	fd = fopen(vspath.c_str(), "rb+");
	cf.close();
	return new DiskDriver(fd, chunkSize, totalChunkNumber);
}


void releaseDiskDriver(DiskDriver* driver)
{
	delete driver;
}