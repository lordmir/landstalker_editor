#include <user_interface/behaviours/include/BehaviourScriptEditorCtrl.h>

#include <wx/propgrid/advprops.h>

wxBEGIN_EVENT_TABLE(BehaviourScriptEditorCtrl, wxPanel)
EVT_CHOICE(wxID_ANY, BehaviourScriptEditorCtrl::OnScriptSelect)
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
    m_script_dropdown = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxArrayString());
    hsizer->Add(script_choice_label, 0, wxALL, 10);
    hsizer->Add(m_script_dropdown, 0, wxALL, 10);

    m_text_ctrl = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxTE_MULTILINE | wxTE_RICH2);
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
    UpdateUI();
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
    if (m_gd)
    {
        auto scripts = m_gd->GetSpriteData()->GetScriptNames();
        m_script_dropdown->Freeze();
        m_text_ctrl->Freeze();
        m_script_dropdown->Enable(true);
        m_text_ctrl->Enable(true);
        m_script_dropdown->Clear();
        m_text_ctrl->Clear();
        auto font = m_text_ctrl->GetFont();
        font.SetFamily(wxFONTFAMILY_TELETYPE);
        font.SetPointSize(10);
        m_text_ctrl->SetBackgroundColour(*wxWHITE);
        m_text_ctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
        for (const auto& script : scripts)
        {
            m_script_dropdown->Append(StrPrintf("[%d] %s", script.first, script.second.c_str()));
        }
        if (m_behaviour_script >= 0 && m_behaviour_script < static_cast<int>(m_script_dropdown->GetCount()))
        {
            m_text_ctrl->AppendText(Behaviours::ToYaml(m_gd->GetSpriteData()->GetScript(m_behaviour_script).second));
            m_script_dropdown->SetSelection(m_behaviour_script);
        }
        else
        {
            m_text_ctrl->Clear();
            m_script_dropdown->SetSelection(wxNOT_FOUND);
        }
        m_script_dropdown->Thaw();
        m_text_ctrl->Thaw();
    }
    else
    {
        m_text_ctrl->Clear();
        m_text_ctrl->Enable(false);
        m_script_dropdown->Clear();
        m_behaviour_script = -1;
        m_script_dropdown->SetSelection(wxNOT_FOUND);
        m_script_dropdown->Enable(false);
    }
}

void BehaviourScriptEditorCtrl::OnScriptSelect(wxCommandEvent& /*evt*/ )
{
    m_behaviour_script = m_script_dropdown->GetSelection();
    UpdateUI();
}
