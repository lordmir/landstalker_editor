#ifndef _TILE_SWAPS_H_
#define _TILE_SWAPS_H_

#include <vector>
#include <cstdint>
#include <map>
#include <memory>
#include <landstalker/3d_maps/include/Tilemap3DCmp.h>

struct TileSwap
{
	enum class Mode : uint8_t
	{
		FLOOR = 0,
		WALL_NE = 1,
		WALL_NW = 2
	};

	enum class Region : uint8_t
	{
		UNDEFINED = 0,
		SOURCE = 1,
		DESTINATION = 2
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
	TileSwap() : trigger(0), map({ 0,0,0,0,1,1 }), heightmap({ 0,0,0,0,1,1 }), mode(Mode::FLOOR), active(false) {}
	TileSwap(uint8_t p_trigger, CopyOp p_map, CopyOp p_heightmap, Mode p_mode) : trigger(p_trigger), map(p_map), heightmap(p_heightmap), mode(p_mode), active(false) {}
	std::vector<uint8_t> GetBytes(uint16_t room, uint8_t idx) const;

	std::vector<std::pair<int, int>> GetMapRegionPoly(Region region = Region::UNDEFINED, int tile_width = 8, int tile_height = 8) const;
	static std::vector<std::pair<int, int>> OffsetRegionPoly(const std::vector<std::pair<int, int>>& points, const std::pair<int, int>& offset, int tile_width = 1, int tile_height = 1);
	std::pair<int, int> GetTileOffset(TileSwap::Region region = TileSwap::Region::UNDEFINED, std::shared_ptr<const Tilemap3D> tilemap = nullptr, const Tilemap3D::Layer& layer = Tilemap3D::Layer::BG) const;
	std::pair<int, int> GetRelTileOffset(const Tilemap3D::Layer& layer = Tilemap3D::Layer::BG) const;
	void DrawSwap(Tilemap3D& map, Tilemap3D::Layer layer) const;
	void DrawHeightmapSwap(Tilemap3D& map) const;
	bool IsHeightmapPointInSwap(int x, int y) const;

	bool operator==(const TileSwap& rhs) const;
	bool operator!=(const TileSwap& rhs) const;

	uint8_t trigger;
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
