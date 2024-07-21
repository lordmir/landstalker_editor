#ifndef _HUFFMAN_STRING_H_
#define _HUFFMAN_STRING_H_

#include <memory>
#include <vector>
#include <cstdint>

#include <landstalker/text/include/LSString.h>
#include <landstalker/text/include/HuffmanTrees.h>

class HuffmanString : public LSString
{
public:
	HuffmanString(const uint8_t* data, size_t len, std::shared_ptr<HuffmanTrees> ht,
	              const CharacterSet& charset = DEFAULT_CHARACTER_SET, uint8_t eos_marker = DEFAULT_EOS_MARKER,
	              const DiacriticMap& diacritic_map = DEFAULT_DIACRITIC_MAP);
	HuffmanString(const StringType& str, std::shared_ptr<HuffmanTrees> ht,
	              const CharacterSet& charset = DEFAULT_CHARACTER_SET, uint8_t eos_marker = DEFAULT_EOS_MARKER,
	              const DiacriticMap& diacritic_map = DEFAULT_DIACRITIC_MAP);
	HuffmanString(std::shared_ptr<HuffmanTrees> ht, const CharacterSet& charset = DEFAULT_CHARACTER_SET,
	              uint8_t eos_marker = DEFAULT_EOS_MARKER,
	              const DiacriticMap& diacritic_map = DEFAULT_DIACRITIC_MAP);

	bool operator==(const HuffmanString& rhs) const;
	bool operator!=(const HuffmanString& rhs) const;

	virtual size_t Decode(const uint8_t* buffer, size_t size);
	virtual size_t Encode(uint8_t* buffer, size_t size) const;
	virtual std::string GetEncodedFileExt() const;

protected:
	virtual size_t DecodeString(const uint8_t* string, size_t len);
	virtual size_t EncodeString(uint8_t* string, size_t len) const;
private:
	std::shared_ptr<HuffmanTrees> m_ht;
};

#endif // _HUFFMAN_STRING_H_