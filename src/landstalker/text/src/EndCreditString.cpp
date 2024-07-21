#include <landstalker/text/include/EndCreditString.h>

#include <stdexcept>
#include <sstream>
#include <cstring>

#include <landstalker/misc/include/Literals.h>

const LSString::CharacterSet ENDING_CHARSET = {
	{ 1_u8, L" "}, { 2_u8, L"A"}, { 3_u8, L"B"}, { 4_u8, L"C"}, { 5_u8, L"D"}, { 6_u8, L"E"}, { 7_u8, L"F"}, { 8_u8, L"G"},
	{ 9_u8, L"H"}, {10_u8, L"I"}, {11_u8, L"J"}, {12_u8, L"K"}, {13_u8, L"L"}, {14_u8, L"M"}, {15_u8, L"N"}, {16_u8, L"O"},
	{17_u8, L"P"}, {18_u8, L"Q"}, {19_u8, L"R"}, {20_u8, L"S"}, {21_u8, L"T"}, {22_u8, L"U"}, {23_u8, L"V"}, {24_u8, L"W"},
	{25_u8, L"X"}, {26_u8, L"Y"}, {27_u8, L"Z"}, {28_u8, L"a"}, {29_u8, L"b"}, {30_u8, L"c"}, {31_u8, L"d"}, {32_u8, L"e"},
	{33_u8, L"f"}, {34_u8, L"g"}, {35_u8, L"h"}, {36_u8, L"i"}, {37_u8, L"j"}, {38_u8, L"k"}, {39_u8, L"l"}, {40_u8, L"m"},
	{41_u8, L"n"}, {42_u8, L"o"}, {43_u8, L"p"}, {44_u8, L"q"}, {45_u8, L"r"}, {46_u8, L"s"}, {47_u8, L"t"}, {48_u8, L"u"},
	{49_u8, L"v"}, {50_u8, L"w"}, {51_u8, L"x"}, {52_u8, L"y"}, {53_u8, L"z"}, {54_u8, L"1"}, {55_u8, L"3"}, {56_u8, L"9"},
	{57_u8, L"(C)"}, {58_u8, L"(3)"}, {59_u8, L"-"}, {60_u8, L","}, {61_u8, L"."},
	{64_u8, L"{K1}"}, {65_u8, L"{K2}"}, {66_u8, L"{K3}"}, {67_u8, L"{K4}"},
	{128_u8, L"_"}, {129_u8, L"{UL1}" }, { 130_u8, L"{UL2}" },
	{131_u8, L"{SEGA_LOGO}" }, {132_u8, L"{CLIMAX_LOGO}"}, {133_u8, L"{DDS520_LOGO}"}, {134_u8, L"{MIRAGE_LOGO}"}
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
	m_height = static_cast<uint8_t>(std::stoi(cell));
	std::getline<EndCreditString::StringType::value_type>(liness, cell, '\t');
	m_column = -static_cast<int8_t>(std::stoi(cell));
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
