#ifndef _WARP_LIST_H_
#define _WARP_LIST_H_

#include "wjakob/filesystem/path.h"
#include <Rom.h>
#include <vector>
#include <cstdint>


class WarpList
{
public:
	struct Warp
	{
		enum class Type : uint8_t
		{
			NORMAL,
			STAIR_SE,
			STAIR_SW,
			UNKNOWN
		};

		uint16_t room1;
		uint8_t x1;
		uint8_t y1;
		uint16_t room2;
		uint8_t x2;
		uint8_t y2;
		uint8_t x_size;
		uint8_t y_size;
		Type type;

		Warp(const std::vector<uint8_t>& raw);
		std::vector<uint8_t> GetRaw();
	};
	WarpList(const filesystem::path& path);
	WarpList(const Rom& rom);
	WarpList() = default;

	std::list<Warp> GetWarpsForRoom(uint16_t room) const;

private:
	std::vector<Warp> m_warps;
};

#endif // _WARP_LIST_H_
