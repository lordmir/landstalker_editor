#ifndef _ASM_FILE_INL_H_
#define _ASM_FILE_INL_H_

#include <sstream>
#include <iomanip>
#include <iostream>
#include <array>
#include <algorithm>
#include <landstalker/main/include/AsmFile.h>
#include <landstalker/misc/include/Utils.h>

template<typename T>
AsmFile& AsmFile::operator<<(const T& data)
{
	if (!Write(data))
	{
		m_good = false;
	}
	return *this;
}

template<typename T>
AsmFile& AsmFile::operator>>(T& data)
{
	if (!Read(data))
	{
		m_good = false;
		throw(std::runtime_error("Read failed"));
	}
	return *this;
}

template<typename T>
AsmFile& AsmFile::operator>>(const T& data)
{
	if (!Read(data))
	{
		m_good = false;
	}
	return *this;
}

template<typename T, typename... Args>
bool AsmFile::Read(T& value, Args&&... args)
{
	return Read(value) && Read(args...);
}

template<typename T>
bool AsmFile::Read(T& value)
{
	value = 0;
	try
	{
		for (std::size_t i = 0; i < sizeof(T); ++i)
		{
			if (m_readptr == m_data.end())
			{
				m_good = false;
				return false;
			}
			value <<= 4;
			value <<= 4;
			value |= std::get<uint8_t>(*m_readptr++);
		}
	}
	catch (const std::bad_variant_access&)
	{
		return false;
	}
	return true;
}

template<>
bool AsmFile::Read(std::string& value);

template<>
bool AsmFile::Read(IncludeFile& value);

template<>
bool AsmFile::Read(Label& label);

template<>
bool AsmFile::Read(std::filesystem::path& label);

template<>
bool AsmFile::Read(ScriptAction& action);

template<>
bool AsmFile::Read(Instruction& inst);

template<template<typename, typename...> class C, typename T, typename... Rest>
bool AsmFile::Read(C<T, Rest...>& container)
{
	bool ret = true;
	for (auto& e : container)
	{
		ret = Read<T>(e);
		if (ret == false)
		{
			break;
		}
	}
	return ret;
}

template<typename T, std::size_t N>
bool AsmFile::Read(std::array<T, N>& container)
{
	bool ret = true;
	for (auto& e : container)
	{
		ret = Read<T>(e);
		if (ret == false)
		{
			break;
		}
	}
	return ret;
}

template<typename T, std::size_t N>
bool AsmFile::Read(T(&array)[N])
{
	bool ret = true;
	for (std::size_t i = 0; i < N; i++)
	{
		ret = Read<T>(array[i]);
		if (ret == false)
		{
			break;
		}
	}
	return ret;
}

template<typename T>
bool AsmFile::Write(const T& value)
{
	Width w = Width::L;
	if constexpr (std::is_integral<T>::value && sizeof(T) < 4)
	{
		w = static_cast<Width>(sizeof(T));
	}
	if (!m_nextline.instruction.empty())
	{
		PushNextLine();
	}
	m_nextline.instruction = FindMapKey(INSTRUCTIONS, Inst::DC)->first;
	m_nextline.width = FindMapKey(WIDTHS, w)->first;
	m_nextline.operand = ToAsmValue(value);
	return true;
}

template<std::size_t N>
inline bool AsmFile::Write(const char(&data)[N])
{
	return Write(std::string(data));
}

template<typename T, typename ...Args>
inline bool AsmFile::Write(const T& first, Args && ...args)
{
	return Write(first) && Write(args...);
}

template<typename Iter>
inline bool AsmFile::Write(Iter begin, Iter end)
{
	Width w = Width::L;
	if constexpr (sizeof(decltype(*begin)) < 4)
	{
		w = static_cast<Width>(sizeof(decltype(*begin)));
	}
	Iter start = begin;
	Iter next = begin;
	while (start != end)
	{
		if (!m_nextline.instruction.empty())
		{
			PushNextLine();
		}
		m_nextline.instruction = FindMapKey(INSTRUCTIONS, Inst::DC)->first;
		m_nextline.width = FindMapKey(WIDTHS, w)->first;

		for (std::size_t i = 0; i < MAX_ELEMENTS_ON_LINE; ++i)
		{
			next++;
			if (next == end)
			{
				break;
			}
		}

		m_nextline.operand = ToAsmValue(start, next);
		start = next;
	}
	return true;
}

