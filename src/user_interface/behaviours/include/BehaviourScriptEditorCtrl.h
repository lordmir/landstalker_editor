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
	void UpdateUI();

	wxTextCtrl* m_text_ctrl;
	wxChoice* m_script_dropdown;
	wxArrayString m_scripts;

	std::shared_ptr<GameData> m_gd;
	int m_behaviour_script;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_
