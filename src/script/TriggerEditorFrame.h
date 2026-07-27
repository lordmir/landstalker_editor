#ifndef _TRIGGER_EDITOR_FRAME_H_
#define _TRIGGER_EDITOR_FRAME_H_

#include <cstddef>
#include <string>

#include <wx/aui/aui.h>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxListCtrl;
class wxListEvent;
class wxButton;
class CutsceneCodeEditor;

// Editor for the behaviour trigger action code ("Trigger Actions"): the TA_xx handlers in
// triggeractions.asm, dispatched by index through triggeractionjumptable.asm (a behaviour
// WaitForCondition's Condition operand). Each row is one trigger id (a stable dispatch slot);
// selecting it shows that handler's raw m68k, editable in a CutsceneCodeEditor (same idioms as the
// cutscene handlers). "Move" swaps two slots' handlers, keeping every external trigger index valid
// (see [[reorder-semantics]] / [[cutscene-two-layer-architecture]]).
class TriggerEditorFrame : public EditorFrame
{
public:
	TriggerEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~TriggerEditorFrame();

	bool Open(int slot = -1);
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	// Flush the open text edit into the game data before a save/build.
	virtual void CommitPendingEdits();

private:
	void BuildUI();
	void RefreshList();
	void RefreshRow(long slot);
	void ShowSlot(long slot);
	void CommitCurrentEdit();
	void UpdateButtons();
	void MoveSelection(int direction);
	void AddTrigger();
	void RemoveTrigger();
	void RenameTrigger();
	long SelectedSlot() const;

	void OnSlotSelected(wxListEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnRemove(wxCommandEvent& evt);
	void OnRename(wxCommandEvent& evt);

	wxAuiManager m_mgr;
	wxListCtrl* m_list = nullptr;
	CutsceneCodeEditor* m_code = nullptr;
	wxButton* m_add = nullptr;
	wxButton* m_remove = nullptr;
	wxButton* m_rename = nullptr;
	wxButton* m_move_up = nullptr;
	wxButton* m_move_down = nullptr;

	// The slot whose block is currently shown in the text control, or -1 when none.
	long m_current_slot = -1;
	std::string m_current_label;
};

#endif // _TRIGGER_EDITOR_FRAME_H_
