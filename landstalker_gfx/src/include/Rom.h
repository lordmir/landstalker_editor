#ifndef ROM_H
#define ROM_H

#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <exception>
#include <vector>
#include <string>
#include <vector>
#include <cstdint>
#include <Utils.h>

#include "RomOffsets.h"

class Rom
{
public:

	explicit Rom(const std::string filename);
	explicit Rom(const std::vector<uint8_t>& arr);
	Rom(const Rom& rhs);
	Rom();

	Rom& operator=(const Rom& rhs);
	void load_from_file(std::string filename);
	void writeFile(const std::string& filename);

	template< class T >
	T read(std::size_t offset) const;
	template< class T >
	T read(const std::string& name) const;
	template< class T >
	T inc_read(std::size_t& offset) const;
	template<class T>
	std::vector<T> read_array(std::size_t offset, std::size_t count) const;
	template<class T>
	std::vector<T> read_array(const std::string& name) const;

	template< class T >
	void write(const T& data, std::size_t offset);
	template< class T >
	void write(const T& data, const std::string& name);
	template< class T >
	void inc_write(const T& data, std::size_t& offset);
	template<class Iter>
	void write_array(Iter begin, Iter end, std::size_t offset);
	template<class Iter>
	void write_array(Iter begin, Iter end, const std::string& name);
	template<template <typename T, typename... Rest> typename C, typename T, typename... Rest>
	void write_array(const C<T, Rest...>& container, std::size_t offset);
	template<template <typename T, typename... Rest> typename C, typename T, typename... Rest>
	void write_array(const C<T, Rest...>& container, const std::string& name);
	template<typename T, std::size_t N>
	void write_array(const T(&arr)[N], std::size_t offset);
	template<typename T, std::size_t N>
	void write_array(const T(&arr)[N], const std::string& name);
	template<template <typename, std::size_t> typename C, typename T, std::size_t N>
	void write_array(const C<T, N>& arr, std::size_t offset);
	template<template <typename, std::size_t> typename C, typename T, std::size_t N>
	void write_array(const C<T, N>& arr, const std::string& name);


	template< class T >
	T deref(std::size_t address, std::size_t offset = 0) const
	{
		return read<T>(read<uint32_t>(address) + offset * sizeof(T));
	}

	std::size_t get_address(const std::string& name) const;
	RomOffsets::Section get_section(const std::string& name) const;
	const uint8_t* data(std::size_t address = 0) const;
	std::size_t size(std::size_t address = 0) const;
	void resize(std::size_t amt);
	std::string get_description() const;
	RomOffsets::Region get_region() const;
	const std::vector<uint8_t>& get_vec() const;

	static bool section_exists(const std::string& name);
	static bool address_exists(const std::string& name);

private:
	void ValidateRomChecksum();
	void FixRomChecksum();

	std::string m_filename;
	bool m_initialised;
	std::vector<uint8_t> m_rom;
	RomOffsets::Region m_region;
};

template< class T >
inline T Rom::read(std::size_t offset) const
{
	T retval = 0;
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to read from uninitialised ROM.");
	}
	for (std::size_t i = 0; i < sizeof(T); ++i)
	{
		retval <<= (8 % (8 * sizeof(T)));
		retval |= m_rom[offset + i];
	}
	return retval;
}

template<>
inline bool Rom::read<bool>(std::size_t offset) const
{
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to read from uninitialised ROM.");
	}
	return (m_rom[offset] > 0);
}

template<class T>
inline T Rom::read(const std::string& name) const
{
	auto addrs = RomOffsets::ADDRESS.find(name);
	if (addrs == RomOffsets::ADDRESS.end())
	{
		throw std::runtime_error("Named address " + name + " does not exist!");
	}
	auto addr = addrs->second.find(m_region);
	if (addrs == RomOffsets::ADDRESS.end())
	{
		throw std::runtime_error("Named address " + name + " does not exist for " + RomOffsets::REGION_NAMES.find(m_region)->second + " ROM!");
	}
	return read<T>(addr->second);
}

