#include <misc/IntroStringDialog.h>
#include <misc/MovableModalDialog.h>

#include <wx/sizer.h>

#include <text/StringEditorFrame.h>

IntroStringDialog::IntroStringDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int index)
	: wxDialog(parent, wxID_ANY, wxString::Format("Intro String %d", index),
		wxDefaultPosition, wxSize(720, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	MakeModalDialogMovable(this);

	auto* sizer = new wxBoxSizer(wxVERTICAL);
	// The string editor ignores its ImageList, so a null one is fine for the embedded instance.
	m_editor = new StringEditorFrame(this, nullptr);
	m_editor->SetGameData(gd);
	m_editor->GoToIntroString(index);
	sizer->Add(m_editor, 1, wxEXPAND | wxALL, 4);
	sizer->Add(CreateStdDialogButtonSizer(wxOK), 0, wxEXPAND | wxALL, 4);
	SetSizer(sizer);
}
