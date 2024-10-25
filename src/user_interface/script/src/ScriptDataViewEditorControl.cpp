#include <user_interface/script/include/ScriptDataViewEditorControl.h>

wxBEGIN_EVENT_TABLE(ScriptDataViewEditorControl, wxWindow)
EVT_KEY_DOWN(ScriptDataViewEditorControl::OnKeyDown)
EVT_LEFT_UP(ScriptDataViewEditorControl::OnClick)
EVT_CHECKBOX(wxID_CLEAR, ScriptDataViewEditorControl::OnClearCheck)
EVT_CHECKBOX(wxID_STOP, ScriptDataViewEditorControl::OnEndCheck)
EVT_SPINCTRL(wxID_ANY, ScriptDataViewEditorControl::OnSpin)
EVT_TEXT_ENTER(wxID_ANY, ScriptDataViewEditorControl::OnTextEnter)
wxEND_EVENT_TABLE()

ScriptDataViewEditorControl::ScriptDataViewEditorControl(wxWindow* parent, const wxRect& rect, uint16_t value, std::shared_ptr<const GameData> gd)
  : wxWindow(parent, wxID_ANY, rect.GetPosition(), rect.GetSize()),
	m_gd(gd)
{
	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	m_clear = new wxCheckBox(this, wxID_CLEAR, "Clear:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	m_end = new wxCheckBox(this, wxID_STOP, "End:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	m_string_select = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxWANTS_CHARS,
		m_gd->GetScriptData()->GetStringStart(), m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN) - 1);
	m_string_preview = new wxStaticText(this, wxID_PREVIEW, "");
	m_string_preview->SetFont(m_string_preview->GetFont().Italic());
	hsizer->Add(m_clear, wxLEFT);
	hsizer->Add(m_end, wxLEFT);
	hsizer->AddSpacer(10);
	hsizer->Add(new wxStaticText(this, wxID_ANY, "String:"), wxLEFT);
	hsizer->Add(m_string_select, wxLEFT);
	hsizer->AddSpacer(10);
	hsizer->Add(m_string_preview, wxEXPAND);
	this->SetSizer(hsizer);
	GetSizer()->Fit(this);
	SetValue(value);

	m_string_select->Bind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
	m_string_select->Bind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
}

ScriptDataViewEditorControl::~ScriptDataViewEditorControl()
{
	m_string_select->Unbind(wxEVT_CHAR_HOOK, &ScriptDataViewEditorControl::OnKeyDown, this);
	m_string_select->Unbind(wxEVT_TEXT_ENTER, &ScriptDataViewEditorControl::OnTextEnter, this);
}

void ScriptDataViewEditorControl::SetValue(uint16_t value)
{
	m_entry = std::move(ScriptTableEntry::FromBytes(value));
	switch (m_entry->GetType())
	{
	case ScriptTableEntryType::STRING:
		{
			const auto& string_entry = dynamic_cast<const ScriptStringEntry&>(*m_entry);
			std::size_t string_idx = string_entry.string + m_gd->GetScriptData()->GetStringStart();
			m_clear->SetValue(string_entry.clear_box);
			m_end->SetValue(string_entry.end);
			m_string_select->SetValue(string_idx);
			m_string_preview->SetLabelText(m_gd->GetStringData()->GetString(StringData::Type::MAIN, string_idx));
		}
		break;
	}
}

uint16_t ScriptDataViewEditorControl::GetValue() const
{
	if (m_entry)
	{
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
	else if (evt.GetKeyCode() == 'c' || evt.GetKeyCode() == 'C')
	{
		m_clear->SetValue(!m_clear->GetValue());
		Update();
	}
	else if (evt.GetKeyCode() == 'e' || evt.GetKeyCode() == 'E')
	{
		m_end->SetValue(!m_end->GetValue());
		Update();
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

void ScriptDataViewEditorControl::Render()
{
	for (const auto& win : m_controls)
	{
		win->Destroy();
	}
	m_controls.clear();
	if (!m_entry || !m_gd)
	{
		return;
	}
	switch (m_entry->GetType())
	{
	case ScriptTableEntryType::STRING:
		m_controls.push_back(new wxCheckBox(this, wxID_CLEAR, "Clear:"));
		m_controls.push_back(new wxCheckBox(this, wxID_EXIT, "End:"));
		m_controls.push_back(new wxStaticText(this, wxID_ANY, "Message:"));
		m_controls.push_back(new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 16384, m_gd->GetScriptData()->GetStringStart(),
			m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN) - m_gd->GetScriptData()->GetStringStart(),
			m_gd->GetScriptData()->GetStringStart()));
		m_controls.push_back(new wxStaticText(this, wxID_ANY, m_gd->GetStringData()->GetString(StringData::Type::MAIN, m_gd->GetScriptData()->GetStringStart())));
		break;
	default:
		m_controls.push_back(new wxStaticText(this, wxID_ANY, m_entry->ToString(m_gd)));
		break;
	}
}

void ScriptDataViewEditorControl::Update()
{
	switch (m_entry->GetType())
	{
	case ScriptTableEntryType::STRING:
	{
		auto& string_entry = dynamic_cast<ScriptStringEntry&>(*m_entry);
		std::size_t string_idx = m_string_select->GetValue() - m_gd->GetScriptData()->GetStringStart();
		string_entry.end = m_end->GetValue();
		string_entry.clear_box = m_clear->GetValue();
		if (string_idx < m_gd->GetStringData()->GetStringCount(StringData::Type::MAIN))
		{
			string_entry.string = string_idx;
		}
		SetValue(m_entry->ToBytes());
	}
	break;
	}
}

void ScriptDataViewEditorControl::SetFocus()
{
	m_string_select->SetFocus();
	m_string_select->SetSelection(0, m_string_select->GetTextValue().Length());
}
