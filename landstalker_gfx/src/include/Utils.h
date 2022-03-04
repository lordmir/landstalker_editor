#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

void Debug(const std::string& message);

template< class T >
std::string Hex(const T& val)
{
	std::ostringstream ss;
	ss << "0x" << std::hex << std::uppercase << std::setw(sizeof(T)*2) << std::setfill('0') << static_cast<unsigned>(val);
	return ss.str();
}

template< class Iter >
std::string HexArray(const Iter start, const Iter end)
{
	std::ostringstream ss;
	for (auto it = start; it != end; ++it)
	{
		ss << Hex(*it);
		if ((it + 1) != end)
		{
			ss << ", ";
		}
	}
	return ss.str();
}

std::vector<uint8_t> ReadBytes(const std::string& filename);

void WriteBytes(const std::vector<uint8_t>& data, const std::string& filename);

#endif // UTILS_H
