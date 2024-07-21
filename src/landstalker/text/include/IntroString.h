#ifndef _INTRO_STRING_H_
#define _INTRO_STRING_H_

#include <landstalker/text/include/LSString.h>

class IntroString : public LSString
{
public:
	IntroString();
	IntroString(uint16_t line1_y, uint16_t line1_x, uint16_t line2_y, uint16_t line2_x, uint16_t display_time, StringType line1, StringType line2);
	IntroString(const StringType& serialised);

	bool operator==(const IntroString& rhs) const;
	bool operator!=(const IntroString& rhs) const;

	virtual size_t Decode(const uint8_t* buffer, size_t size);
	virtual size_t Encode(uint8_t* buffer, size_t size) const;
	virtual StringType Serialise() const;
	virtual void Deserialise(const StringType& ss);
	virtual StringType GetHeaderRow() const;

	uint16_t GetLine1Y() const;
	void SetLine1Y(uint16_t val);
	uint16_t GetLine1X() const;
	void SetLine1X(uint16_t val);
	uint16_t GetLine2Y() const;
	void SetLine2Y(uint16_t val);
	uint16_t GetLine2X() const;
	void SetLine2X(uint16_t val);
	uint16_t GetDisplayTime() const;
	void SetDisplayTime(uint16_t val);
	StringType GetLine(int line) const;
	void SetLine(int line, const StringType& str);
protected:
	virtual size_t DecodeString(const uint8_t* string, size_t len);
	virtual size_t EncodeString(uint8_t* string, size_t len) const;
private:
	uint16_t m_line1Y;
	uint16_t m_line1X;
	uint16_t m_line2Y;
	uint16_t m_line2X;
	uint16_t m_displayTime;
	StringType m_line2;
};

#endif // _INTRO_STRING_H