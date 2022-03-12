#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <boost/filesystem.hpp>

void Debug(const std::string& message);

template<template<typename, typename, typename...> class C, typename T, typename U, typename... Rest>
T FindMapKey(const C<T, U, Rest...>& map, const U& value)
{
	for (auto it = map.begin(); it != map.end(); ++it)
	{
		if (it->second == value)
		{
			return it->first;
		}
	}
	return T();
}

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

template< template< typename, typename...> typename C, typename T, typename... Rest >
std::string HexArray(const C<T, Rest...>& container)
{
	return HexArray(container.begin(), container.end());
}

template< template< typename, std::size_t> typename C, typename T, std::size_t N >
std::string HexArray(const C<T, N>& container)
{
	return HexArray(container.begin(), container.end());
}

template<typename T, std::size_t N>
std::string HexArray(const T(&val)[N])
{
	const T* begin = val;
	const T* end = val + N;
	return HexArray(begin, end);
}

std::vector<uint8_t> ReadBytes(const std::string& filename);
std::vector<uint8_t> ReadBytes(const boost::filesystem::path& filename);

void WriteBytes(const std::vector<uint8_t>& data, const std::string& filename);
void WriteBytes(const std::vector<uint8_t>& data, const boost::filesystem::path& filename);

bool IsHex(const std::string& str);

std::string Trim(const std::string& str);

std::string RemoveQuotes(const std::string& str);

#endif // UTILS_H
