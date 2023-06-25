#ifndef _FLAGS_H_
#define _FLAGS_H_

#include <cstdint>
#include <array>

enum class FlagType
{
	ENTITY_VISIBILITY
};

struct FlagTrigger
{
};

struct EntityVisibilityFlags : public FlagTrigger
{
	uint16_t room;
	uint16_t flag;
	uint8_t entity;
	bool set;

	EntityVisibilityFlags(const std::array<uint8_t, 4>& data);
	EntityVisibilityFlags(uint16_t rm) : room(rm), entity(0), flag(0), set(false) {}
	std::array<uint8_t, 4> GetData() const;
	bool operator==(const EntityVisibilityFlags& rhs) const;
	bool operator!=(const EntityVisibilityFlags& rhs) const;
};

#endif // _FLAGS_H_
