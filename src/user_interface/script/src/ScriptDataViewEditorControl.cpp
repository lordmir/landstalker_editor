#include <user_interface/script/include/ScriptDataViewEditorControl.h>

#include <algorithm>

wxBEGIN_EVENT_TABLE(ScriptDataViewEditorControl, wxWindow)
EVT_KEY_DOWN(ScriptDataViewEditorControl::OnKeyDown)
EVT_LEFT_UP(ScriptDataViewEditorControl::OnClick)
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
	SetBackgroundColour(*wxWHITE);
	SetBackgroundStyle(wxBackgroundStyle::wxBG_STYLE_SYSTEM);
	m_main_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_type_ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);
	m_type_select = new wxChoice(this, wxID_ANY);
	m_type_select->Insert(std::vector<wxString>{ "String", "Set Item", "Set Global Char", "Set Number", "Set Flag", "Give Item To Player", "Give Money To Player", "Play BGM", "Set Speaker", "Set Global Speaker", "Play Cutscene", "Custom"}, 0);
	m_type_select->SetSize({ 160, m_type_select->GetBestHeight(160) });

	m_main_sizer->Add(m_type_select, wxLEFT);
	m_main_sizer->Add(m_type_ctrl_sizer, wxEXPAND);

	m_panels[EditorMode::STRING] = new wxPanel(this);
	m_panel_sizers[EditorMode::STRING] = new wxBoxSizer(wxHORIZONTAL);
	m_string_select = new wxSpinCtrl(m_panels[EditorMode::STRING], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS,
		m_gd->GetScriptData()->GetStringStart(), m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN) - 1);
	m_string_preview = new wxTextCtrl(m_panels[EditorMode::STRING], wxID_PREVIEW, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxWANTS_CHARS);
	wxFont tt_font = m_string_preview->GetFont();
	tt_font.SetFamily(wxFONTFAMILY_TELETYPE);
	m_string_preview->SetFont(tt_font);
	m_type_ctrl_sizer->Add(m_panels[EditorMode::STRING], 1, wxGROW);
	m_panel_sizers[EditorMode::STRING]->AddSpacer(10);
	m_panel_sizers[EditorMode::STRING]->Add(new wxStaticText(m_panels[EditorMode::STRING], wxID_ANY, "String:"), wxLEFT);
	m_panel_sizers[EditorMode::STRING]->Add(m_string_select, wxLEFT);
	m_panel_sizers[EditorMode::STRING]->AddSpacer(10);
	m_panel_sizers[EditorMode::STRING]->Add(m_string_preview, wxEXPAND);
	m_panels[EditorMode::STRING]->SetSizer(m_panel_sizers[EditorMode::STRING]);

	m_panels[EditorMode::VALUE] = new wxPanel(this);
	m_panel_sizers[EditorMode::VALUE] = new wxBoxSizer(wxHORIZONTAL);
	m_value_label = new wxStaticText(m_panels[EditorMode::VALUE], wxID_ANY, "???: ");
	m_value_select = new wxSpinCtrl(m_panels[EditorMode::VALUE], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0,1023);
	m_type_ctrl_sizer->Add(m_panels[EditorMode::VALUE], 1, wxGROW);
	m_value_preview = new wxStaticText(m_panels[EditorMode::VALUE], wxID_ANY, wxEmptyString);
	m_value_preview->SetFont(tt_font);
	m_panel_sizers[EditorMode::VALUE]->AddSpacer(10);
	m_panel_sizers[EditorMode::VALUE]->Add(m_value_label, wxLEFT);
	m_panel_sizers[EditorMode::VALUE]->Add(m_value_select, wxLEFT);
	m_panel_sizers[EditorMode::VALUE]->AddSpacer(20);
	m_panel_sizers[EditorMode::VALUE]->Add(m_value_preview, wxEXPAND);
	m_panels[EditorMode::VALUE]->SetSizer(m_panel_sizers[EditorMode::VALUE]);

	m_panels[EditorMode::NONE] = new wxPanel(this);
	m_panel_sizers[EditorMode::NONE] = new wxBoxSizer(wxHORIZONTAL);
	m_type_ctrl_sizer->Add(m_panels[EditorMode::NONE], 1, wxGROW);
	m_null_preview = new wxStaticText(m_panels[EditorMode::NONE], wxID_ANY, wxEmptyString);
	m_panel_sizers[EditorMode::NONE]->Add(m_null_preview, wxEXPAND);
	m_panels[EditorMode::NONE]->SetSizer(m_panel_sizers[EditorMode::NONE]);

	m_panels[EditorMode::SLOT_AND_VALUE] = new wxPanel(this);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE] = new wxBoxSizer(wxHORIZONTAL);
	m_slot_select = new wxChoice(m_panels[EditorMode::SLOT_AND_VALUE], wxID_ANY);
	m_slot_select->Insert(std::vector<wxString>{ "1", "2", "3", "4" }, 0);
	m_slot_select->Select(0);
	m_slot_value_label = new wxStaticText(m_panels[EditorMode::SLOT_AND_VALUE], wxID_ANY, "???: ");
	m_slot_value_select = new wxSpinCtrl(m_panels[EditorMode::SLOT_AND_VALUE], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 63);
	m_slot_value_preview = new wxStaticText(m_panels[EditorMode::SLOT_AND_VALUE], wxID_ANY, wxEmptyString);
	m_slot_value_preview->SetFont(tt_font);
	m_type_ctrl_sizer->Add(m_panels[EditorMode::SLOT_AND_VALUE], 1, wxGROW);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->AddSpacer(10);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->Add(new wxStaticText(m_panels[EditorMode::SLOT_AND_VALUE], wxID_ANY, "Slot: "), wxLEFT);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->AddSpacer(10);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->Add(m_slot_select, wxLEFT);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->AddSpacer(20);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->Add(m_slot_value_label, wxLEFT);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->Add(m_slot_value_select, wxLEFT);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->AddSpacer(20);
	m_panel_sizers[EditorMode::SLOT_AND_VALUE]->Add(m_slot_value_preview, wxEXPAND);
	m_panels[EditorMode::SLOT_AND_VALUE]->SetSizer(m_panel_sizers[EditorMode::SLOT_AND_VALUE]);

	m_panels[EditorMode::BGM] = new wxPanel(this);
	m_panel_sizers[EditorMode::BGM] = new wxBoxSizer(wxHORIZONTAL);
	m_bgm_select = new wxChoice(m_panels[EditorMode::BGM], wxID_ANY);
	auto bgms = std::vector<wxString>{};
	std::transform(ScriptPlayBgmEntry::BGMS.cbegin(), ScriptPlayBgmEntry::BGMS.cend(), std::back_inserter(bgms), [](const auto& bgm) {
		return *Labels::Get(Labels::C_SOUNDS, bgm);
	});
	m_bgm_select->Insert(bgms, 0);
	m_bgm_select->Select(0);
	m_type_ctrl_sizer->Add(m_panels[EditorMode::BGM], 1, wxGROW);
	m_panel_sizers[EditorMode::BGM]->AddSpacer(10);
	m_panel_sizers[EditorMode::BGM]->Add(new wxStaticText(m_panels[EditorMode::BGM], wxID_ANY, "BGM: "), wxLEFT);
	m_panel_sizers[EditorMode::BGM]->AddSpacer(10);
	m_panel_sizers[EditorMode::BGM]->Add(m_bgm_select, wxLEFT);
	m_panel_sizers[EditorMode::BGM]->AddSpacer(20);
	m_panel_sizers[EditorMode::BGM]->Add(new wxStaticText(m_panels[EditorMode::BGM], wxID_ANY, wxEmptyString), wxEXPAND);
	m_panels[EditorMode::BGM]->SetSizer(m_panel_sizers[EditorMode::BGM]);
	
	this->SetSizer(m_main_sizer);
	GetSizer()->Fit(this);
	Thaw();
	SetValue(m_gd->GetScriptData()->GetScript()->GetScriptLine(value).ToBytes());

	m_string_preview->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_slot_value_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_slot_value_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_value_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_value_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
}

