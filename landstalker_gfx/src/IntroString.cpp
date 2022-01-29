#include "IntroString.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cstring>

const LSString::CharacterSet INTRO_CHARSET = {
	{ 0, L" "}, { 1, L"A"}, { 2, L"B"}, { 3, L"C"}, { 4, L"D"}, { 5, L"E"}, { 6, L"F"}, { 7, L"G"},
	{ 8, L"H"}, { 9, L"I"}, {10, L"J"}, {11, L"K"}, {12, L"L"}, {13, L"M"}, {14, L"N"}, {15, L"O"},
	{16, L"P"}, {17, L"Q"}, {18, L"R"}, {19, L"S"}, {20, L"T"}, {21, L"U"}, {22, L"V"}, {23, L"W"},
	{24, L"X"}, {25, L"Y"}, {26, L"Z"}, {27, L"1"}, {28, L"2"}, {29, L"3"}
};

IntroString::IntroString()
	: LSString(INTRO_CHARSET),
	  m_line1Y(0),
	  m_line1X(0),
	  m_line2Y(0),
	  m_line2X(0),
	  m_displayTime(0)
{
}

IntroString::IntroString(uint16_t line1_y, uint16_t line1_x, uint16_t line2_y, uint16_t line2_x, uint16_t display_time, IntroString::StringType line1, IntroString::StringType line2)
	: LSString(m_str, INTRO_CHARSET),
	m_line1Y(line1_y),
	m_line1X(line1_x),
	m_line2Y(line2_y),
	m_line2X(line2_x),
	m_line2(line2),
	m_displayTime(display_time)
{
}

IntroString::IntroString(const IntroString::StringType& serialised)
	: LSString(INTRO_CHARSET),
	  m_line1Y(0),
	  m_line1X(0),
	  m_line2Y(0),
	  m_line2X(0),
	  m_displayTime(0)
{
	Deserialise(serialised);
}

IntroString::StringType IntroString::Serialise() const
{
	std::basic_ostringstream<IntroString::StringType::value_type> ss;
	ss << m_line1X << "\t";
	ss << m_line1Y << "\t";
	ss << m_line2X << "\t";
	ss << m_line2Y << "\t";
	ss << m_displayTime << "\t";
	ss << m_str << "\t";
	ss << m_line2;
	return ss.str();
}

void IntroString::Deserialise(const IntroString::StringType& in)
{
	std::basic_istringstream<IntroString::StringType::value_type> liness(in);
	IntroString::StringType cell;
	std::getline<IntroString::StringType::value_type>(liness, cell, '\t');
	m_line1X = std::stoi(cell);
	std::getline<IntroString::StringType::value_type>(liness, cell, '\t');
	m_line1Y = std::stoi(cell.c_str());
	std::getline<IntroString::StringType::value_type>(liness, cell, '\t');
	m_line2X = std::stoi(cell.c_str());
	std::getline<IntroString::StringType::value_type>(liness, cell, '\t');
	m_line2Y = std::stoi(cell.c_str());
	std::getline<IntroString::StringType::value_type>(liness, cell, '\t');
	m_displayTime = std::stoi(cell.c_str());
	std::getline<IntroString::StringType::value_type>(liness, m_str, '\t');
	std::getline<IntroString::StringType::value_type>(liness, m_line2, '\t');
}

IntroString::StringType IntroString::GetHeaderRow() const
{
	const char* ret = "Line1_X\tLine1_Y\tLine2_X\tLine2_Y\tDisplayTime\tLine1\tLine2";
	return IntroString::StringType(ret, ret + strlen(ret));
}

template<class T>
inline T ReadNum(const uint8_t* buffer, size_t size)
{
	if (size < sizeof(T))
	{
		throw std::runtime_error("Not enough bytes in buffer to decode string params");
	}
	T retval = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		retval <<= 8;
		retval |= buffer[i];
	}
	return retval;
}

template<class T>
inline size_t WriteNum(uint8_t* buffer, size_t size, T value)
{
	if (size < sizeof(uint16_t))
	{
		throw std::runtime_error("Not enough bytes in buffer to decode string params");
	}
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		buffer[i] = (value >> (sizeof(T) - i - 1) * 8) & 0xFF;
	}
	return sizeof(T);
}

size_t IntroString::Decode(const uint8_t* buffer, size_t size)
{
	m_line1Y = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_line1X = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_line2Y = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_line2X = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	m_displayTime = ReadNum<uint16_t>(buffer, size);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	return sizeof(uint16_t) * 5 + DecodeString(buffer, size);
}

size_t IntroString::Encode(uint8_t* buffer, size_t size) const
{
	WriteNum<uint16_t>(buffer, size, m_line1Y);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_line1X);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_line2Y);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_line2X);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	WriteNum<uint16_t>(buffer, size, m_displayTime);
	buffer += sizeof(uint16_t);
	size -= sizeof(uint16_t);
	return sizeof(uint16_t) * 5 + EncodeString(buffer, size);
}

uint16_t IntroString::GetLine1Y() const
{
	return m_line1Y;
}

uint16_t IntroString::GetLine1X() const
{
	return m_line1X;
}

uint16_t IntroString::GetLine2Y() const
{
	return m_line2Y;
}

uint16_t IntroString::GetLine2X() const
{
	return m_line2X;
}

uint16_t IntroString::GetDisplayTime() const
{
	return m_displayTime;
}

IntroString::StringType IntroString::GetLine(int line) const
{
	if (line == 0)
	{
		return m_str;
	}
	else
	{
		return m_line2;
	}
}

size_t IntroString::DecodeString(const uint8_t* string, size_t len)
{
	m_str.clear();
	size_t i = 0;
	while (string[i] != 0xFF && i < 32)
	{
		if (i < 16)
		{
			m_str += DecodeChar(string[i++]);
		}
		else
		{
			m_line2 += DecodeChar(string[i++]);
		}
		if(i >= len)
		{
			throw std::runtime_error("Not enough bytes in buffer to decode string");
		}
	}
	return i + 1;
}

size_t IntroString::EncodeString(uint8_t* string, size_t len) const
{
	size_t i = 0;
	size_t j = 0;
	while (j < m_str.size() && i < len)
	{
		j += EncodeChar(m_str, j, string[i++]);
	}
	if (m_line2.empty() == false)
	{
		while (i < 16)
		{
			const char* space = " ";
			EncodeChar(IntroString::StringType(space, space + strlen(space)), 0, string[i++]);
		}
		j = 0;
		while (j < m_line2.size() && i < len)
		{
			j += EncodeChar(m_line2, j, string[i++]);
		}
	}
	string[i] = 0xFF;
	return i + 1;
}
