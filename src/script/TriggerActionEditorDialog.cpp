#include <script/TriggerActionEditorDialog.h>

#include <misc/MovableModalDialog.h>

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <landstalker/main/ScriptData.h>
#include <landstalker/script/AsmFunctionTable.h>
#include <landstalker/misc/Labels.h>
#include <script/CutsceneCodeEditor.h>

TriggerActionEditorDialog::TriggerActionEditorDialog(wxWindow* parent,
	std::shared_ptr<Landstalker::GameData> gd, int trigger_index)
	: wxDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(720, 560),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	m_gd(std::move(gd)),
	m_index(trigger_index)
{
	MakeModalDialogMovable(this);

	std::shared_ptr<Landstalker::AsmFunctionTable> table;
	if (m_gd && m_gd->GetScriptData())
	{
		table = m_gd->GetScriptData()->GetTriggerActions();
	}
	const Landstalker::AsmFunctionTable::Block* block = nullptr;
	if (table && table->IsValid() && m_index >= 0 && m_index < static_cast<int>(table->SlotCount()))
	{
		m_label = table->GetSlotLabel(static_cast<std::size_t>(m_index));
		block = table->FindBlock(m_label);
	}

	const auto name = Landstalker::Labels::Get(Landstalker::Labels::C_TRIGGER, m_index);
	const wxString generic = wxString::Format("TriggerAction%02X", static_cast<unsigned>(m_index));
	wxString title = wxString::Format("Trigger Action $%02X - ", static_cast<unsigned>(m_index));
	title += (name && !name->empty()) ? wxString(*name) : generic;
	SetTitle(title);

	auto* sizer = new wxBoxSizer(wxVERTICAL);
	// Editable name (C_TRIGGER): custom value shown, generic seed as the placeholder.
	auto* name_row = new wxBoxSizer(wxHORIZONTAL);
	name_row->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	m_name = new wxTextCtrl(this, wxID_ANY, (name && !name->empty()) ? wxString(*name) : wxString());
	m_name->SetHint(generic);
	name_row->Add(m_name, 1, wxALIGN_CENTER_VERTICAL);
	sizer->Add(name_row, 0, wxEXPAND | wxALL, 4);

	m_code = new CutsceneCodeEditor(this, wxID_ANY);
	m_code->SetGameData(m_gd);
	if (block)
	{
		m_code->LoadAsm(block->text, true);
	}
	else
	{
		m_code->ClearAsm(); // external / unavailable: nothing editable here
	}
	sizer->Add(m_code, 1, wxEXPAND | wxALL, 4);
	auto* buttons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
	sizer->Add(buttons, 0, wxEXPAND | wxALL, 4);
	SetSizer(sizer);

	Bind(wxEVT_BUTTON, &TriggerActionEditorDialog::OnOk, this, wxID_OK);
}

void TriggerActionEditorDialog::OnOk(wxCommandEvent& evt)
{
	std::shared_ptr<Landstalker::AsmFunctionTable> table;
	if (m_gd && m_gd->GetScriptData())
	{
		table = m_gd->GetScriptData()->GetTriggerActions();
	}
	if (table && !m_label.empty() && m_code && m_code->IsDirty())
	{
		table->SetBlockText(m_label, m_code->GetAsm());
	}
	// Persist a custom name (blank leaves the generic seed in place).
	if (m_name)
	{
		const wxString text = m_name->GetValue().Trim(true).Trim(false);
		const auto current = Landstalker::Labels::Get(Landstalker::Labels::C_TRIGGER, m_index);
		if (!text.empty() && (!current || *current != text.ToStdWstring()))
		{
			Landstalker::Labels::Update(Landstalker::Labels::C_TRIGGER, m_index, text.ToStdWstring());
		}
	}
	evt.Skip(); // let the default handler close the dialog with wxID_OK
}
