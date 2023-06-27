#include <Flags.h>

EntityFlag::EntityFlag(const std::array<uint8_t, 4>& data)
{
	room = (data[0] << 8) | data[1];
	entity = data[3] & 0x1F;
	flag = (data[2] & 0x7F) << 3 | (data[3] >> 5);
	set = (data[2] & 0x80) > 0;
}

std::array<uint8_t, 4> EntityFlag::GetData() const
{
	std::array<uint8_t, 4> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = ((flag >> 3) & 0x7F) | (set ? 0x80 : 0x00);
	data[3] = entity & 0x1F | ((flag & 0x07) << 5);
	return data;
}

bool EntityFlag::operator==(const EntityFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->entity == rhs.entity &&
		this->flag == rhs.flag &&
		this->set == rhs.set);
}

bool EntityFlag::operator!=(const EntityFlag& rhs) const
{
	return !(*this == rhs);
}

OneTimeEventFlag::OneTimeEventFlag(const std::array<uint8_t, 6>& data)
{
	room = (data[0] << 8) | data[1];
	entity = data[3] & 0x1F;
	flag_on = (data[2] & 0x7F) << 3 | (data[3] >> 5);
	flag_on_set = (data[2] & 0x80) > 0;
	flag_off = (data[4] & 0x7F) << 3 | (data[5] & 0x7);
	flag_off_set = (data[4] & 0x80) > 0;
}

std::array<uint8_t, 6> OneTimeEventFlag::GetData() const
{
	std::array<uint8_t, 6> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = ((flag_on >> 3) & 0x7F) | (flag_on_set ? 0x80 : 0x00);
	data[3] = entity & 0x1F | ((flag_on & 0x07) << 5);
	data[4] = ((flag_off >> 3) & 0x7F) | (flag_off_set ? 0x80 : 0x00);
	data[5] = flag_off & 0x07;
	return data;
}

bool OneTimeEventFlag::operator==(const OneTimeEventFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->entity == rhs.entity &&
		this->flag_on == rhs.flag_on &&
		this->flag_on_set == rhs.flag_on_set &&
		this->flag_off == rhs.flag_off &&
		this->flag_off_set == rhs.flag_off_set);
}

bool OneTimeEventFlag::operator!=(const OneTimeEventFlag& rhs) const
{
	return !(*this == rhs);
}

SacredTreeFlag::SacredTreeFlag(const std::array<uint8_t, SIZE>& data)
{
	room = (data[0] << 8) | data[1];
	flag = data[2] << 3 | (data[3] & 0x07);
}

std::array<uint8_t, 4> SacredTreeFlag::GetData() const
{
	std::array<uint8_t, 4> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = (flag >> 3) & 0xFF;
	data[3] = flag & 0x07;
	return data;
}

bool SacredTreeFlag::operator==(const SacredTreeFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->flag == rhs.flag);
}

bool SacredTreeFlag::operator!=(const SacredTreeFlag& rhs) const
{
	return !(*this == rhs);
}
