#include <script/DataViewScriptActionEditorControl.h>


DataViewScriptActionEditorControl::DataViewScriptActionEditorControl(wxWindow* parent, const wxRect& rect,
	const Landstalker::ScriptTable::Action& value, const std::vector<std::string>& choices, std::shared_ptr<Landstalker::GameData> gd)
	: wxWindow(parent, wxID_ANY, rect.GetPosition(), rect.GetSize()),
	  m_entry(value),
	  m_choices(),
	  m_editor(Editor::NONE),
	  m_gd(gd)
{
	Freeze();
	for (const auto& choice : choices)
	{
		m_choices.Add(choice);
	}
	SetBackgroundColour(*wxWHITE);
	SetBackgroundStyle(wxBackgroundStyle::wxBG_STYLE_SYSTEM);
	m_spin_ctrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, { 150, rect.GetHeight() }, wxSP_ARROW_KEYS | wxWANTS_CHARS, 0, 32767);
	m_combo_ctrl = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, { 150, rect.GetHeight() }, m_choices, wxWANTS_CHARS);
	m_combo_ctrl->AutoComplete(m_choices);
	m_comment = new wxStaticText(this, wxID_ANY, wxEmptyString, { 154, 0 }, { std::min(10, rect.GetWidth() - 154), rect.GetHeight() });
	Thaw();
	SetValue(value);

	m_combo_ctrl->Bind(wxEVT_TEXT_ENTER, &DataViewScriptActionEditorControl::OnTextEnter, this);
	m_combo_ctrl->Bind(wxEVT_COMBOBOX, &DataViewScriptActionEditorControl::OnTextEnter, this);
	m_combo_ctrl->Bind(wxEVT_CHAR_HOOK, &DataViewScriptActionEditorControl::OnKeyDown, this);
	m_combo_ctrl->Bind(wxEVT_TEXT, &DataViewScriptActionEditorControl::OnChange, this);
	m_spin_ctrl->Bind(wxEVT_TEXT_ENTER, &DataViewScriptActionEditorControl::OnTextEnter, this);
	m_spin_ctrl->Bind(wxEVT_CHAR_HOOK, &DataViewScriptActionEditorControl::OnKeyDown, this);
	m_spin_ctrl->Bind(wxEVT_SPIN, &DataViewScriptActionEditorControl::OnTextEnter, this);
	m_spin_ctrl->Bind(wxEVT_TEXT, &DataViewScriptActionEditorControl::OnChange, this);
}

DataViewScriptActionEditorControl::~DataViewScriptActionEditorControl()
{
	m_combo_ctrl->Unbind(wxEVT_TEXT_ENTER, &DataViewScriptActionEditorControl::OnTextEnter, this);
	m_combo_ctrl->Unbind(wxEVT_COMBOBOX, &DataViewScriptActionEditorControl::OnTextEnter, this);
	m_combo_ctrl->Unbind(wxEVT_CHAR_HOOK, &DataViewScriptActionEditorControl::OnKeyDown, this);
	m_combo_ctrl->Unbind(wxEVT_TEXT, &DataViewScriptActionEditorControl::OnChange, this);
	m_spin_ctrl->Unbind(wxEVT_TEXT_ENTER, &DataViewScriptActionEditorControl::OnTextEnter, this);
	m_spin_ctrl->Unbind(wxEVT_CHAR_HOOK, &DataViewScriptActionEditorControl::OnKeyDown, this);
	m_spin_ctrl->Unbind(wxEVT_SPIN, &DataViewScriptActionEditorControl::OnSpin, this);
	m_spin_ctrl->Unbind(wxEVT_TEXT, &DataViewScriptActionEditorControl::OnChange, this);
}

void DataViewScriptActionEditorControl::SetValue(const Landstalker::ScriptTable::Action& value)
{
	Freeze();
	m_entry = value;
	m_editor = Editor::NONE;
	m_spin_ctrl->Hide();
	m_combo_ctrl->Hide();
	std::visit([this](const auto& arg)
	{
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, uint16_t>)
		{
			m_editor = Editor::SPIN_CTRL;
			m_spin_ctrl->SetValue(arg);
			m_spin_ctrl->Show();
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			m_editor = Editor::COMBO_CTRL;
			m_combo_ctrl->SetValue(arg);
			m_combo_ctrl->Show();
		}
	}, m_entry);

	m_comment->SetLabelText(Landstalker::ScriptTable::GetActionSummary(m_entry, m_gd));
	SetFocus();
	Thaw();
}

Landstalker::ScriptTable::Action DataViewScriptActionEditorControl::GetValue() const
{
	UpdateControls();
	return m_entry;
}

void DataViewScriptActionEditorControl::OnKeyDown(wxKeyEvent& evt)
{
	if (evt.GetKeyCode() == WXK_RETURN)
	{
		UpdateControls();
		evt.Skip();
	}
	else
	{
		evt.Skip();
	}
}

void DataViewScriptActionEditorControl::OnChange(wxCommandEvent& evt)
{
	UpdateControls();
	evt.Skip();
}

void DataViewScriptActionEditorControl::OnSpin(wxSpinEvent& evt)
{
	UpdateControls();
	evt.Skip();
}

void DataViewScriptActionEditorControl::OnTextEnter(wxCommandEvent& evt)
{
	UpdateControls();
	evt.Skip();
}

void DataViewScriptActionEditorControl::UpdateControls() const
{
	wxString text;
	if (m_editor == Editor::COMBO_CTRL)
	{
		text = m_combo_ctrl->GetValue();
	}
	else
	{
		text = m_spin_ctrl->GetTextValue();
	}

	if (text.Strip().IsEmpty())
	{
		return;
	}

	m_entry = Landstalker::ScriptTable::FromString(text.ToStdString());

	m_comment->SetLabelText(Landstalker::ScriptTable::GetActionSummary(m_entry, m_gd));
}

void DataViewScriptActionEditorControl::SetFocus()
{
	if (m_editor == Editor::COMBO_CTRL)
	{
		m_combo_ctrl->SetFocus();
		m_combo_ctrl->SelectAll();
	}
	else if (m_editor == Editor::SPIN_CTRL)
	{
		m_spin_ctrl->SetFocus();
		m_spin_ctrl->SetSelection(-1, -1);
	}
}
