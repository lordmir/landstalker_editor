#ifndef _DATA_VIEW_SCRIPT_ACTION_EDITOR_CONTROL_H_
#define _DATA_VIEW_SCRIPT_ACTION_EDITOR_CONTROL_H_

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <landstalker/main/include/GameData.h>
#include <landstalker/script/include/ScriptTable.h>
#include <memory>
#include <vector>
#include <string>
#include <map>


class DataViewScriptActionEditorControl : public wxWindow
{
public:
	DataViewScriptActionEditorControl(wxWindow* parent, const wxRect& rect, const ScriptTable::Action& value, const std::vector<std::string>& choices, std::shared_ptr<GameData> gd);
	virtual ~DataViewScriptActionEditorControl();

	void SetValue(const ScriptTable::Action& value);
	ScriptTable::Action GetValue() const;
private:
	enum class Editor
	{
		NONE,
		SPIN_CTRL,
		COMBO_CTRL
	};
	void OnKeyDown(wxKeyEvent& evt);
	void OnChange(wxCommandEvent& evt);
	void OnSpin(wxSpinEvent& evt);
	void OnTextEnter(wxCommandEvent& evt);
	void UpdateControls() const;
	virtual void SetFocus() override;

	
	mutable ScriptTable::Action m_entry;
	wxArrayString m_choices;
	Editor m_editor;
	std::shared_ptr<GameData> m_gd;

	wxSpinCtrl* m_spin_ctrl = nullptr;
	wxComboBox* m_combo_ctrl = nullptr;
	wxStaticText* m_comment = nullptr;
};

#endif // _DATA_VIEW_SCRIPT_ACTION_EDITOR_CONTROL_H_
