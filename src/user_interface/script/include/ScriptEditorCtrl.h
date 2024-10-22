#ifndef _SCRIPT_EDITOR_CTRL_H_
#define _SCRIPT_EDITOR_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/include/GameData.h>
#include <user_interface/script/include/ScriptDataViewModel.h>

class ScriptEditorCtrl : public wxPanel
{
public:
	ScriptEditorCtrl(wxWindow* parent);
	virtual ~ScriptEditorCtrl();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void Open();

private:
	void UpdateUI();

	wxDataViewCtrl* m_dvc_ctrl;
	ScriptDataViewModel* m_model;

	std::shared_ptr<GameData> m_gd;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_
