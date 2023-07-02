#ifndef _CHESTS_H_
#define _CHESTS_H_

#include <cstdint>
#include <vector>
#include <set>
#include <map>

typedef uint8_t ChestItem;

class GameData;

class Chests
{
public:
	Chests(const std::vector<uint8_t>& offsets, const std::vector<uint8_t>& contents);
	Chests();

	bool operator==(const Chests& rhs) const;
	bool operator!=(const Chests& rhs) const;

	std::pair<std::vector<uint8_t>, std::vector<uint8_t>> GetData(int roomcount) const;

	std::vector<ChestItem> GetChestsForRoom(uint16_t room) const;
	bool RoomHasChests(uint16_t room) const;
	bool RoomHasNoChestsSet(uint16_t room) const;
	void SetRoomChests(uint16_t room, const std::vector<ChestItem>& chests);
	void ClearRoomChests(uint16_t room);
	void SetRoomNoChestsFlag(uint16_t room);
	void ClearRoomNoChestsFlag(uint16_t room);
	bool CleanupRoomChests(const GameData& gd);

private:
	std::set<uint16_t> m_enabled;
	std::map<uint16_t, std::vector<ChestItem>> m_chests;

};

#endif // _CHESTS_H_
