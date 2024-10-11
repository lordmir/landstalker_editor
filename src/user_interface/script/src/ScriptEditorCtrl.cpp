#include <user_interface\script\include\ScriptEditorCtrl.h>

wxBEGIN_EVENT_TABLE(ScriptEditorCtrl, wxPanel)
wxEND_EVENT_TABLE()

ScriptEditorCtrl::ScriptEditorCtrl(wxWindow* parent)
	: wxPanel(parent)
{
	wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(hsizer);

	m_text_ctrl = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), wxTE_MULTILINE | wxTE_RICH2);
	hsizer->Add(m_text_ctrl, 1, wxALL | wxEXPAND, 5);

	GetSizer()->Fit(this);
}

ScriptEditorCtrl::~ScriptEditorCtrl()
{
}

void ScriptEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
}

void ScriptEditorCtrl::ClearGameData()
{
	m_gd.reset();
}

void ScriptEditorCtrl::Open()
{
	if (m_gd)
	{
		m_text_ctrl->Clear();
		auto font = m_text_ctrl->GetFont();
		font.SetFamily(wxFONTFAMILY_TELETYPE);
		font.SetPointSize(10);
		m_text_ctrl->SetBackgroundColour(*wxWHITE);
		m_text_ctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
		m_text_ctrl->AppendText(_(m_gd->GetScriptData()->GetScript().GetAllScriptStrings(m_gd)));
	}
}

void ScriptEditorCtrl::UpdateUI()
{
	if (m_gd)
	{
		m_text_ctrl->Clear();
		auto font = m_text_ctrl->GetFont();
		font.SetFamily(wxFONTFAMILY_TELETYPE);
		font.SetPointSize(10);
		m_text_ctrl->SetBackgroundColour(*wxWHITE);
		m_text_ctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
		m_text_ctrl->AppendText(_(m_gd->GetScriptData()->GetScript().GetAllScriptStrings(m_gd)));
	}
}
