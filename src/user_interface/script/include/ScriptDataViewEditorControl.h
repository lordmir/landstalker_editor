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
	void OnChangeType(wxCommandEvent& evt);
	void Render();
	void Update();
	virtual void SetFocus() override;

	void ShowEditor(const ScriptTableEntryType& type);

	ScriptTableEntryType m_type = ScriptTableEntryType::INVALID;
	std::unique_ptr<ScriptTableEntry> m_entry = nullptr;
	std::shared_ptr<const GameData> m_gd;

	wxBoxSizer* m_main_sizer = nullptr;
	wxBoxSizer* m_type_ctrl_sizer = nullptr;
	std::map<ScriptTableEntryType, wxPanel*> m_panels;
	std::map<ScriptTableEntryType, wxBoxSizer*> m_panel_sizers;
	wxChoice* m_type_select = nullptr;

	wxTextCtrl* m_string_preview = nullptr;
	wxSpinCtrl* m_string_select = nullptr;
	wxCheckBox* m_string_clear = nullptr;
	wxCheckBox* m_string_end = nullptr;

	wxChoice* m_load_global_char_slot = nullptr;
	wxSpinCtrl* m_load_global_char_select = nullptr;
	wxStaticText* m_load_global_char_name = nullptr;

	wxChoice* m_item_slot = nullptr;
	wxSpinCtrl* m_item_select = nullptr;
	wxStaticText* m_item_name = nullptr;

	wxSpinCtrl* m_num_select = nullptr;

	wxChoice* m_bgm_select = nullptr;

	wxSpinCtrl* m_cutscene_select = nullptr;
	wxStaticText* m_cutscene_name = nullptr;

	wxSpinCtrl* m_flag_select = nullptr;
	wxStaticText* m_flag_name = nullptr;

	wxSpinCtrl* m_set_global_char_select = nullptr;
	wxStaticText* m_set_global_char_name = nullptr;

	wxSpinCtrl* m_set_char_select = nullptr;
	wxStaticText* m_set_char_name = nullptr;

	wxSpinCtrl* m_custom_value = nullptr;

	wxDECLARE_EVENT_TABLE();
};

#endif // _SCRIPT_DATA_VIEW_EDITOR_CONTROL_
