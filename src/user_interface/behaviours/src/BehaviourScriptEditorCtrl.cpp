#include <user_interface/behaviours/include/BehaviourScriptEditorCtrl.h>
#include <landstalker/behaviours/include/BehaviourYamlConverter.h>

#include <wx/propgrid/advprops.h>

wxBEGIN_EVENT_TABLE(BehaviourScriptEditorCtrl, wxPanel)
EVT_CHOICE(wxID_ANY, BehaviourScriptEditorCtrl::OnScriptSelect)
EVT_BUTTON(wxID_SAVE, BehaviourScriptEditorCtrl::OnSaveClick)
EVT_BUTTON(wxID_RESET, BehaviourScriptEditorCtrl::OnResetClick)
EVT_TEXT(wxID_ANY, BehaviourScriptEditorCtrl::OnScriptChange)
EVT_CLOSE(BehaviourScriptEditorCtrl::OnClose)
wxEND_EVENT_TABLE()

BehaviourScriptEditorCtrl::BehaviourScriptEditorCtrl(wxWindow* parent)
	: wxPanel(parent),
	  m_behaviour_script(-1),
      dirty(false)
{
    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(vsizer);

    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    vsizer->Add(hsizer, 0, wxALL, 0);

    wxStaticText* script_choice_label = new wxStaticText(this, wxID_ANY, _("Script:"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1, -1)), 0);
    m_script_dropdown = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxArrayString());
    m_save_btn = new wxButton(this, wxID_SAVE, "Save", wxDefaultPosition, wxDefaultSize);
    m_reset_btn = new wxButton(this, wxID_RESET, "Reset", wxDefaultPosition, wxDefaultSize);
    hsizer->Add(script_choice_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    hsizer->Add(m_script_dropdown, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    hsizer->Add(m_save_btn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    hsizer->Add(m_reset_btn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);

    m_text_ctrl = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxTE_MULTILINE | wxTE_RICH2);
    vsizer->Add(m_text_ctrl, 1, wxALL | wxEXPAND, 5);

    GetSizer()->Fit(this);
    RefreshBehaviourScript();
}

BehaviourScriptEditorCtrl::~BehaviourScriptEditorCtrl()
{
}

void BehaviourScriptEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
    RefreshBehaviourScript();
}

void BehaviourScriptEditorCtrl::ClearGameData()
{
	m_gd = nullptr;
	m_behaviour_script = -1;
    RefreshBehaviourScript();
}

void BehaviourScriptEditorCtrl::Open(int id)
{
	m_behaviour_script = id;
    RefreshBehaviourScript();
}

int BehaviourScriptEditorCtrl::GetOpenScriptId() const
{
	return m_behaviour_script;
}

void BehaviourScriptEditorCtrl::RefreshBehaviourScript()
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
            m_script_dropdown->Append(StrWPrintf("[%d] %s", script.first, m_gd->GetSpriteData()->GetBehaviourDisplayName(script.first).c_str()));
        }
        if (m_behaviour_script >= 0 && m_behaviour_script < static_cast<int>(m_script_dropdown->GetCount()))
        {
            orig = utf8_to_wstr(BehaviourYamlConverter::ToYaml(m_gd->GetSpriteData()->GetScript(m_behaviour_script).second));
            m_text_ctrl->AppendText(orig);
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
    dirty = false;
    UpdateUI();
}

void BehaviourScriptEditorCtrl::UpdateUI()
{
    bool changed = !(m_text_ctrl->GetValue() == orig);
    if (dirty == false && changed == true)
    {
        dirty = true;
        m_save_btn->Enable(true);
        m_save_btn->SetLabel("Save");
    }
    else if (dirty == true && changed == false)
    {
        dirty = false;
        m_save_btn->Enable(false);
        m_save_btn->SetLabel("Saved");
    }
}

bool BehaviourScriptEditorCtrl::Commit()
{
    try
    {
        auto result = BehaviourYamlConverter::FromYaml(wstr_to_utf8(m_text_ctrl->GetValue().ToStdWstring()));
        if (m_gd && m_behaviour_script >= 0)
        {
            m_gd->GetSpriteData()->SetScript(m_behaviour_script, result);
        }
        dirty = false;
        UpdateUI();
    }
    catch (const std::exception& e)
    {
        dirty = true;
        m_save_btn->SetLabel("Save");
        m_save_btn->Enable(true);
        wxMessageBox(e.what(), "Error parsing YAML", wxOK | wxICON_ERROR | wxCENTRE);
        return false;
    }
    return true;
}

void BehaviourScriptEditorCtrl::OnScriptSelect(wxCommandEvent& /*evt*/)
{
    if (Commit())
    {
        m_behaviour_script = m_script_dropdown->GetSelection();
        RefreshBehaviourScript();
    }
    else
    {
        m_script_dropdown->SetSelection(m_behaviour_script);
        dirty = true;
        UpdateUI();
    }
}

void BehaviourScriptEditorCtrl::OnSaveClick(wxCommandEvent& evt)
{
    if (Commit())
    {
        RefreshBehaviourScript();
    }
    else
    {
        dirty = true;
        UpdateUI();
    }
    evt.Skip();
}

void BehaviourScriptEditorCtrl::OnResetClick(wxCommandEvent& evt)
{
    RefreshBehaviourScript();
    dirty = false;
    UpdateUI();
    evt.Skip();
}

void BehaviourScriptEditorCtrl::OnScriptChange(wxCommandEvent& evt)
{
    UpdateUI();
    evt.Skip();
}

void BehaviourScriptEditorCtrl::OnClose(wxCloseEvent& evt)
{
    if (dirty)
    {
        if (!Commit())
        {
            evt.Veto();
        }
    }
    evt.Skip();
}
