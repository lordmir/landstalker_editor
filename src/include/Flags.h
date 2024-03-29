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
	ROOM_TRANSITION
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
	EntityFlag(uint16_t rm) : room(rm), entity(0), flag(0), set(false) {}
	std::array<uint8_t, 4> GetData() const;
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
	std::array<uint8_t, 6> GetData() const;
	bool operator==(const OneTimeEventFlag& rhs) const;
	bool operator!=(const OneTimeEventFlag& rhs) const;
};

struct SacredTreeFlag : public FlagTrigger
{
	static const int SIZE = 4;

	uint16_t room;
	uint16_t flag;

	SacredTreeFlag(const std::array<uint8_t, SIZE>& data);
	SacredTreeFlag(uint16_t rm) : room(rm), flag(0) {}
	std::array<uint8_t, 4> GetData() const;
	bool operator==(const SacredTreeFlag& rhs) const;
	bool operator!=(const SacredTreeFlag& rhs) const;
};

#endif // _FLAGS_H_
