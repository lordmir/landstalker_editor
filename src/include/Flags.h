#ifndef _FLAGS_H_
#define _FLAGS_H_

#include <cstdint>
#include <array>

enum class FlagType
{
	ENTITY_VISIBILITY,
	ONE_TIME_ENTITY_VISIBILITY,
	HIDE_MULTIPLE_ENTITIES,
	LOCKED_DOOR,
	PERMANENT_SWITCH,
	SACRED_TREE,
	ROOM_TRANSITION,
	LOCKED_DOOR_TILESWAP,
	TILESWAP,
	TREE_WARP
};

struct FlagTrigger
{
};

struct EntityFlag : public FlagTrigger
{
	static const int SIZE = 4;

	uint16_t room;
	uint16_t flag;
	uint8_t entity;
	bool set;

	EntityFlag(const std::array<uint8_t, SIZE>& data);
	EntityFlag(uint16_t rm) : room(rm), flag(0), entity(0), set(false) {}
	std::array<uint8_t, SIZE> GetData() const;
	bool operator==(const EntityFlag& rhs) const;
	bool operator!=(const EntityFlag& rhs) const;
};

struct OneTimeEventFlag : public FlagTrigger
{
	static const int SIZE = 6;

	uint16_t room;
	uint8_t entity;
	uint16_t flag_on;
	bool flag_on_set;
	uint16_t flag_off;
	bool flag_off_set;

	OneTimeEventFlag(const std::array<uint8_t, SIZE>& data);
	OneTimeEventFlag(uint16_t rm) : room(rm), entity(0), flag_on(0), flag_on_set(false), flag_off(0), flag_off_set(false) {}
	std::array<uint8_t, SIZE> GetData() const;
	bool operator==(const OneTimeEventFlag& rhs) const;
	bool operator!=(const OneTimeEventFlag& rhs) const;
};

struct RoomClearFlag : public FlagTrigger
{
	static const int SIZE = 4;

	uint16_t room;
	uint16_t flag;
	uint8_t entity;

	RoomClearFlag(const std::array<uint8_t, SIZE>& data);
	RoomClearFlag(uint16_t rm) : room(rm), flag(0), entity(0) {}
	std::array<uint8_t, SIZE> GetData() const;
	bool operator==(const RoomClearFlag& rhs) const;
	bool operator!=(const RoomClearFlag& rhs) const;
};

struct SacredTreeFlag : public FlagTrigger
{
	static const int SIZE = 4;

	uint16_t room;
	uint16_t flag;

	SacredTreeFlag(const std::array<uint8_t, SIZE>& data);
	SacredTreeFlag(uint16_t rm) : room(rm), flag(0) {}
	std::array<uint8_t, SIZE> GetData() const;
	bool operator==(const SacredTreeFlag& rhs) const;
	bool operator!=(const SacredTreeFlag& rhs) const;
};

struct TileSwapFlag : public FlagTrigger
{
	static const int SIZE = 4;

	uint16_t room;
	uint16_t flag;
	uint8_t index;
	bool always;

	TileSwapFlag(const std::array<uint8_t, SIZE>& data);
	TileSwapFlag(uint16_t rm, uint8_t idx) : room(rm), flag(0), index(idx), always(false) {}
	std::array<uint8_t, SIZE> GetData() const;
	bool operator==(const TileSwapFlag& rhs) const;
	bool operator!=(const TileSwapFlag& rhs) const;
};

struct TreeWarpFlag : public FlagTrigger
{
	static const int SIZE = 8;

	uint16_t room1;
	uint16_t room2;
	uint16_t flag;

	TreeWarpFlag(const std::array<uint8_t, SIZE>& data);
	TreeWarpFlag(uint16_t rm1, uint16_t rm2, uint16_t flag) : room1(rm1), room2(rm2), flag(flag) {}
	TreeWarpFlag(uint16_t rm) : room1(rm), room2(0), flag(0) {}
	std::array<uint8_t, SIZE> GetData() const;
	bool operator==(const TreeWarpFlag& rhs) const;
	bool operator!=(const TreeWarpFlag& rhs) const;
};

#endif // _FLAGS_H_
