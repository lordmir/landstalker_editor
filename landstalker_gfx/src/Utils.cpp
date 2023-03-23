#include "Utils.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <iostream>
#endif

#include <fstream>
#include <iterator>
#include <algorithm>

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

std::vector<uint8_t> ReadBytes(const filesystem::path& filename)
{
	return ReadBytes(filename.str());
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

void WriteBytes(const std::vector<uint8_t>& data, const filesystem::path& filename)
{
	WriteBytes(data, filename.str());
}

bool IsHex(const std::string& str)
{
	return str.compare(0, 2, "0x") == 0
		&& str.size() > 2
		&& str.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
}

std::string Trim(const std::string& str)
{
	const auto whitespace = " \t";
	const auto start = str.find_first_not_of(whitespace);
	if (start == std::string::npos)
		return ""; // no content

	const auto end = str.find_last_not_of(whitespace);
	const auto len = end - start + 1;

	return str.substr(start, len);
}

std::string RemoveQuotes(const std::string& str)
{
	std::string s(Trim(str));
	s.erase(std::remove(s.begin(), s.end(), '\"'), s.end());
	return s;
}

std::string ReformatPath(const std::string& str)
{
	auto s = RemoveQuotes(str);
	// Convert Backslashes to forward slashes
	std::replace(s.begin(), s.end(), '\\', '/');
	return s;
}

bool CreateDirectoryTree(const filesystem::path& path)
{
	if (!path.parent_path().is_directory() &&
		!filesystem::create_directories(path.parent_path()))
	{
		throw std::runtime_error(std::string("Unable to create directory \"") + path.parent_path().str() + "\"");
	}
	return true;
}
