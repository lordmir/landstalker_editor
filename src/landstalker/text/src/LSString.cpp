#include <landstalker/text/include/LSString.h>

#include <stdexcept>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include <landstalker/text/include/Charset.h>
#include <landstalker/misc/include/Utils.h>

const LSString::CharacterSet& LSString::DEFAULT_CHARACTER_SET = Charset::DEFAULT_ENGLISH_CHARSET;

const LSString::DiacriticMap& LSString::DEFAULT_DIACRITIC_MAP = Charset::DEFAULT_DIACRITIC_MAP;

LSString::LSString(const LSString::CharacterSet& charset, const LSString::DiacriticMap& diacritic_map)
	: m_charset(charset),
	  m_diacritic_map(diacritic_map),
	  m_eos_marker(DEFAULT_EOS_MARKER)
{
}

LSString::LSString(const StringType& s, const LSString::CharacterSet& charset, const LSString::DiacriticMap& diacritic_map)
	: m_str(),
	  m_charset(charset),
	  m_diacritic_map(diacritic_map),
	  m_eos_marker(DEFAULT_EOS_MARKER)
{
	Deserialise(s);
}

bool LSString::operator==(const LSString& rhs) const
{
	return this->m_str == rhs.m_str;
}

bool LSString::operator!=(const LSString& rhs) const
{
	return !(*this == rhs);
}

size_t LSString::Decode(const uint8_t* buffer, size_t size)
{
	return DecodeString(buffer, size);
}

size_t LSString::Encode(uint8_t* buffer, size_t size) const
{
	return EncodeString(buffer, size);
}

LSString::StringType LSString::Serialise() const
{
	std::basic_ostringstream<LSString::StringType::value_type> ss;
	ss << ApplyDiacritics(m_str);
	return ss.str();
}

void LSString::Deserialise(const LSString::StringType& in)
{
	m_str = RemoveDiacritics(in);
}

LSString::StringType LSString::GetHeaderRow() const
{
	return LSString::StringType();
}

std::string LSString::GetEncodedFileExt() const
{
	return ".bin";
}

void LSString::AddFrequencyCounts(FrequencyCounts& frequencies) const
{
	std::vector<uint8_t> buffer(1024);
	buffer.resize(EncodeString(buffer.data(), buffer.size()));
	uint8_t last = m_eos_marker;
	for (auto c : buffer)
	{
		frequencies[last][c]++;
		last = c;
	}
}

size_t LSString::DecodeString(const uint8_t* string, size_t len)
{
	m_str.clear();
	size_t strsize = string[0];
	const uint8_t* c = string + 1;
	if (strsize > len)
	{
		throw std::runtime_error("Not enough bytes in buffer to decode string");
	}
	while (strsize > 0)
	{
		m_str += DecodeChar(*c++);
		strsize--;
	}
	return c - string;
}

size_t LSString::EncodeString(uint8_t* string, size_t len) const
{
	size_t i = 1;
	size_t j = 0;
	while (j < m_str.size() && i < len)
	{
		j += EncodeChar(m_str, j, string[i++]);
	}
	if (i > 0x100)
	{
		throw std::runtime_error("String is too long.");
	}
	string[0] = (i - 1) & 0xFF;
	return i;
}

LSString::StringType LSString::DecodeChar(uint8_t chr) const
{
	if (m_charset.find(chr) != m_charset.end())
	{
		return m_charset.at(chr);
	}
	else
	{
		std::basic_ostringstream<LSString::StringType::value_type> ss;
		ss << L"{" << std::setw(2) << std::setfill(L'0') << std::hex << std::uppercase << std::setw(2) << std::setfill(L'0')
		   << static_cast<unsigned>(chr) << L"}";
		return ss.str();
	}
}

size_t LSString::EncodeChar(LSString::StringType str, size_t index, uint8_t& chr) const
{
	for (const auto& c : m_charset)
	{
		if (c.second.size() <= str.size() - index)
		{
			if (c.second == str.substr(index, c.second.size()))
			{
				chr = c.first;
				return c.second.size();
			}
		}
	}
	if (str[index] == '{')
	{
		size_t end_of_number = str.find_first_of('}', index + 1);
		if (end_of_number == std::string::npos)
		{
			std::ostringstream ss;
			ss << "Bad character number in position " << index << " found when parsing string.";
			throw std::runtime_error(ss.str());
		}
		LSString::StringType charnum = str.substr(index + 1, end_of_number - index - 1);
		std::basic_istringstream<LSString::StringType::value_type> iss(charnum);
		size_t num;
		iss >> std::hex >> num;
		if (num > 0xFF)
		{
			std::ostringstream ss;
			ss << "Bad character number in position " << index << " found when parsing string.";
			throw std::runtime_error(ss.str());
		}
		chr = static_cast<uint8_t>(num);
		return 2 + charnum.length();
	}
	std::ostringstream ss;
	ss << "Bad character code \'" << static_cast<int>(str[index]) << "\' in position " << index << " found when parsing string.";
	throw std::runtime_error(ss.str());
}

LSString::StringType LSString::ApplyDiacritics(const StringType& str) const
{
	StringType ret;
	std::size_t index = 0;
	
	while (index < str.size())
	{
		bool found = false;
		for (const auto& d : m_diacritic_map)
		{
			if (d.first.size() <= str.size() - index)
			{
				if (d.first == str.substr(index, d.first.size()))
				{
					for (const auto& c : d.second)
					{
						if (c.first.size() + d.first.size() <= str.size() - index)
						{
							if (c.first == str.substr(index + d.first.size(), c.first.size()))
							{
								ret += c.second;
								index += c.first.size() + d.first.size();
								found = true;
								break;
							}
						}
					}
				}
				if (found)
				{
					break;
				}
			}
		}
		if (!found)
		{
			ret += str[index++];
		}
	}
	return ret;
}

LSString::StringType LSString::RemoveDiacritics(const StringType& str) const
{
	StringType ret;
	std::size_t index = 0;

	while (index < str.size())
	{
		bool found = false;
		for (const auto& d : m_diacritic_map)
		{
			for (const auto& c : d.second)
			{
				if (c.second.size() <= str.size() - index)
				{
					if (c.second == str.substr(index, c.second.size()))
					{
						ret += d.first + c.first;
						index += c.second.size();
						found = true;
						break;
					}
				}
			}
			if (found)
			{
				break;
			}
		}
		if (!found)
		{
			ret += str[index++];
		}
	}
	return ret;
}

const LSString::CharacterSet& LSString::GetCharset() const
{
	return m_charset;
}

void LSString::SetCharset(const CharacterSet& charset)
{
	m_charset = charset;
}