ScriptDataViewEditorControl::~ScriptDataViewEditorControl()
{
	m_string_preview->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_slot_value_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_slot_value_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_value_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_value_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
}

void ScriptDataViewEditorControl::SetValue(uint16_t value)
{
	m_entry = ScriptTableEntry::FromBytes(value);
	m_type = m_entry->GetType();
	if (m_type >= ScriptTableEntryType::STRING && m_type <= ScriptTableEntryType::PLAY_CUTSCENE)
	{
		m_type_select->Select(static_cast<int>(m_type) - 1);
	}
	else
	{
		// Select invalid entry
		m_type_select->Select(m_type_select->GetCount() - 1);
	}

	ShowEditor(m_type);

	Freeze();
	switch (m_type)
	{
	case ScriptTableEntryType::STRING:
		{
			const auto& string_entry = dynamic_cast<const ScriptStringEntry&>(*m_entry);
			std::size_t string_idx = string_entry.string + m_gd->GetScriptData()->GetStringStart();
			m_string_select->SetValue(string_idx);
			if (string_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN))
			{
				m_string_preview->SetValue(m_gd->GetStringData()->GetString(StringData::Type::MAIN, string_idx));
			}
			else
			{
				m_string_preview->SetValue("???");
			}
		}
		break;
	case ScriptTableEntryType::PLAY_CUTSCENE:
		{
			const auto& cutscene_entry = dynamic_cast<const ScriptInitiateCutsceneEntry&>(*m_entry);
			m_value_select->SetValue(cutscene_entry.cutscene);
			m_value_preview->SetLabelText(m_gd->GetScriptData()->GetCutsceneDisplayName(cutscene_entry.cutscene));
		}
		break;
	case ScriptTableEntryType::ITEM_LOAD:
		{
			const auto& item_entry = dynamic_cast<const ScriptItemLoadEntry&>(*m_entry);
			m_slot_select->Select(item_entry.slot);
			m_slot_value_select ->SetValue(item_entry.item);
			m_slot_value_preview->SetLabelText(m_gd->GetStringData()->GetItemDisplayName(item_entry.item));
		}
		break;
	case ScriptTableEntryType::NUMBER_LOAD:
		m_value_select->SetValue(dynamic_cast<const ScriptNumLoadEntry&>(*m_entry).num);
		break;
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		{
			const auto& char_entry = dynamic_cast<const ScriptGlobalCharLoadEntry&>(*m_entry);
			m_slot_select->Select(char_entry.slot);
			m_slot_value_select->SetValue(char_entry.chr);
			m_slot_value_preview->SetLabelText(m_gd->GetStringData()->GetGlobalCharacterDisplayName(char_entry.chr));
		}
		break;
	case ScriptTableEntryType::SET_FLAG:
		{
			const auto& flag_entry = dynamic_cast<const ScriptSetFlagEntry&>(*m_entry);
			m_value_select->SetValue(flag_entry.flag);
			m_value_preview->SetLabelText(ScriptData::GetFlagDisplayName(flag_entry.flag));
		}
		break;
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		{
			const auto& char_entry = dynamic_cast<const ScriptSetGlobalSpeakerEntry&>(*m_entry);
			m_value_select->SetValue(char_entry.chr);
			m_value_preview->SetLabelText(m_gd->GetStringData()->GetGlobalCharacterDisplayName(char_entry.chr));
		}
		break;
	case ScriptTableEntryType::SET_SPEAKER:
		{
			const auto& char_entry = dynamic_cast<const ScriptSetSpeakerEntry&>(*m_entry);
			m_value_select->SetValue(char_entry.chr);
			m_value_preview->SetLabelText(m_gd->GetStringData()->GetCharacterDisplayName(char_entry.chr));
		}
		break;
	case ScriptTableEntryType::PLAY_BGM:
		m_bgm_select->Select(dynamic_cast<const ScriptPlayBgmEntry&>(*m_entry).bgm);
		break;
	case ScriptTableEntryType::INVALID:
		m_value_select->SetValue(m_entry->ToBytes());
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
	bool clear = m_entry->GetClear();
	bool end = m_entry->GetEnd();
	uint16_t data = m_entry->GetData();
	m_entry = std::move(ScriptTableEntry::MakeEntry(m_type));
	m_entry->SetClear(clear);
	m_entry->SetEnd(end);
	m_entry->SetData(data);
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
			if (string_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN))
			{
				string_entry.string = string_idx;
			}
		}
		break;
	case ScriptTableEntryType::PLAY_CUTSCENE:
		dynamic_cast<ScriptInitiateCutsceneEntry&>(*m_entry).cutscene = static_cast<uint16_t>(m_value_select->GetValue());
		break;
	case ScriptTableEntryType::ITEM_LOAD:
		{
			auto& item_entry = dynamic_cast<ScriptItemLoadEntry&>(*m_entry);
			std::size_t item_idx = m_slot_value_select->GetValue();
			item_entry.slot = m_slot_select->GetSelection();
			if (item_idx < 64)
			{
				item_entry.item = item_idx;
			}
		}
		break;
	case ScriptTableEntryType::NUMBER_LOAD:
		dynamic_cast<ScriptNumLoadEntry&>(*m_entry).num = m_value_select->GetValue();
		break;
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		{
			auto& char_entry = dynamic_cast<ScriptGlobalCharLoadEntry&>(*m_entry);
			std::size_t char_idx = m_slot_value_select->GetValue();
			char_entry.slot = m_slot_select->GetSelection();
			if (char_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
			{
				char_entry.chr = char_idx;
			}
		}
		break;
	case ScriptTableEntryType::SET_FLAG:
		dynamic_cast<ScriptSetFlagEntry&>(*m_entry).flag = m_value_select->GetValue();
		break;
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		{
			auto& char_entry = dynamic_cast<ScriptSetGlobalSpeakerEntry&>(*m_entry);
			std::size_t char_idx = m_value_select->GetValue();
			if (char_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::SPECIAL_NAMES))
			{
				char_entry.chr = char_idx;
			}
		}
		break;
	case ScriptTableEntryType::SET_SPEAKER:
		dynamic_cast<ScriptSetSpeakerEntry&>(*m_entry).chr = m_value_select->GetValue();
		break;
	case ScriptTableEntryType::PLAY_BGM:
		dynamic_cast<ScriptPlayBgmEntry&>(*m_entry).bgm = m_bgm_select->GetSelection();
		break;
	case ScriptTableEntryType::INVALID:
		m_entry = std::move(ScriptTableEntry::FromBytes(m_value_select->GetValue()));
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
	case ScriptTableEntryType::NUMBER_LOAD:
	case ScriptTableEntryType::SET_FLAG:
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
	case ScriptTableEntryType::SET_SPEAKER:
	case ScriptTableEntryType::INVALID:
		m_value_select->SetFocus();
		m_value_select->SetSelection(0, m_value_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::ITEM_LOAD:
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		m_slot_value_select->SetFocus();
		m_slot_value_select->SetSelection(0, m_slot_value_select->GetTextValue().Length());
		break;
	case ScriptTableEntryType::PLAY_BGM:
		m_bgm_select->SetFocus();
		break;
	case ScriptTableEntryType::GIVE_ITEM:
	case ScriptTableEntryType::GIVE_MONEY:
	default:
		break;
	}
}

