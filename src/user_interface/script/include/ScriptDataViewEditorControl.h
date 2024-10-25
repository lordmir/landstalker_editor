#ifndef _SCRIPT_DATA_VIEW_EDITOR_CONTROL_
#define _SCRIPT_DATA_VIEW_EDITOR_CONTROL_

#include <list>
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <landstalker/script/include/ScriptTableEntry.h>
#include <landstalker/main/include/GameData.h>

class ScriptDataViewEditorControl : public wxWindow
{
public:
	ScriptDataViewEditorControl(wxWindow* parent, const wxRect& rect, uint16_t value, std::shared_ptr<const GameData> gd);
	virtual ~ScriptDataViewEditorControl();

	void SetValue(uint16_t value);
	uint16_t GetValue() const;
private:
	void OnClick(wxMouseEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnClearCheck(wxCommandEvent& evt);
	void OnEndCheck(wxCommandEvent& evt);
	void OnSpin(wxSpinEvent& evt);
	void OnTextEnter(wxCommandEvent& evt);
	void Render();
	void Update();
	virtual void SetFocus() override;

	ScriptTableEntryType m_type = ScriptTableEntryType::INVALID;
	std::unique_ptr<ScriptTableEntry> m_entry = nullptr;
	std::shared_ptr<const GameData> m_gd;
	std::list<wxWindow*> m_controls;

	wxStaticText* m_string_preview;
	wxSpinCtrl* m_string_select;
	wxCheckBox* m_clear;
	wxCheckBox* m_end;

	wxDECLARE_EVENT_TABLE();
};

#endif // _SCRIPT_DATA_VIEW_EDITOR_CONTROL_
