#ifndef _CUTSCENE_EDITOR_FRAME_H_
#define _CUTSCENE_EDITOR_FRAME_H_

#include <cstddef>
#include <string>

#include <wx/aui/aui.h>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxListCtrl;
class wxListEvent;
class wxButton;
class CutsceneCodeEditor;

// Editor for the cutscene action code ("Cutscenes"): the CSA_xxxx handlers in
// dialogueactions.asm, dispatched by index through dialogueactionjumptable.asm. Each row is one
// cutscene id (a stable dispatch slot); selecting it shows that handler's raw m68k, editable as
// text in a CutsceneCodeEditor. "Move" swaps two slots' handlers, keeping every external cutscene
// index valid (see [[reorder-semantics]] / [[cutscene-two-layer-architecture]]). Distinct from the
// script-VM "Cutscene Scripts" editor.
class CutsceneEditorFrame : public EditorFrame
{
public:
	CutsceneEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~CutsceneEditorFrame();

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
	void AddCutscene();
	void RemoveCutscene();
	void RenameCutscene();
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

	// The slot whose block is currently shown in the text control, or -1 when none. Used to write
	// edits back to the right block on selection change / commit.
	long m_current_slot = -1;
	std::string m_current_label;
};

#endif // _CUTSCENE_EDITOR_FRAME_H_
