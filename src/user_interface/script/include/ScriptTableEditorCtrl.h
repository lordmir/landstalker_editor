#ifndef _SCRIPT_TABLE_EDITOR_CTRL_H_
#define _SCRIPT_TABLE_EDITOR_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/include/GameData.h>
#include <user_interface/script/include/ScriptTableDataViewModel.h>

class ScriptTableEditorCtrl : public wxPanel
{
public:

	ScriptTableEditorCtrl(wxWindow* parent);
	virtual ~ScriptTableEditorCtrl();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index = 0);
	void RefreshData();

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

	wxDataViewCtrl* m_dvc_ctrl;
	ScriptTableDataViewModel* m_model;

	std::shared_ptr<GameData> m_gd;

	wxDECLARE_EVENT_TABLE();
};

#endif // _SCRIPT_TABLE_EDITOR_CTRL_H_
