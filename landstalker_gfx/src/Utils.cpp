#include "Utils.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <iostream>
#endif

#include <fstream>
#include <iterator>

void Debug(const std::string& message)
{
#if defined _WIN32 && defined _DEBUG
	OutputDebugStringA(message.c_str());
	OutputDebugStringA("\n");
#elif !defined NDEBUG
	std::cout << message << std::endl;
#endif
}

std::vector<uint8_t> ReadBytes(const std::string& filename)
{
	std::vector<uint8_t> ret;

	std::ifstream file(filename, std::ios::binary);
	if (!file.good())
	{
		throw std::runtime_error("Unable to open file for reading");
	}

	// stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	std::streampos fileSize;
	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// reserve capacity
	ret.reserve(static_cast<unsigned int>(fileSize));

	// read the data:
	ret.insert(ret.begin(),
		std::istream_iterator<uint8_t>(file),
		std::istream_iterator<uint8_t>());

	return ret;
}

void WriteBytes(const std::vector<uint8_t>& data, const std::string& filename)
{
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (!file.good())
	{
		throw std::runtime_error("Unable to open file for writing");
	}
	file.write(reinterpret_cast<const char*>(&data[0]), data.size());
}