void ScriptDataViewEditorControl::ShowEditor(const ScriptTableEntryType& type)
{
	Freeze();
	switch (type)
	{
	case ScriptTableEntryType::STRING:
		m_mode = EditorMode::STRING;
		break;
	case ScriptTableEntryType::ITEM_LOAD:
		m_mode = EditorMode::SLOT_AND_VALUE;
		m_slot_value_label->SetLabelText("Item: ");
		m_slot_value_preview->SetLabelText(wxEmptyString);
		m_slot_value_select->SetRange(0, 0x3F);
		break;
	case ScriptTableEntryType::GLOBAL_CHAR_LOAD:
		m_mode = EditorMode::SLOT_AND_VALUE;
		m_slot_value_label->SetLabelText("Global Character: ");
		m_slot_value_preview->SetLabelText(wxEmptyString);
		m_slot_value_select->SetRange(0, 23);
		break;
	case ScriptTableEntryType::NUMBER_LOAD:
		m_mode = EditorMode::VALUE;
		m_value_label->SetLabelText("Number: ");
		m_value_preview->SetLabelText(wxEmptyString);
		m_value_select->SetRange(0, 999);
		break;
	case ScriptTableEntryType::SET_FLAG:
		m_mode = EditorMode::VALUE;
		m_value_label->SetLabelText("Flag: ");
		m_value_preview->SetLabelText(wxEmptyString);
		m_value_select->SetRange(0, 999);
		break;
	case ScriptTableEntryType::SET_SPEAKER:
		m_mode = EditorMode::VALUE;
		m_value_label->SetLabelText("Character: ");
		m_value_preview->SetLabelText(wxEmptyString);
		m_value_select->SetRange(0, 999);
		break;
	case ScriptTableEntryType::SET_GLOBAL_SPEAKER:
		m_mode = EditorMode::VALUE;
		m_value_label->SetLabelText("Character: ");
		m_value_preview->SetLabelText(wxEmptyString);
		m_value_select->SetRange(0, 23);
		break;
	case ScriptTableEntryType::PLAY_CUTSCENE:
		m_mode = EditorMode::VALUE;
		m_value_label->SetLabelText("Cutscene: ");
		m_value_preview->SetLabelText(wxEmptyString);
		m_value_select->SetRange(0, 1023);
		break;
	case ScriptTableEntryType::INVALID:
		m_mode = EditorMode::VALUE;
		m_value_label->SetLabelText("Value: ");
		m_value_preview->SetLabelText(wxEmptyString);
		m_value_select->SetRange(0, 65535);
		break;
	case ScriptTableEntryType::PLAY_BGM:
		m_mode = EditorMode::BGM;
		break;
	case ScriptTableEntryType::GIVE_ITEM:
	case ScriptTableEntryType::GIVE_MONEY:
	default:
		m_mode = EditorMode::NONE;
		m_null_preview->SetLabelText(wxEmptyString);
		break;
	}
	for (const auto& panel : m_panels)
	{
		m_type_ctrl_sizer->Show(panel.second, panel.first == m_mode);
	}
	m_type_ctrl_sizer->Layout();
	Thaw();
	SetFocus();
}
