#ifndef _ROOM_ACTION_DISPLAY_H_
#define _ROOM_ACTION_DISPLAY_H_

#include <string>

#include <wx/string.h>

#include <landstalker/main/GameData.h>
#include <landstalker/main/RoomData.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/misc/Utils.h>
#include <landstalker/script/RoomActionTable.h>

// Small display helpers shared by the Room Actions frame and dialog.
namespace RoomActionDisplay {

// The action's name: its custom C_ROOM_ACTION label (keyed by chain position), else a generic
// "RoomActionNN" seed. Replaces listing actions by their comment text.
inline wxString ActionName(int index)
{
	const auto label = Landstalker::Labels::Get(Landstalker::Labels::C_ROOM_ACTION, index);
	if (label && !label->empty()) return wxString(*label);
	return wxString::Format("RoomAction%02d", index);
}

inline wxString KeyType(const Landstalker::RoomActionTable::Branch& b)
{
	switch (b.key)
	{
	case Landstalker::RoomActionTable::KeyType::ROOM: return "Room";
	case Landstalker::RoomActionTable::KeyType::BGM:  return "BGM";
	default:                                          return "Helper";
	}
}

inline wxString KeyName(const Landstalker::RoomActionTable::Branch& b,
	const std::shared_ptr<Landstalker::GameData>& gd)
{
	using KT = Landstalker::RoomActionTable::KeyType;
	if (b.key == KT::ROOM && gd && gd->GetRoomData()
		&& b.value >= 0 && b.value < static_cast<int>(gd->GetRoomData()->GetRoomCount()))
	{
		return wxString::FromUTF8(Landstalker::wstr_to_utf8(
			gd->GetRoomData()->GetRoomDisplayName(static_cast<uint16_t>(b.value))));
	}
	if (b.key == KT::BGM)
	{
		const auto label = Landstalker::Labels::Get(Landstalker::Labels::C_BGMS, b.value);
		return label ? wxString(*label) : wxString::Format("BGM $%02X", static_cast<unsigned>(b.value));
	}
	return wxString::FromUTF8(b.label); // OTHER / unresolved: show the entry label
}

// A one-line summary of a branch body: its first comment, else its first code line after the guard.
inline wxString Summary(const std::string& text)
{
	std::string first_comment;
	std::string first_code;
	int code_seen = 0;
	std::size_t start = 0;
	while (start < text.size())
	{
		std::size_t end = text.find('\n', start);
		if (end == std::string::npos) end = text.size();
		std::string line = text.substr(start, end - start);
		start = end + 1;
		while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
		std::size_t i = 0;
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
		line = line.substr(i);
		if (line.empty()) continue;
		if (line[0] == ';')
		{
			std::string body = line.substr(1);
			while (!body.empty() && (body.front() == ' ' || body.front() == '-')) body.erase(body.begin());
			if (!body.empty() && first_comment.empty()) first_comment = body;
		}
		else if (line.back() != ':')
		{
			// Skip the label line and the guard (cmpi + branch); show the first "real" body line.
			if (++code_seen > 2 && first_code.empty()) first_code = line;
		}
	}
	if (!first_comment.empty()) return wxString::FromUTF8(first_comment);
	return wxString::FromUTF8(first_code);
}

} // namespace RoomActionDisplay

#endif // _ROOM_ACTION_DISPLAY_H_
