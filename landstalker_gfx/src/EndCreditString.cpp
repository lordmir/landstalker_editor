#include "EndCreditString.h"
#include <stdexcept>
#include <sstream>
#include <cstring>

const LSString::CharacterSet ENDING_CHARSET = {
	{ 1, L" "}, { 2, L"A"}, { 3, L"B"}, { 4, L"C"}, { 5, L"D"}, { 6, L"E"}, { 7, L"F"}, { 8, L"G"},
	{ 9, L"H"}, {10, L"I"}, {11, L"J"}, {12, L"K"}, {13, L"L"}, {14, L"M"}, {15, L"N"}, {16, L"O"},
	{17, L"P"}, {18, L"Q"}, {19, L"R"}, {20, L"S"}, {21, L"T"}, {22, L"U"}, {23, L"V"}, {24, L"W"},
	{25, L"X"}, {26, L"Y"}, {27, L"Z"}, {28, L"a"}, {29, L"b"}, {30, L"c"}, {31, L"d"}, {32, L"e"},
	{33, L"f"}, {34, L"g"}, {35, L"h"}, {36, L"i"}, {37, L"j"}, {38, L"k"}, {39, L"l"}, {40, L"m"},
	{41, L"n"}, {42, L"o"}, {43, L"p"}, {44, L"q"}, {45, L"r"}, {46, L"s"}, {47, L"t"}, {48, L"u"},
	{49, L"v"}, {50, L"w"}, {51, L"x"}, {52, L"y"}, {53, L"z"}, {54, L"1"}, {55, L"3"}, {56, L"9"},
	{57, L"(C)"}, {58, L"(3)"}, {59, L"-"}, {60, L","}, {61, L"."},
	{64, L"{K1}"}, {65, L"{K2}"}, {66, L"{K3}"}, {67, L"{K4}"},
	{128, L"_"}, {129, L"{UL1}" }, { 130, L"{UL2}" },
	{131, L"{SEGA_LOGO}" }, {132, L"{CLIMAX_LOGO}"}, {133, L"{DDS520_LOGO}"}, {134, L"{MIRAGE_LOGO}"}
};

EndCreditString::EndCreditString()
	: LSString(ENDING_CHARSET),
	m_height(0),
	m_column(-1)
{
}

EndCreditString::EndCreditString(int8_t fmt0, int8_t fmt1, const EndCreditString::StringType& str)
	: LSString(str, ENDING_CHARSET),
	m_height(fmt0),
	m_column(fmt1)
{
}

EndCreditString::EndCreditString(const EndCreditString::StringType& serialised)
	: LSString(ENDING_CHARSET),
	m_height(0),
	m_column(-1)
{
	Deserialise(serialised);
}

bool EndCreditString::operator==(const EndCreditString& rhs) const
{
	return ((this->m_column == rhs.m_column) &&
		    (this->m_height == rhs.m_height) &&
		    (this->m_str == rhs.m_str));
}

bool EndCreditString::operator!=(const EndCreditString& rhs) const
{
	return !(*this == rhs);
}

size_t EndCreditString::Decode(const uint8_t* buffer, size_t size)
{
	size_t ret = 0;
	if (size < 2)
	{
		throw std::runtime_error("Not enough bytes to decode string");
	}
	m_height = *reinterpret_cast<const int8_t*>(buffer++);
	m_column = *reinterpret_cast<const int8_t*>(buffer++);
	ret += 2;
	size -= 2;
	if (m_height != -1)
	{
		if (m_column < 0)
		{
			ret += DecodeString(buffer, size);
		}
		else
		{
			ret++;
			size--;
			m_str = DecodeChar(*buffer++);
		}
	}
	return ret;
}

size_t EndCreditString::Encode(uint8_t* buffer, size_t size) const
{
	size_t ret = 0;
	if (size < 2)
	{
		throw std::runtime_error("Not enough bytes to encode string");
	}
	*buffer++ = *reinterpret_cast<const uint8_t*>(&m_height);
	*buffer++ = *reinterpret_cast<const uint8_t*>(&m_column);
	size -= 2;
	ret += 2;
	if (m_height != -1)
	{
		if (m_column < 0)
		{
			ret += EncodeString(buffer, size);
		}
		else
		{
			ret++;
			EncodeChar(m_str, 0, *buffer++);
		}
	}
	return ret;
}

EndCreditString::StringType EndCreditString::Serialise() const
{
	std::basic_ostringstream<EndCreditString::StringType::value_type> ss;
	ss << static_cast<int>(m_height) << "\t";
	ss << -static_cast<int>(m_column) << "\t";
	ss << m_str;
	return ss.str();
}

void EndCreditString::Deserialise(const EndCreditString::StringType& in)
{
	std::basic_istringstream<EndCreditString::StringType::value_type> liness(in);
	EndCreditString::StringType cell;
	std::getline<EndCreditString::StringType::value_type>(liness, cell, '\t');
	m_height = std::stoi(cell);
	std::getline<EndCreditString::StringType::value_type>(liness, cell, '\t');
	m_column = -std::stoi(cell);
	std::getline<EndCreditString::StringType::value_type>(liness, m_str, '\t');
}

EndCreditString::StringType EndCreditString::GetHeaderRow() const
{
	const char* ret = "Height\tColumn\tString";
	return EndCreditString::StringType(ret, ret + strlen(ret));
}

int8_t EndCreditString::GetHeight() const
{
	return m_height;
}

int8_t EndCreditString::GetColumn() const
{
	return m_column;
}

void EndCreditString::SetHeight(int8_t val)
{
	m_height = val;
}

void EndCreditString::SetColumn(int8_t val)
{
	m_column = val;
}

void EndCreditString::SetStr(const StringType& str)
{
	m_str = str;
}

size_t EndCreditString::DecodeString(const uint8_t* string, size_t len)
{
	m_str.clear();
	const uint8_t* c = string;
	while (*c != 0x00)
	{
		m_str += DecodeChar(*c++);
		if (static_cast<size_t>(c - string) >= len)
		{
			throw std::runtime_error("Not enough bytes in buffer to decode string");
		}
	}
	return c - string + 1;
}

size_t EndCreditString::EncodeString(uint8_t* string, size_t len) const
{
	size_t i = 0;
	size_t j = 0;
	while (j < m_str.size() && i < len - 1)
	{
		j += EncodeChar(m_str, j, string[i++]);
	}
	string[i] = 0x00;
	return i + 1;
}
