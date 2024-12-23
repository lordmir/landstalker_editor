#include <landstalker/misc/include/Utils.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <iostream>
#endif

#include <fstream>
#include <iterator>
#include <algorithm>
#include <codecvt>

#ifndef NDEBUG
void Debug(const std::string& message)
{
#if defined _WIN32
	OutputDebugStringA(message.c_str());
	OutputDebugStringA("\n");
#else
	std::cout << message << std::endl;
#endif
}
#else
void Debug(const std::string&)
{
}
#endif

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

std::vector<uint8_t> ReadBytes(const std::filesystem::path& filename)
{
	return ReadBytes(filename.string());
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

void WriteBytes(const std::vector<uint8_t>& data, const std::filesystem::path& filename)
{
	WriteBytes(data, filename.string());
}

bool IsHex(const std::string& str)
{
	return str.compare(0, 2, "0x") == 0
		&& str.size() > 2
		&& str.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
}

std::string Trim(const std::string& str)
{
	const auto whitespace = " \t\r";
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

bool StrToHex(const std::string& s, uint32_t& val)
{
	bool valid = false;
	val = 0;
	for (char c : s)
	{
		if (c >= '0' && c <= '9')
		{
			val <<= 4;
			val |= (c - '0');
			valid = true;
		}
		else if (c >= 'a' && c <= 'f')
		{
			val <<= 4;
			val |= (c - 'a' + 10);
			valid = true;
		}
		else if (c >= 'A' && c <= 'F')
		{
			val <<= 4;
			val |= (c - 'A' + 10);
			valid = true;
		}
	}
	return valid;
}

bool CreateDirectoryTree(const std::filesystem::path& path)
{
	if (!std::filesystem::is_directory(path.parent_path()) &&
		!std::filesystem::create_directories(path.parent_path()))
	{
		throw std::runtime_error(std::string("Unable to create directory \"") + path.parent_path().string() + "\"");
	}
	return true;
}

bool StrToInt(const std::string& s, uint32_t& val)
{
	try
	{
		auto pos1 = s.find_first_of('x');
		auto pos2 = s.find_first_of('X');
		if (pos1 != std::string::npos || pos2 != std::string::npos)
		{
			val = std::stoi(s, nullptr, 16);
		}
		else
		{
			val = std::stoi(s);
		}
		return true;
	}
	catch (...)
	{
	}
	return false;
}

std::wstring utf8_to_wstr(const std::string& utf8)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	return cvt.from_bytes(utf8);
}

std::string wstr_to_utf8(const std::wstring& unicode)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	return cvt.to_bytes(unicode);
}

std::string str_to_lower(const std::string& str)
{
	std::string result(str);
	std::transform(str.begin(), str.end(), result.begin(),
		[](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });

	return result;
}
