#include <misc/InputScriptDialog.h>
#include <misc/MovableModalDialog.h>

#include <wx/sizer.h>

#include <misc/InputTableFrame.h>

InputScriptDialog::InputScriptDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int sequence)
	: wxDialog(parent, wxID_ANY, wxString::Format("Input Script $%03X", static_cast<unsigned>(sequence)),
		wxDefaultPosition, wxSize(640, 520), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	MakeModalDialogMovable(this);

	auto* sizer = new wxBoxSizer(wxVERTICAL);
	// The input editor ignores its ImageList, so a null one is fine for the embedded instance.
	m_editor = new InputTableFrame(this, nullptr);
	m_editor->SetGameData(gd);
	m_editor->ShowSequenceListPane(false); // focused single-sequence view, like the other popups
	m_editor->GoToSequence(sequence);
	sizer->Add(m_editor, 1, wxEXPAND | wxALL, 4);
	sizer->Add(CreateStdDialogButtonSizer(wxOK), 0, wxEXPAND | wxALL, 4);
	SetSizer(sizer);

	Bind(wxEVT_BUTTON, &InputScriptDialog::OnOk, this, wxID_OK);
}

void InputScriptDialog::OnOk(wxCommandEvent& evt)
{
	if (m_editor)
	{
		m_editor->CommitPendingEdits(); // flush a duration typed but not yet blurred
	}
	evt.Skip(); // let the default handler close with wxID_OK
}
