#ifndef _SANDBOX_H
#define _SANDBOX_H

#include "Shell.h"

class SandBox
{
	Shell* shell;
public:
	SandBox(Shell* sh);
	~SandBox();

	void run(const std::string &sh, size_t argc, const std::string* argv);

};

#endif