#include <user_interface/behaviours/include/BehaviourScriptEditorCtrl.h>

#include <wx/propgrid/advprops.h>

wxBEGIN_EVENT_TABLE(BehaviourScriptEditorCtrl, wxPanel)
wxEND_EVENT_TABLE()

BehaviourScriptEditorCtrl::BehaviourScriptEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_behaviour_script(-1)
{
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vsizer);

    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    vsizer->Add(hsizer, 0, wxALL, 0);

    wxStaticText* script_choice_label = new wxStaticText(this, wxID_ANY, _("Script:"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_script_dropdown = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), m_scripts, 0);
    hsizer->Add(script_choice_label, 0, wxALL, 10);
    hsizer->Add(m_script_dropdown, 0, wxALL, 10);

    m_text_ctrl = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxTE_MULTILINE | wxTE_RICH2);
    auto font = m_text_ctrl->GetFont();
    font.SetFamily(wxFONTFAMILY_TELETYPE);
    font.SetPointSize(12);
    m_text_ctrl->SetBackgroundColour(*wxWHITE);
    m_text_ctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
    vsizer->Add(m_text_ctrl, 1, wxALL | wxEXPAND, 5);

    GetSizer()->Fit(this);
}

BehaviourScriptEditorCtrl::~BehaviourScriptEditorCtrl()
{
}

void BehaviourScriptEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	UpdateUI();
}

void BehaviourScriptEditorCtrl::ClearGameData()
{
	m_gd = nullptr;
	m_behaviour_script = -1;
}

void BehaviourScriptEditorCtrl::Open(int id)
{
	m_behaviour_script = id;
	UpdateUI();
}

int BehaviourScriptEditorCtrl::GetOpenScriptId() const
{
	return m_behaviour_script;
}

void BehaviourScriptEditorCtrl::UpdateUI()
{
}
