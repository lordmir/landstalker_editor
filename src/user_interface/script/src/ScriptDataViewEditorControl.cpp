#include <user_interface/script/include/ScriptDataViewEditorControl.h>

#include <algorithm>

wxBEGIN_EVENT_TABLE(ScriptDataViewEditorControl, wxWindow)
EVT_KEY_DOWN(ScriptDataViewEditorControl::OnKeyDown)
EVT_LEFT_UP(ScriptDataViewEditorControl::OnClick)
EVT_CHECKBOX(wxID_CLEAR, ScriptDataViewEditorControl::OnClearCheck)
EVT_CHECKBOX(wxID_STOP, ScriptDataViewEditorControl::OnEndCheck)
EVT_SPINCTRL(wxID_ANY, ScriptDataViewEditorControl::OnSpin)
EVT_TEXT_ENTER(wxID_ANY, ScriptDataViewEditorControl::OnTextEnter)
EVT_TEXT_ENTER(wxID_PREVIEW, ScriptDataViewEditorControl::OnTextEnter)
EVT_CHOICE(wxID_ANY, ScriptDataViewEditorControl::OnChangeType)
wxEND_EVENT_TABLE()

ScriptDataViewEditorControl::ScriptDataViewEditorControl(wxWindow* parent, const wxRect& rect, uint16_t value, std::shared_ptr<const GameData> gd)
  : wxWindow(parent, wxID_ANY, rect.GetPosition(), rect.GetSize()),
	m_gd(gd)
{
	Freeze();
	m_main_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_type_ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_type_select = new wxChoice(this, wxID_ANY);
	m_type_select->Insert(std::vector<wxString>{ "String", "Set Item", "Set Global Char", "Set Number", "Set Flag", "Give Item To Player", "Give Money To Player", "Play BGM", "Set Speaker", "Set Global Speaker", "Play Cutscene", "Custom"}, 0);
	m_type_select->SetSize({ 160, m_type_select->GetBestHeight(160) });

	m_main_sizer->Add(m_type_select, wxLEFT);
	m_main_sizer->Add(m_type_ctrl_sizer, wxEXPAND);

	m_panels[ScriptTableEntryType::STRING] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::STRING] = new wxBoxSizer(wxHORIZONTAL);
	m_string_clear = new wxCheckBox(m_panels[ScriptTableEntryType::STRING], wxID_CLEAR, "Clear:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	m_string_end = new wxCheckBox(m_panels[ScriptTableEntryType::STRING], wxID_STOP, "End:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	m_string_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::STRING], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS,
		m_gd->GetScriptData()->GetStringStart(), m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN) - 1);
	m_string_preview = new wxTextCtrl(m_panels[ScriptTableEntryType::STRING], wxID_PREVIEW, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxWANTS_CHARS);
	wxFont tt_font = m_string_preview->GetFont();
	tt_font.SetFamily(wxFONTFAMILY_TELETYPE);
	m_string_preview->SetFont(tt_font);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::STRING], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::STRING]->Add(m_string_clear, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::STRING]->Add(m_string_end, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::STRING]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::STRING]->Add(new wxStaticText(m_panels[ScriptTableEntryType::STRING], wxID_ANY, "String:"), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::STRING]->Add(m_string_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::STRING]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::STRING]->Add(m_string_preview, wxEXPAND);
	m_panels[ScriptTableEntryType::STRING]->SetSizer(m_panel_sizers[ScriptTableEntryType::STRING]);

	m_panels[ScriptTableEntryType::PLAY_CUTSCENE] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::PLAY_CUTSCENE] = new wxBoxSizer(wxHORIZONTAL);
	m_cutscene_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::PLAY_CUTSCENE], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0,1023);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::PLAY_CUTSCENE], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::PLAY_CUTSCENE]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::PLAY_CUTSCENE]->Add(new wxStaticText(m_panels[ScriptTableEntryType::PLAY_CUTSCENE], wxID_ANY, "Cutscene Index: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::PLAY_CUTSCENE]->Add(m_cutscene_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::PLAY_CUTSCENE]->Add(new wxStaticText(m_panels[ScriptTableEntryType::PLAY_CUTSCENE], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[ScriptTableEntryType::PLAY_CUTSCENE]->SetSizer(m_panel_sizers[ScriptTableEntryType::PLAY_CUTSCENE]);

	m_panels[ScriptTableEntryType::GIVE_ITEM] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::GIVE_ITEM] = new wxBoxSizer(wxHORIZONTAL);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::GIVE_ITEM], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::GIVE_ITEM]->Add(new wxStaticText(m_panels[ScriptTableEntryType::GIVE_ITEM], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[ScriptTableEntryType::GIVE_ITEM]->SetSizer(m_panel_sizers[ScriptTableEntryType::GIVE_ITEM]);

	m_panels[ScriptTableEntryType::GIVE_MONEY] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::GIVE_MONEY] = new wxBoxSizer(wxHORIZONTAL);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::GIVE_MONEY], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::GIVE_MONEY]->Add(new wxStaticText(m_panels[ScriptTableEntryType::GIVE_MONEY], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[ScriptTableEntryType::GIVE_MONEY]->SetSizer(m_panel_sizers[ScriptTableEntryType::GIVE_MONEY]);

	m_panels[ScriptTableEntryType::ITEM_LOAD] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD] = new wxBoxSizer(wxHORIZONTAL);
	m_item_slot = new wxChoice(m_panels[ScriptTableEntryType::ITEM_LOAD], wxID_ANY);
	m_item_slot->Insert(std::vector<wxString>{ "1", "2", "3", "4" }, 0);
	m_item_slot->Select(0);
	m_item_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::ITEM_LOAD], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 63);
	m_item_name = new wxStaticText(m_panels[ScriptTableEntryType::ITEM_LOAD], wxID_ANY, wxEmptyString);
	m_item_name->SetFont(tt_font);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::ITEM_LOAD], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->Add(new wxStaticText(m_panels[ScriptTableEntryType::ITEM_LOAD], wxID_ANY, "Item Slot: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->Add(m_item_slot, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->AddSpacer(20);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->Add(new wxStaticText(m_panels[ScriptTableEntryType::ITEM_LOAD], wxID_ANY, "Item: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->Add(m_item_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->AddSpacer(20);
	m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]->Add(m_item_name, wxEXPAND);
	m_panels[ScriptTableEntryType::ITEM_LOAD]->SetSizer(m_panel_sizers[ScriptTableEntryType::ITEM_LOAD]);

	m_panels[ScriptTableEntryType::NUMBER_LOAD] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::NUMBER_LOAD] = new wxBoxSizer(wxHORIZONTAL);
	m_num_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::NUMBER_LOAD], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 999);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::NUMBER_LOAD], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::NUMBER_LOAD]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::NUMBER_LOAD]->Add(new wxStaticText(m_panels[ScriptTableEntryType::NUMBER_LOAD], wxID_ANY, "Number: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::NUMBER_LOAD]->Add(m_num_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::NUMBER_LOAD]->Add(new wxStaticText(m_panels[ScriptTableEntryType::NUMBER_LOAD], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[ScriptTableEntryType::NUMBER_LOAD]->SetSizer(m_panel_sizers[ScriptTableEntryType::NUMBER_LOAD]);

	m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD] = new wxBoxSizer(wxHORIZONTAL);
	m_load_global_char_slot = new wxChoice(m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD], wxID_ANY);
	m_load_global_char_slot->Insert(std::vector<wxString>{ "1", "2", "3", "4" }, 0);
	m_load_global_char_slot->Select(0);
	m_load_global_char_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 23);
	m_load_global_char_name = new wxStaticText(m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD], wxID_ANY, wxEmptyString);
	m_load_global_char_name->SetFont(tt_font);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->Add(new wxStaticText(m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD], wxID_ANY, "Char Slot: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->Add(m_load_global_char_slot, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->AddSpacer(20);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->Add(new wxStaticText(m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD], wxID_ANY, "Character: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->Add(m_load_global_char_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->AddSpacer(20);
	m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->Add(m_load_global_char_name, wxEXPAND);
	m_panels[ScriptTableEntryType::GLOBAL_CHAR_LOAD]->SetSizer(m_panel_sizers[ScriptTableEntryType::GLOBAL_CHAR_LOAD]);

	m_panels[ScriptTableEntryType::SET_FLAG] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::SET_FLAG] = new wxBoxSizer(wxHORIZONTAL);
	m_flag_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::SET_FLAG], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 999);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::SET_FLAG], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::SET_FLAG]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::SET_FLAG]->Add(new wxStaticText(m_panels[ScriptTableEntryType::SET_FLAG], wxID_ANY, "Flag: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::SET_FLAG]->Add(m_flag_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::SET_FLAG]->Add(new wxStaticText(m_panels[ScriptTableEntryType::SET_FLAG], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[ScriptTableEntryType::SET_FLAG]->SetSizer(m_panel_sizers[ScriptTableEntryType::SET_FLAG]);

	m_panels[ScriptTableEntryType::SET_SPEAKER] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::SET_SPEAKER] = new wxBoxSizer(wxHORIZONTAL);
	m_set_char_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::SET_SPEAKER], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 999);
	m_set_char_name = new wxStaticText(m_panels[ScriptTableEntryType::SET_SPEAKER], wxID_ANY, wxEmptyString);
	m_set_char_name->SetFont(tt_font);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::SET_SPEAKER], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::SET_SPEAKER]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::SET_SPEAKER]->Add(new wxStaticText(m_panels[ScriptTableEntryType::SET_SPEAKER], wxID_ANY, "Character: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::SET_SPEAKER]->Add(m_set_char_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::SET_SPEAKER]->AddSpacer(20);
	m_panel_sizers[ScriptTableEntryType::SET_SPEAKER]->Add(m_set_char_name, wxEXPAND);
	m_panels[ScriptTableEntryType::SET_SPEAKER]->SetSizer(m_panel_sizers[ScriptTableEntryType::SET_SPEAKER]);

	m_panels[ScriptTableEntryType::SET_GLOBAL_SPEAKER] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::SET_GLOBAL_SPEAKER] = new wxBoxSizer(wxHORIZONTAL);
	m_set_global_char_select = new wxSpinCtrl(m_panels[ScriptTableEntryType::SET_GLOBAL_SPEAKER], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 23);
	m_set_global_char_name = new wxStaticText(m_panels[ScriptTableEntryType::SET_GLOBAL_SPEAKER], wxID_ANY, wxEmptyString);
	m_set_global_char_name->SetFont(tt_font);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::SET_GLOBAL_SPEAKER], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::SET_GLOBAL_SPEAKER]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::SET_GLOBAL_SPEAKER]->Add(new wxStaticText(m_panels[ScriptTableEntryType::SET_GLOBAL_SPEAKER], wxID_ANY, "Character: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::SET_GLOBAL_SPEAKER]->Add(m_set_global_char_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::SET_GLOBAL_SPEAKER]->AddSpacer(20);
	m_panel_sizers[ScriptTableEntryType::SET_GLOBAL_SPEAKER]->Add(m_set_global_char_name, wxEXPAND);
	m_panels[ScriptTableEntryType::SET_GLOBAL_SPEAKER]->SetSizer(m_panel_sizers[ScriptTableEntryType::SET_GLOBAL_SPEAKER]);

	m_panels[ScriptTableEntryType::PLAY_BGM] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::PLAY_BGM] = new wxBoxSizer(wxHORIZONTAL);
	m_bgm_select = new wxChoice(m_panels[ScriptTableEntryType::PLAY_BGM], wxID_ANY);
	auto bgms = std::vector<wxString>{};
	std::transform(ScriptPlayBgmEntry::BGMS.cbegin(), ScriptPlayBgmEntry::BGMS.cend(), std::back_inserter(bgms), [](const auto& bgm) {
		return *Labels::Get(Labels::C_SOUNDS, bgm);
	});
	m_bgm_select->Insert(bgms, 0);
	m_bgm_select->Select(0);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::PLAY_BGM], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::PLAY_BGM]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::PLAY_BGM]->Add(new wxStaticText(m_panels[ScriptTableEntryType::PLAY_BGM], wxID_ANY, "BGM: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::PLAY_BGM]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::PLAY_BGM]->Add(m_bgm_select, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::PLAY_BGM]->AddSpacer(20);
	m_panel_sizers[ScriptTableEntryType::PLAY_BGM]->Add(new wxStaticText(m_panels[ScriptTableEntryType::PLAY_BGM], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[ScriptTableEntryType::PLAY_BGM]->SetSizer(m_panel_sizers[ScriptTableEntryType::PLAY_BGM]);

	m_panels[ScriptTableEntryType::INVALID] = new wxPanel(this);
	m_panel_sizers[ScriptTableEntryType::INVALID] = new wxBoxSizer(wxHORIZONTAL);
	m_custom_value = new wxSpinCtrl(m_panels[ScriptTableEntryType::INVALID], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 0xFFFF);
	m_custom_value->SetBase(16);
	m_type_ctrl_sizer->Add(m_panels[ScriptTableEntryType::INVALID], 1, wxGROW);
	m_panel_sizers[ScriptTableEntryType::INVALID]->AddSpacer(10);
	m_panel_sizers[ScriptTableEntryType::INVALID]->Add(new wxStaticText(m_panels[ScriptTableEntryType::INVALID], wxID_ANY, "Custom Value: "), wxLEFT);
	m_panel_sizers[ScriptTableEntryType::INVALID]->Add(m_custom_value, wxLEFT);
	m_panel_sizers[ScriptTableEntryType::INVALID]->Add(new wxStaticText(m_panels[ScriptTableEntryType::INVALID], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[ScriptTableEntryType::INVALID]->SetSizer(m_panel_sizers[ScriptTableEntryType::INVALID]);
	
	this->SetSizer(m_main_sizer);
	GetSizer()->Fit(this);
	Thaw();
	SetValue(value);

	m_string_preview->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_item_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_item_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_num_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_num_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_load_global_char_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_load_global_char_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_flag_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_flag_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_set_char_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_set_char_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_set_global_char_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_set_global_char_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_custom_value->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_custom_value->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
}

ScriptDataViewEditorControl::~ScriptDataViewEditorControl()
{
	m_string_preview->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_item_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_item_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_num_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_num_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_load_global_char_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_load_global_char_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_flag_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_flag_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_set_char_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_set_char_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_set_global_char_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_set_global_char_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_custom_value->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_custom_value->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
}

void ScriptDataViewEditorControl::SetValue(uint16_t value)
{
	Freeze();
	m_entry = std::move(ScriptTableEntry::FromBytes(value));
	m_type = m_entry->GetType();
	if (m_type >= ScriptTableEntryType::STRING && m_type <= ScriptTableEntryType::PLAY_CUTSCENE)
	{
		m_type_select->Select(static_cast<int>(m_type) - 1);
	}
	else
	{
		m_type_select->Select(m_type_select->GetCount() - 1);
	}
	ShowEditor(m_type);

	switch (m_type)
	{
	case ScriptTableEntryType::STRING:
		{
			const auto& string_entry = dynamic_cast<const ScriptStringEntry&>(*m_entry);
			std::size_t string_idx = string_entry.string + m_gd->GetScriptData()->GetStringStart();
			m_string_clear->SetValue(string_entry.clear_box);
			m_string_end->SetValue(string_entry.end);
			m_string_select->SetValue(string_idx);
			if (string_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN))
			{
				m_string_preview->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::MAIN, string_idx));
			}
			else
			{
				m_string_preview->SetLabelText("???");
			}
		}
		break;
	case ScriptTableEntryType::PLAY_CUTSCENE:
		m_cutscene_select->SetValue(dynamic_cast<const ScriptInitiateCutsceneEntry&>(*m_entry).cutscene);
		break;
	case ScriptTableEntryType::ITEM_LOAD:
		{
			const auto& item_entry = dynamic_cast<const ScriptItemLoadEntry&>(*m_entry);
			m_item_slot->Select(item_entry.slot);
			m_item_select->SetValue(item_entry.item);
			if (item_entry.item < m_gd->GetStringData()->GetStringCount(StringData::Type::ITEM_NAMES))
			{
				m_item_name->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::ITEM_NAMES, item_entry.item));
			}
			else
			{
				m_item_name->SetLabelText("???");
			}
		}
		break;
	case ScriptTableEntryType::NUMBER_LOAD:
		m_num_select->SetValue(dynamic_cast<const ScriptNumLoadEntry&>(*m_entry).num);
		break;
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		{
			const auto& char_entry = dynamic_cast<const ScriptGlobalCharLoadEntry&>(*m_entry);
			m_load_global_char_slot->Select(char_entry.slot);
			m_load_global_char_select->SetValue(char_entry.chr);
			if (char_entry.chr < m_gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
			{
				m_load_global_char_name->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::SPECIAL_NAMES, char_entry.chr));
			}
			else
			{
				m_load_global_char_name->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::DEFAULT_NAME, 0));
			}
		}
		break;
	case ScriptTableEntryType::SET_FLAG:
		m_flag_select->SetValue(dynamic_cast<const ScriptSetFlagEntry&>(*m_entry).flag);
		break;
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		{
			const auto& char_entry = dynamic_cast<const ScriptSetGlobalSpeakerEntry&>(*m_entry);
			m_set_global_char_select->SetValue(char_entry.chr);
			if (char_entry.chr < m_gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
			{
				m_set_global_char_name->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::SPECIAL_NAMES, char_entry.chr));
			}
			else
			{
				m_set_global_char_name->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::DEFAULT_NAME, 0));
			}
		}
		break;
	case ScriptTableEntryType::SET_SPEAKER:
		{
			const auto& char_entry = dynamic_cast<const ScriptSetSpeakerEntry&>(*m_entry);
			m_set_char_select->SetValue(char_entry.chr);
			if (char_entry.chr < m_gd->GetStringData()->GetStringCount(StringData::Type::NAMES))
			{
				m_set_char_name->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::NAMES, char_entry.chr));
			}
			else
			{
				m_set_char_name->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::DEFAULT_NAME, 0));
			}
		}
		break;
	case ScriptTableEntryType::PLAY_BGM:
		m_bgm_select->Select(dynamic_cast<const ScriptPlayBgmEntry&>(*m_entry).bgm);
		break;
	case ScriptTableEntryType::INVALID:
		m_custom_value->SetValue(m_entry->ToBytes());
		break;
	case ScriptTableEntryType::GIVE_ITEM:
	case ScriptTableEntryType::GIVE_MONEY:
	default:
		break;
	}
	Thaw();
}

