#include "SandBox.h"

SandBox::SandBox(Shell* sh)
	: shell(sh)
{
}

SandBox::~SandBox()
{
}

void SandBox::run(const std::string &sh, size_t argc, const std::string* argv)
{
}