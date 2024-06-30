#ifndef _LITERALS_H_
#define _LITERALS_H_

#include <cstdint>

inline uint8_t operator ""_u8(unsigned long long val)
{
	return static_cast<uint8_t>(val);
}

inline uint16_t operator ""_u16(unsigned long long val)
{
	return static_cast<uint16_t>(val);
}

inline uint32_t operator ""_u32(unsigned long long val)
{
	return static_cast<uint32_t>(val);
}

inline uint64_t operator ""_u64(unsigned long long val)
{
	return static_cast<uint64_t>(val);
}

inline int8_t operator ""_s8(unsigned long long val)
{
	return static_cast<int8_t>(val);
}

inline int16_t operator ""_s16(unsigned long long val)
{
	return static_cast<int16_t>(val);
}

inline int32_t operator ""_s32(unsigned long long val)
{
	return static_cast<int32_t>(val);
}

inline int64_t operator ""_s64(unsigned long long val)
{
	return static_cast<int64_t>(val);
}

#endif // _LITERALS_H_