uint16_t ScriptDataViewEditorControl::GetValue() const
{
	if (m_entry)
	{
		if (m_entry->GetType() == ScriptTableEntryType::STRING)
		{
			const auto& string_entry = dynamic_cast<const ScriptStringEntry&>(*m_entry);
			std::size_t string_idx = string_entry.string + m_gd->GetScriptData()->GetStringStart();
			if (m_string_preview->GetValue().ToStdWstring() != m_gd->GetStringData()->GetString(StringData::Type::MAIN, string_idx))
			{
				m_gd->GetStringData()->SetString(StringData::Type::MAIN, string_idx, m_string_preview->GetValue().ToStdWstring());
			}
		}
		return m_entry->ToBytes();
	}
	return 0xFFFF;
}

void ScriptDataViewEditorControl::OnClick(wxMouseEvent& evt)
{
	evt.Skip();
}

void ScriptDataViewEditorControl::OnKeyDown(wxKeyEvent& evt)
{
	if (evt.GetKeyCode() == WXK_RETURN)
	{
		Update();
		evt.Skip();
	}
	else
	{
		evt.Skip();
	}
}

void ScriptDataViewEditorControl::OnClearCheck(wxCommandEvent& evt)
{
	Update();
	evt.Skip();
}

void ScriptDataViewEditorControl::OnEndCheck(wxCommandEvent& evt)
{
	Update();
	evt.Skip();
}

