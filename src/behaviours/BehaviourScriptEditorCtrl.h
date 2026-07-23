#ifndef _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_
#define _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/GameData.h>
#include <behaviours/BehaviourScriptDataViewModel.h>

class BehaviourScriptEditorCtrl : public wxPanel
{
public:
	BehaviourScriptEditorCtrl(wxWindow* parent);
	virtual ~BehaviourScriptEditorCtrl();

	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	void Open(int id = 0);
	int GetOpenScriptId() const;

	void AppendRow();
	void InsertRow();
	void DeleteRow();
	void MoveRowUp();
	void MoveRowDown();

	bool IsRowSelected() const;
	bool IsSelTop() const;
	bool IsSelBottom() const;

private:
	// Tears down and rebuilds the dataview's model/columns for the currently open script -
	// edits commit straight into SpriteData via the model, so there's no Save/Reset step.
	void RecreateModel();
	// Resizes the Command column to fill whatever width is left after the Index column - wx has
	// no built-in fill mode for dataview columns (see the bind in the constructor).
	void FitCommandColumn();
	void UpdateButtonStates();

	void OnSelectionChange(wxDataViewEvent& evt);
	void OnContextMenu(wxDataViewEvent& evt);

	// Row-index-parameterized operations backing the context menu, which acts on whichever row
	// was right-clicked rather than the current selection (same as ScriptEditorCtrl).
	void AddRowAt(int row, bool below);
	void DeleteRowAt(int row);
	void MoveRowAt(int row, int direction);
	void EditRowAt(int row);

	int RowFromItem(const wxDataViewItem& item) const;
	wxDataViewItem ItemFromRow(int row) const;

	wxDataViewCtrl* m_dvc_ctrl;
	BehaviourScriptDataViewModel* m_model;

	wxButton* m_append_button = nullptr;
	wxButton* m_insert_button = nullptr;
	wxButton* m_delete_button = nullptr;
	wxButton* m_move_up_button = nullptr;
	wxButton* m_move_down_button = nullptr;

	std::shared_ptr<Landstalker::GameData> m_gd;
	int m_behaviour_script;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_
