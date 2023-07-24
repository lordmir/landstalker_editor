#ifndef _TILE_SWAPS_H_
#define _TILE_SWAPS_H_

#include <vector>
#include <cstdint>
#include <map>

struct TileSwap
{
	enum class Mode : uint8_t
	{
		FLOOR = 0,
		WALL_NE = 1,
		WALL_NW = 2
	};

	struct CopyOp
	{
		uint8_t src_x;
		uint8_t src_y;
		uint8_t dst_x;
		uint8_t dst_y;
		uint8_t width;
		uint8_t height;
	};

	TileSwap(const std::vector<uint8_t>& in);
	TileSwap() : map({ 0,0,0,0,1,1 }), heightmap({ 0,0,0,0,1,1 }), mode(Mode::FLOOR), active(false) {}
	std::vector<uint8_t> GetBytes(uint16_t room, uint8_t idx) const;

	bool operator==(const TileSwap& rhs) const;
	bool operator!=(const TileSwap& rhs) const;

	CopyOp map;
	CopyOp heightmap;
	Mode mode;
	bool active;
};

class TileSwaps
{
public:
	TileSwaps(const std::vector<uint8_t>& bytes);
	TileSwaps();

	bool operator==(const TileSwaps& rhs) const;
	bool operator!=(const TileSwaps& rhs) const;

	std::vector<uint8_t> GetData() const;
	std::vector<TileSwap> GetSwapsForRoom(uint16_t room) const;
	bool RoomHasSwaps(uint16_t room) const;
	void SetRoomSwaps(uint16_t room, const std::vector<TileSwap>& swaps);
private:
	std::map<uint16_t, std::vector<TileSwap>> m_swaps;
};

#endif // _TILE_SWAPS_H_
