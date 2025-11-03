#ifndef _SCRIPT_DATA_VIEW_EDITOR_CONTROL_
#define _SCRIPT_DATA_VIEW_EDITOR_CONTROL_

#include <list>
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <landstalker/script/ScriptTableEntry.h>
#include <landstalker/main/GameData.h>

class ScriptDataViewEditorControl : public wxWindow
{
public:
	ScriptDataViewEditorControl(wxWindow* parent, const wxRect& rect, uint16_t value, std::shared_ptr<const Landstalker::GameData> gd);
	virtual ~ScriptDataViewEditorControl();

	void SetValue(uint16_t value);
	uint16_t GetValue() const;
private:
	enum class EditorMode
	{
		STRING,
		SLOT_AND_VALUE,
		VALUE,
		BGM,
		NONE
	};
	void OnClick(wxMouseEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnSpin(wxSpinEvent& evt);
	void OnTextEnter(wxCommandEvent& evt);
	void OnChangeType(wxCommandEvent& evt);
	void Render();
	void Update();
	virtual void SetFocus() override;

	void ShowEditor(const Landstalker::ScriptTableEntryType& type);

	Landstalker::ScriptTableEntryType m_type = Landstalker::ScriptTableEntryType::INVALID;
	EditorMode m_mode = EditorMode::NONE;
	std::unique_ptr<Landstalker::ScriptTableEntry> m_entry = nullptr;
	std::shared_ptr<const Landstalker::GameData> m_gd;

	wxBoxSizer* m_main_sizer = nullptr;
	wxBoxSizer* m_type_ctrl_sizer = nullptr;
	std::map<EditorMode, wxPanel*> m_panels;
	std::map<EditorMode, wxBoxSizer*> m_panel_sizers;
	wxChoice* m_type_select = nullptr;

	wxTextCtrl* m_string_preview = nullptr;
	wxSpinCtrl* m_string_select = nullptr;

	wxChoice* m_slot_select = nullptr;
	wxStaticText* m_slot_value_label = nullptr;
	wxSpinCtrl* m_slot_value_select = nullptr;
	wxStaticText* m_slot_value_preview = nullptr;

	wxStaticText* m_value_label = nullptr;
	wxSpinCtrl* m_value_select = nullptr;
	wxStaticText* m_value_preview = nullptr;

	wxStaticText* m_null_preview = nullptr;

	wxChoice* m_bgm_select = nullptr;

	wxDECLARE_EVENT_TABLE();
};

#endif // _SCRIPT_DATA_VIEW_EDITOR_CONTROL_
