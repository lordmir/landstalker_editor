#ifndef _ROOM_ACTION_ADD_DIALOG_H_
#define _ROOM_ACTION_ADD_DIALOG_H_

#include <memory>
#include <vector>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>

class wxChoice;
class LookupChoiceControl;

// Small picker for adding a room action: choose whether it keys off a room number or a BGM id, and
// pick the value from a searchable name dropdown (a LookupChoiceControl per key type, swapped by the
// Key choice). Used by both the standalone RoomActionDialog and the global RoomActionsEditorFrame.
class RoomActionAddDialog : public wxDialog
{
public:
	// default_room >= 0 preselects Room with that value (the room being edited).
	RoomActionAddDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int default_room = -1);

	bool IsBgm() const;
	int GetValue() const;

private:
	void UpdateVisibility();

	std::shared_ptr<Landstalker::GameData> m_gd;
	wxChoice* m_type = nullptr;
	LookupChoiceControl* m_room_pick = nullptr; // room list index == room number
	LookupChoiceControl* m_bgm_pick = nullptr;
	std::vector<int> m_bgm_ids; // bgm_pick selection index -> real BGM id (the category is sparse)
};

#endif // _ROOM_ACTION_ADD_DIALOG_H_
