#ifndef _SCRIPT_TABLE_EDITOR_CTRL_H_
#define _SCRIPT_TABLE_EDITOR_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/include/GameData.h>
#include <user_interface/script/include/ScriptTableDataViewModel.h>
#include <user_interface/script/include/DataViewScriptActionEditorControl.h>

class ScriptTableEditorCtrl : public wxPanel
{
public:

	ScriptTableEditorCtrl(wxWindow* parent);
	virtual ~ScriptTableEditorCtrl();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index = 0, int row = -1);
	void RefreshData();

	void AppendRow();
	void InsertRow();
	void DeleteRow();
	void MoveRowUp();
	void MoveRowDown();

	bool IsRowSelected() const;
	bool IsSelTop() const;
	bool IsSelBottom() const;

private:
	void UpdateUI();

	void OnSelectionChange(wxDataViewEvent& evt);
	void OnMotion(wxMouseEvent& evt);
	void OnRClick(wxMouseEvent& evt);
	void HandleMouseMove(const wxPoint& mouse_pos);
	void FireEvent(const wxEventType& e, const wxString& data = wxEmptyString, long numeric_data = 0, long extra_numeric_data = 0, long extra_extra_numeric_data = 0);

	wxDataViewCtrl* m_dvc_ctrl;
	ScriptTableDataViewModel* m_model;

	std::shared_ptr<GameData> m_gd;

	int m_last_tooltip_item = -1;

	wxDECLARE_EVENT_TABLE();
};

#endif // _SCRIPT_TABLE_EDITOR_CTRL_H_