void ScriptDataViewEditorControl::OnSpin(wxSpinEvent& evt)
{
	Update();
	evt.Skip();
}

void ScriptDataViewEditorControl::OnTextEnter(wxCommandEvent& evt)
{
	Update();
	evt.Skip();
}

void ScriptDataViewEditorControl::OnChangeType(wxCommandEvent& evt)
{
	m_type = ScriptTableEntryType::INVALID;
	int selected = m_type_select->GetSelection();
	if (selected >= (static_cast<int>(ScriptTableEntryType::STRING) - 1) && selected <= (static_cast<int>(ScriptTableEntryType::PLAY_CUTSCENE) - 1))
	{
		m_type = static_cast<ScriptTableEntryType>(selected + 1);
	}
	m_entry = std::move(ScriptTableEntry::MakeEntry(m_type));
	if (m_type == ScriptTableEntryType::INVALID)
	{
		ShowEditor(m_type);
	}
	else
	{
		Update();
	}
	evt.Skip();
}

void ScriptDataViewEditorControl::Render()
{
}

void ScriptDataViewEditorControl::Update()
{
	switch (m_entry->GetType())
	{
	case ScriptTableEntryType::STRING:
		{
			auto& string_entry = dynamic_cast<ScriptStringEntry&>(*m_entry);
			std::size_t string_idx = m_string_select->GetValue() - m_gd->GetScriptData()->GetStringStart();
			string_entry.end = m_string_end->GetValue();
			string_entry.clear_box = m_string_clear->GetValue();
			if (string_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN))
			{
				string_entry.string = string_idx;
			}
		}
		break;
	case ScriptTableEntryType::PLAY_CUTSCENE:
		dynamic_cast<ScriptInitiateCutsceneEntry&>(*m_entry).cutscene = static_cast<uint16_t>(m_cutscene_select->GetValue());
		break;
	case ScriptTableEntryType::ITEM_LOAD:
		{
			auto& item_entry = dynamic_cast<ScriptItemLoadEntry&>(*m_entry);
			std::size_t item_idx = m_item_select->GetValue();
			item_entry.slot = m_item_slot->GetSelection();
			if (item_idx < 64)
			{
				item_entry.item = item_idx;
			}
		}
		break;
	case ScriptTableEntryType::NUMBER_LOAD:
		dynamic_cast<ScriptNumLoadEntry&>(*m_entry).num = m_num_select->GetValue();
		break;
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		{
			auto& char_entry = dynamic_cast<ScriptGlobalCharLoadEntry&>(*m_entry);
			std::size_t char_idx = m_load_global_char_select->GetValue();
			char_entry.slot = m_item_slot->GetSelection();
			if (char_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
			{
				char_entry.chr = char_idx;
			}
		}
		break;
	case ScriptTableEntryType::SET_FLAG:
		dynamic_cast<ScriptSetFlagEntry&>(*m_entry).flag = m_flag_select->GetValue();
		break;
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		{
			auto& char_entry = dynamic_cast<ScriptSetGlobalSpeakerEntry&>(*m_entry);
			std::size_t char_idx = m_set_global_char_select->GetValue();
			if (char_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
			{
				char_entry.chr = char_idx;
			}
		}
		break;
	case ScriptTableEntryType::SET_SPEAKER:
		dynamic_cast<ScriptSetSpeakerEntry&>(*m_entry).chr = m_set_char_select->GetValue();
		break;
	case ScriptTableEntryType::PLAY_BGM:
		dynamic_cast<ScriptPlayBgmEntry&>(*m_entry).bgm = m_bgm_select->GetSelection();
		break;
	case ScriptTableEntryType::INVALID:
		m_entry = std::move(ScriptTableEntry::FromBytes(m_custom_value->GetValue()));
		break;
	case ScriptTableEntryType::GIVE_ITEM:
	case ScriptTableEntryType::GIVE_MONEY:
	default:
		break;
	}
	if (m_type != ScriptTableEntryType::INVALID)
	{
		SetValue(m_entry->ToBytes());
	}
}

void ScriptDataViewEditorControl::SetFocus()
{
	switch (m_type)
	{
	case ScriptTableEntryType::STRING:
		m_string_select->SetFocus();
		m_string_select->SetSelection(0, m_string_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::PLAY_CUTSCENE:
		m_cutscene_select->SetFocus();
		m_cutscene_select->SetSelection(0, m_cutscene_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::ITEM_LOAD:
		m_item_select->SetFocus();
		m_item_select->SetSelection(0, m_item_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::NUMBER_LOAD:
		m_num_select->SetFocus();
		m_num_select->SetSelection(0, m_num_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		m_load_global_char_select->SetFocus();
		m_load_global_char_select->SetSelection(0, m_load_global_char_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::SET_FLAG:
		m_flag_select->SetFocus();
		m_flag_select->SetSelection(0, m_flag_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		m_set_global_char_select->SetFocus();
		m_set_global_char_select->SetSelection(0, m_set_global_char_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::SET_SPEAKER:
		m_set_char_select->SetFocus();
		m_set_char_select->SetSelection(0, m_set_char_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::PLAY_BGM:
		m_bgm_select->SetFocus();
		break;
	case ScriptTableEntryType::INVALID:
		m_custom_value->SetFocus();
		m_custom_value->SetSelection(0, m_custom_value->GetTextValue().Length());
		break;
	case ScriptTableEntryType::GIVE_ITEM:
	case ScriptTableEntryType::GIVE_MONEY:
	default:
		break;
	}
}

void ScriptDataViewEditorControl::ShowEditor(const ScriptTableEntryType& type)
{
	for (const auto& panel : m_panels)
	{
		m_type_ctrl_sizer->Show(panel.second, panel.first == type);
	}
	m_type_ctrl_sizer->Layout();
}
