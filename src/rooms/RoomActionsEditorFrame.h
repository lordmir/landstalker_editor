#ifndef _ROOM_ACTIONS_EDITOR_FRAME_H_
#define _ROOM_ACTIONS_EDITOR_FRAME_H_

#include <cstddef>
#include <string>

#include <wx/aui/aui.h>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxListCtrl;
class wxListEvent;
class wxButton;
class CutsceneCodeEditor;

// Global editor for the per-room fixup chain ("Room Actions": customroomactions1/2.asm). Lists every
// branch (keyed by room / BGM / a helper block) with the selected one's raw m68k editable in a
// CutsceneCodeEditor. Add appends a room-/BGM-keyed branch; Remove splices a branch out (re-pointing
// the predecessor guard). Reachable under Assembly and mirrored by the per-room RoomActionDialog.
class RoomActionsEditorFrame : public EditorFrame
{
public:
	RoomActionsEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~RoomActionsEditorFrame();

	bool Open(const std::string& select_label = std::string());
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	virtual void CommitPendingEdits();

private:
	void BuildUI();
	void RefreshList();
	void RefreshRow(long row);
	void ShowRow(long row);
	void CommitCurrentEdit();
	void UpdateButtons();
	void OnAdd();
	void OnRemove();
	long SelectedRow() const;

	void OnRowSelected(wxListEvent& evt);

	wxAuiManager m_mgr;
	wxListCtrl* m_list = nullptr;
	CutsceneCodeEditor* m_code = nullptr;
	wxButton* m_add = nullptr;
	wxButton* m_remove = nullptr;

	long m_current_row = -1;
	std::string m_current_label;
};

#endif // _ROOM_ACTIONS_EDITOR_FRAME_H_
