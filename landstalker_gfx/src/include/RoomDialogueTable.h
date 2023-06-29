#ifndef _ROOM_DIALOGUE_TABLE_H_
#define _ROOM_DIALOGUE_TABLE_H_

#include <cstdint>
#include <vector>
#include <map>

class RoomDialogueTable
{
public:
	RoomDialogueTable(const std::vector<uint16_t>& data);
	RoomDialogueTable();

	bool operator==(const RoomDialogueTable& rhs) const;
	bool operator!=(const RoomDialogueTable& rhs) const;

	std::vector<uint16_t> GetData() const;
private:
	std::map<uint16_t, std::vector<uint16_t>> m_dialogue_table;
};

#endif // _ROOM_DIALOGUE_TABLE_H_
