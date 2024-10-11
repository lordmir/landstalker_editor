#ifndef _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_
#define _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_

#include <memory>

#include <wx/wx.h>

#include <landstalker/main/include/GameData.h>

class BehaviourScriptEditorCtrl : public wxPanel
{
public:
	BehaviourScriptEditorCtrl(wxWindow* parent);
	virtual ~BehaviourScriptEditorCtrl();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void Open(int id = 0);
	int GetOpenScriptId() const;

private:
	void RefreshBehaviourScript();
	void UpdateUI();

	bool Commit();

	void OnScriptSelect(wxCommandEvent& evt);
	void OnSaveClick(wxCommandEvent& evt);
	void OnResetClick(wxCommandEvent& evt);
	void OnScriptChange(wxCommandEvent& evt);
	void OnClose(wxCloseEvent& evt);

	wxTextCtrl* m_text_ctrl;
	wxChoice* m_script_dropdown;
	wxButton* m_save_btn;
	wxButton* m_reset_btn;

	std::shared_ptr<GameData> m_gd;
	int m_behaviour_script;
	std::string orig;
	bool dirty;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_
