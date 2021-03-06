#ifndef _HUFFMAN_STRING_H_
#define _HUFFMAN_STRING_H_

#include <LSString.h>

#include <memory>
#include <vector>
#include <cstdint>

#include <HuffmanTrees.h>

class HuffmanString : public LSString
{
public:
	HuffmanString(const uint8_t* data, size_t len, std::shared_ptr<HuffmanTrees> ht);
	HuffmanString(std::string str, std::shared_ptr<HuffmanTrees> ht);
	HuffmanString(std::shared_ptr<HuffmanTrees> ht);

	virtual size_t Decode(const uint8_t* buffer, size_t size);
	virtual size_t Encode(uint8_t* buffer, size_t size) const;
	virtual std::string Serialise() const;
	virtual void Deserialise(const std::string& ss);
	virtual std::string GetEncodedFileExt() const;

protected:
	virtual size_t DecodeString(const uint8_t* string, size_t len);
	virtual size_t EncodeString(uint8_t* string, size_t len) const;
private:
	std::shared_ptr<HuffmanTrees> m_ht;
};

#endif // _HUFFMAN_STRING_H_