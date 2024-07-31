#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cassert>
#include <cstdint>
#include <wjakob/filesystem/path.h>

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

template<typename... Args>
std::string StrPrintf(const std::string& fmt, Args... args)
{
	int reqd = std::snprintf(nullptr, 0, fmt.c_str(), args...);
	std::vector<char> buf(reqd + 1);
	std::snprintf(buf.data(), buf.size(), fmt.c_str(), args...);
	return std::string(buf.data(), buf.data() + reqd);
}

std::vector<uint8_t> ReadBytes(const std::string& filename);
std::vector<uint8_t> ReadBytes(const filesystem::path& filename);

void WriteBytes(const std::vector<uint8_t>& data, const std::string& filename);
void WriteBytes(const std::vector<uint8_t>& data, const filesystem::path& filename);

bool IsHex(const std::string& str);

std::string Trim(const std::string& str);

std::string RemoveQuotes(const std::string& str);

std::string ReformatPath(const std::string& str);

bool StrToHex(const std::string& s, uint32_t& val);

bool CreateDirectoryTree(const filesystem::path& path);

bool StrToInt(const std::string& s, uint32_t& val);

template<class T, class U>
std::vector<T> Split(U data)
{
	assert(sizeof(T) <= sizeof(U));
	assert(sizeof(U) % sizeof(T) == 0);
	std::vector<T> ret;
	int elems = sizeof(U) / sizeof(T);
	const U mask = (1 << (8 * sizeof(T))) - 1;
	ret.resize(elems);
	for (int i = 0; i < elems; ++i)
	{
		ret[elems - i - 1] = static_cast<T>(data & mask);
		data >>= (sizeof(T) * 8);
	}
	return ret;
}

template<class T, class Iter>
Iter Insert(Iter it, T data)
{
	assert(sizeof(typename Iter::value_type) <= sizeof(T));
	assert(sizeof(T) % sizeof(typename Iter::value_type) == 0);
	int elems = sizeof(T) / sizeof(typename Iter::value_type);
	const T mask = (1 << (8 * sizeof(typename Iter::value_type))) - 1;
	it += elems;
	auto wit = it;
	for (int i = 0; i < elems; ++i)
	{
		*--wit = static_cast<typename Iter::value_type>(data & mask);
		data >>= (sizeof(typename Iter::value_type) * 8);
	}
	return it;
}

#endif // UTILS_H
