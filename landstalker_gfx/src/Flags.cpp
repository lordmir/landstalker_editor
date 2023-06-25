#include <Flags.h>

EntityVisibilityFlags::EntityVisibilityFlags(const std::array<uint8_t, 4>& data)
{
	room = (data[0] << 8) | data[1];
	entity = data[3] & 0x1F;
	flag = (data[2] & 0x7F) << 3 | (data[3] >> 5);
	set = (data[2] & 0x80) > 0;
}

std::array<uint8_t, 4> EntityVisibilityFlags::GetData() const
{
	std::array<uint8_t, 4> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = ((flag >> 3) & 0x7F) | (set ? 0x80 : 0x00);
	data[3] = entity & 0x1F | ((flag & 0x07) << 5);
	return data;
}

bool EntityVisibilityFlags::operator==(const EntityVisibilityFlags& rhs) const
{
	return (
		this->room == rhs.room &&
		this->entity == rhs.entity &&
		this->flag == rhs.flag &&
		this->set == rhs.set);
}

bool EntityVisibilityFlags::operator!=(const EntityVisibilityFlags& rhs) const
{
	return !(*this == rhs);
}
