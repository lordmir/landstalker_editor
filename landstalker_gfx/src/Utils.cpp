#include "Utils.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <iostream>
#endif

void Debug(const std::string& message)
{
#if defined _WIN32 && defined _DEBUG
	OutputDebugStringA(message.c_str());
	OutputDebugStringA("\n");
#elif !defined NDEBUG
	std::cout << message << std::endl;
#endif
}

