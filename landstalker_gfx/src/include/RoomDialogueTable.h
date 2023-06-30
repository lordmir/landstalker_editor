#ifndef _ROOM_DIALOGUE_TABLE_H_
#define _ROOM_DIALOGUE_TABLE_H_

#include <cstdint>
#include <vector>
#include <map>

typedef uint16_t Character;

class RoomDialogueTable
{
public:
	RoomDialogueTable(const std::vector<uint16_t>& data);
	RoomDialogueTable();

	bool operator==(const RoomDialogueTable& rhs) const;
	bool operator!=(const RoomDialogueTable& rhs) const;

	std::vector<uint16_t> GetData() const;

	std::vector<Character> GetRoomCharacters(uint16_t room) const;
	void SetRoomCharacters(uint16_t room, const std::vector<Character>& chars);
private:
	std::map<uint16_t, std::vector<Character>> m_dialogue_table;
};

#endif // _ROOM_DIALOGUE_TABLE_H_
