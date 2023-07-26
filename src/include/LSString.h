#ifndef _LS_STRING_H_
#define _LS_STRING_H_

#include <string>
#include <cstdint>
#include <unordered_map>

class LSString
{
public:
	typedef std::wstring StringType;
	typedef std::unordered_map<uint8_t, std::unordered_map<uint8_t, size_t>> FrequencyCounts;
	typedef std::unordered_map<uint8_t, StringType> CharacterSet;
	typedef std::unordered_map<LSString::StringType, std::unordered_map<LSString::StringType, LSString::StringType>> DiacriticMap;

	LSString(const CharacterSet& charset = DEFAULT_CHARACTER_SET, const DiacriticMap& diacritic_map = DEFAULT_DIACRITIC_MAP);
	LSString(const StringType& s, const CharacterSet& charset = DEFAULT_CHARACTER_SET, const DiacriticMap& diacritic_map = DEFAULT_DIACRITIC_MAP);

	bool operator==(const LSString& rhs) const;
	bool operator!=(const LSString& rhs) const;

	virtual size_t Decode(const uint8_t* buffer, size_t size);
	virtual size_t Encode(uint8_t* buffer, size_t size) const;
	virtual StringType Serialise() const;
	virtual void Deserialise(const StringType& ss);
	virtual StringType GetHeaderRow() const;
	virtual std::string GetEncodedFileExt() const;
	virtual const CharacterSet& GetCharset() const;
	void SetCharset(const CharacterSet& charset);
	void AddFrequencyCounts(FrequencyCounts& frequencies) const;

	StringType Str() const
	{
		return ApplyDiacritics(m_str);
	}

	static const CharacterSet& DEFAULT_CHARACTER_SET;
	static const DiacriticMap& DEFAULT_DIACRITIC_MAP;
	static const uint8_t DEFAULT_EOS_MARKER = 0x55;
protected:
	virtual size_t DecodeString(const uint8_t* string, size_t len);
	virtual size_t EncodeString(uint8_t* string, size_t len) const;
	StringType DecodeChar(uint8_t chr) const;
	size_t EncodeChar(StringType str, size_t index, uint8_t& chr) const;
	StringType m_str;
	CharacterSet m_charset;
	DiacriticMap m_diacritic_map;
	uint8_t m_eos_marker;

private:
	StringType ApplyDiacritics(const StringType& str) const;
	StringType RemoveDiacritics(const StringType& str) const;
};

#endif // _LS_STRING_H_
