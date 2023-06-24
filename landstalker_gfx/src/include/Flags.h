#ifndef _FLAGS_H_
#define _FLAGS_H_

#include <cstdint>
#include <array>

enum FlagType
{
	ENTITY_VISIBILITY
};

struct EntityVisibilityFlags
{
	uint16_t room;
	uint8_t entity;
	uint16_t flag;
	bool set;

	EntityVisibilityFlags(const std::array<uint8_t, 4>& data);
	std::array<uint8_t, 4> GetData() const;
	bool operator==(const EntityVisibilityFlags& rhs) const;
	bool operator!=(const EntityVisibilityFlags& rhs) const;
};

#endif // _FLAGS_H_