template<typename T, std::size_t N>
inline bool AsmFile::Write(const T(&val)[N])
{
	const T* begin = val;
	const T* end = val + N;
	return Write(begin, end);
}

template<template <typename, typename ...> typename C, typename T, typename... Rest>
inline bool AsmFile::Write(const C<T, Rest...>& container)
{
	return Write(container.begin(), container.end());
}

template<template <typename, std::size_t> typename C, typename T, std::size_t N>
inline bool AsmFile::Write(const C<T, N>& container)
{
	return Write(container.begin(), container.end());
}

template <class T>
std::string AsmFile::ToAsmValue(T value, AsmFile::Base base)
{
	const char bases[] = "  %     @       $";
	const char digits[] = "0123456789ABCDEF";
	std::unordered_map<Base, std::array<int, 6>> places_u =
	{
		// Width     N  B   W       L  S
		{Base::BIN, {0, 8, 16, 24, 32, 8}},
		{Base::OCT, {0, 3,  6,  8, 11, 3}},
		{Base::DEC, {0, 3,  5,  8, 10, 3}},
		{Base::HEX, {0, 2,  4,  6,  8, 2}}
	};
	std::unordered_map<Base, std::array<int, 6>> places_s =
	{
		// Width     N  B   W       L  S
		{Base::BIN, {0, 7, 15, 23, 31, 7}},
		{Base::OCT, {0, 3,  5,  8, 11, 3}},
		{Base::DEC, {0, 3,  5,  7, 10, 3}},
		{Base::HEX, {0, 2,  4,  6,  8, 2}}
	};
	std::string num;
	bool neg = false;
	int places = 0;
	std::size_t width = std::min(sizeof(T), sizeof(uint32_t));
	uint64_t v;
	if (value < 0)
	{
		neg = true;
		v = static_cast<uint64_t>(-static_cast<int64_t>(value));
		places = places_s[base][width];
	}
	else
	{
		v = static_cast<uint64_t>(value);
		places = places_u[base][width];
	}
	uint64_t max = 1ULL;
	max <<= width * 8 - (neg ? 1 : 0);
	max -= neg ? 0 : 1;
	if (v > max)
	{
		v = max;
	}

	for (int i = 0; i < places; i++)
	{
		num += digits[v % static_cast<int>(base)];
		v /= static_cast<int>(base);
	}
	if (num.empty())
	{
		num = '0';
	}
	if (bases[static_cast<int>(base)] != ' ')
	{
		num += bases[static_cast<int>(base)];
	}
	if (neg)
	{
		num += '-';
	}
	std::reverse(num.begin(), num.end());
	return num;
}

template<typename T>
std::string AsmFile::ToAsmValue(const T& value)
{
	return std::string(value);
}

template<std::size_t N>
std::string AsmFile::ToAsmValue(const char(&val)[N])
{
	return std::string(val);
}

template<typename Iter>
std::string AsmFile::ToAsmValue(Iter begin, Iter end)
{
	std::string str = "";
	while (begin != end)
	{
		str += ToAsmValue(*begin++);
		if (begin != end)
		{
			str += ", ";
		}
	}
	return str;
}

template<typename T, std::size_t N>
std::string AsmFile::ToAsmValue(const T(&val)[N])
{
	const T* begin = val;
	const T* end = val + N;
	return ToAsmValue(begin, end);
}

template<template <typename, typename ...> typename C, typename T, typename... Rest>
std::string AsmFile::ToAsmValue(const C<T, Rest...>& container)
{
	return ToAsmValue(container.begin(), container.end());
}

template<template <typename, std::size_t> typename C, typename T, std::size_t N>
std::string AsmFile::ToAsmValue(const C<T, N>& val)
{
	return ToAsmValue(val.begin(), val.end());
}

#endif // _ASM_FILE_INL_H_