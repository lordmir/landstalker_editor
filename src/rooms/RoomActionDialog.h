#ifndef _ROOM_ACTION_DIALOG_H_
#define _ROOM_ACTION_DIALOG_H_

#include <memory>
#include <string>
#include <vector>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>

class wxListBox;
class wxButton;
class CutsceneCodeEditor;

// Focused dialog for the hand-coded actions of one room (customroomactions1/2.asm), launched from
// the room editor's info overlay. Lists the room's branches, edits the selected one's raw m68k in a
// CutsceneCodeEditor, and Add (this room or a BGM) / Remove manage them. Edits commit into the shared
// RoomActionTable on OK. See [[cutscene-two-layer-architecture]].
class RoomActionDialog : public wxDialog
{
public:
	RoomActionDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int room);

private:
	void Rebuild(const std::string& select_label = std::string());
	void ShowSelected();
	void CommitCurrentEdit();
	void OnAdd();
	void OnRemove();
	void OnOk(wxCommandEvent& evt);
	void UpdateButtons();

	std::shared_ptr<Landstalker::GameData> m_gd;
	int m_room;
	wxListBox* m_list = nullptr;
	CutsceneCodeEditor* m_code = nullptr;
	wxButton* m_remove = nullptr;
	std::vector<std::string> m_labels; // branch label per list row
	std::string m_current_label;
};

#endif // _ROOM_ACTION_DIALOG_H_