template< class T >
inline T Rom::inc_read(std::size_t& offset) const
{
	auto ret = read<T>(offset);
	offset += sizeof(T);
	return ret;
}

template<class T>
inline std::vector<T> Rom::read_array(size_t offset, std::size_t count) const
{
	std::vector<T> ret;
	ret.reserve(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		ret.push_back(read<T>(offset + i * sizeof(T)));
	}
	return ret;
}

template<>
inline std::vector<bool> Rom::read_array(std::size_t offset, std::size_t count) const
{
	std::vector<bool> ret;
	ret.reserve(count);
	for (std::size_t i = 0; i < count; ++i)
	{
		ret.push_back(read<bool>(offset + i));
	}
	return ret;
}

template<class T>
inline std::vector<T> Rom::read_array(const std::string& name) const
{
	auto secs = RomOffsets::SECTION.find(name);
	if (secs == RomOffsets::SECTION.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist!");
	}
	auto sec = secs->second.find(m_region);
	if (sec == secs->second.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist for " + RomOffsets::REGION_NAMES.find(m_region)->second + " ROM!");
	}
	return read_array<T>(sec->second.begin, sec->second.size() / sizeof(T));
}

template<>
inline std::vector<bool> Rom::read_array<bool>(const std::string& name) const
{
	auto secs = RomOffsets::SECTION.find(name);
	if (secs == RomOffsets::SECTION.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist!");
	}
	auto sec = secs->second.find(m_region);
	if (sec == secs->second.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist for " + RomOffsets::REGION_NAMES.find(m_region)->second + " ROM!");
	}
	return read_array<bool>(sec->second.begin, sec->second.size());
}

template<class T>
inline void Rom::write(const T& data, std::size_t offset)
{
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to write to uninitialised ROM.");
	}
	for (std::size_t i = 0; i < sizeof(T); ++i)
	{
		m_rom[offset++] = (data >> ((sizeof(T) - i - 1) * 8)) & 0xFF;
	}
}

template<class T>
inline void Rom::write(const T& data, const std::string& name)
{
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to write to uninitialised ROM.");
	}
	std::size_t offset = get_address(name);
	write<T>(data, offset);
}

template<class T>
inline void Rom::inc_write(const T& data, std::size_t& offset)
{
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to write to uninitialised ROM.");
	}
	write<T>(data, offset);
	offset += sizeof(T);
}

template<class Iter>
inline void Rom::write_array(Iter begin, Iter end, std::size_t offset)
{
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to write to uninitialised ROM.");
	}
	while (begin != end)
	{
		inc_write(*begin++, offset);
	}
}

template<class Iter>
inline void Rom::write_array(Iter begin, Iter end, const std::string& name)
{
	std::size_t offset = get_address(name);
	write_array(begin, end, offset);
}

template<template <typename T, typename... Rest> typename C, typename T, typename... Rest>
inline void Rom::write_array(const C<T, Rest...>& container, std::size_t offset)
{
	write_array(container.begin(), container.end(), offset);
}

template<template <typename T, typename... Rest> typename C, typename T, typename... Rest>
inline void Rom::write_array(const C<T, Rest...>& container, const std::string& name)
{
	std::size_t offset = get_address(name);
	write_array(container, offset);
}

template<typename T, std::size_t N>
inline void Rom::write_array(const T(&arr)[N], std::size_t offset)
{
	const T* begin = arr;
	const T* end = arr + N;
	write_array(begin, end, offset);
}

template<typename T, std::size_t N>
inline void Rom::write_array(const T(&arr)[N], const std::string& name)
{
	std::size_t offset = get_address(name);
	write_array(arr, offset);
}

template<template <typename, std::size_t> typename C, typename T, std::size_t N>
inline void Rom::write_array(const C<T, N>& arr, const std::string& name)
{
	write_array(arr.begin(), arr.end(), get_address(name));
}

template<template <typename, std::size_t> typename C, typename T, std::size_t N>
inline void Rom::write_array(const C<T, N>& arr, std::size_t offset)
{
	write_array(arr.begin(), arr.end(), offset);
}

#endif // ROM_H
