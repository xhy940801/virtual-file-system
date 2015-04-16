#ifndef _SANDBOX_H
#define _SANDBOX_H

class SandBox
{
	Shell* shell;
public:
	SandBox(Shell* sh);
	~SandBox();

	run(std::string sh, size_t argc, std::string* argv);

};

#endif