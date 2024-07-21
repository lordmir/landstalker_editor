#include <landstalker/text/include/HuffmanString.h>
#include <stdexcept>

HuffmanString::HuffmanString(const uint8_t* data, size_t len, std::shared_ptr<HuffmanTrees> ht,
                             const HuffmanString::CharacterSet& charset, uint8_t eos_marker,
                             const DiacriticMap& diacritic_map)
	: LSString(charset, diacritic_map),
	  m_ht(ht)
{
	m_eos_marker = eos_marker;
	Decode(data, len);
}

HuffmanString::HuffmanString(const HuffmanString::StringType& str, std::shared_ptr<HuffmanTrees> ht,
                             const HuffmanString::CharacterSet& charset, uint8_t eos_marker,
                             const DiacriticMap& diacritic_map)
	: LSString(str, charset, diacritic_map),
	  m_ht(ht)
{
	m_eos_marker = eos_marker;
}

HuffmanString::HuffmanString(std::shared_ptr<HuffmanTrees> ht, const HuffmanString::CharacterSet& charset,
                             uint8_t eos_marker, const DiacriticMap& diacritic_map)
	: LSString(charset, diacritic_map),
	  m_ht(ht)
{
	m_eos_marker = eos_marker;
}

bool HuffmanString::operator==(const HuffmanString& rhs) const
{
	return this->m_str == rhs.m_str;
}

bool HuffmanString::operator!=(const HuffmanString& rhs) const
{
	return !(*this == rhs);
}

size_t HuffmanString::Decode(const uint8_t* buffer, size_t size)
{
	if (size == 0 || size < *buffer)
	{
		throw std::runtime_error("Not enough data in buffer to decode string");
	}
	std::vector<uint8_t> compressed(buffer + 1, buffer + buffer[0]);
	std::vector<uint8_t> decompressed(m_ht->DecompressString(compressed, m_eos_marker));
	DecodeString(decompressed.data(), decompressed.size());
	return buffer[0];
}

size_t HuffmanString::Encode(uint8_t* buffer, size_t size) const
{
	std::vector<uint8_t> encoded(1024);
	size_t encoded_len = EncodeString(encoded.data(), encoded.size());
	encoded.resize(encoded_len);
	std::vector<uint8_t> compressed = m_ht->CompressString(encoded, m_eos_marker);
	if (compressed.size() > size)
	{
		throw std::runtime_error("Not enough space in buffer to encode string");
	}
	buffer[0] = static_cast<uint8_t>(compressed.size() + 1);
	std::copy(compressed.begin(), compressed.end(), buffer + 1);
	return compressed.size() + 1;
}

std::string HuffmanString::GetEncodedFileExt() const
{
	return ".huf";
}

size_t HuffmanString::DecodeString(const uint8_t* string, size_t len)
{
	m_str.clear();
	size_t i = 0;
	while (string[i] != m_eos_marker)
	{
		m_str += DecodeChar(string[i++]);
		if (i >= len)
		{
			throw std::runtime_error("Not enough bytes in buffer to decode string");
		}
	}
	return i + 1;
}

size_t HuffmanString::EncodeString(uint8_t* string, size_t len) const
{
	size_t i = 0;
	size_t j = 0;
	while (j < m_str.size() && i < len)
	{
		j += EncodeChar(m_str, j, string[i++]);
	}
	string[i] = m_eos_marker;
	return i + 1;
}
