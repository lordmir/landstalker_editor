#include <PreferencesDialog.h>
#include <wx/config.h>
#include <wxcrafter.h>
#include <AssemblyBuilderDialog.h>

PreferencesDialog::PreferencesDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Preferences", wxDefaultPosition, wxSize(600, 500))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    wxGridSizer* gsizer = new wxGridSizer(2);
    sizer->Add(gsizer, 1, wxEXPAND, 5);

    m_ctrl_clone_in_new_dir = new wxCheckBox(this, wxID_ANY, "Clone When Saving Assembly to Empty Directory");
    m_ctrl_clonecmd = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_cloneurl = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_clonetag = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_build_on_save = new wxCheckBox(this, wxID_ANY, "Build After Saving Assembly");
    m_ctrl_assembler = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_asmargs = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_baseasm = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_outname = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_run_after_build = new wxCheckBox(this, wxID_ANY, "Run Emulator Following Build");
    m_ctrl_emulator = new wxTextCtrl(this, wxID_ANY);



    gsizer->Add(m_ctrl_clone_in_new_dir, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, wxEmptyString), 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Clone Command:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_clonecmd, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Clone URL:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_cloneurl, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Clone Tag:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_clonetag, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_build_on_save, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, wxEmptyString), 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Assembler Location:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_assembler, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Assembler Flags:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_asmargs, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Base Assembly File:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_baseasm, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Output Binary Filename:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_outname, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_run_after_build, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, wxEmptyString), 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(new wxStaticText(this, wxID_ANY, "Emulator Command:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gsizer->Add(m_ctrl_emulator, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);

    wxStdDialogButtonSizer* btnszr = new wxStdDialogButtonSizer();
    m_ok = new wxButton(this, wxID_OK, "OK");
    m_cancel = new wxButton(this, wxID_CANCEL, "Cancel");

    sizer->Add(btnszr, 0, wxALL, 5);
    m_ok->SetDefault();
    btnszr->AddButton(m_ok);
    btnszr->AddButton(m_cancel);
    btnszr->Realize();

    this->SetSizer(sizer);
    this->Layout();

    m_ok->Bind(wxEVT_BUTTON, &PreferencesDialog::OnOK, this);
    m_cancel->Bind(wxEVT_BUTTON, &PreferencesDialog::OnCancel, this);

    Init();
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::Init()
{
    AssemblyBuilderDialog::InitConfig(APPLICATION_NAME);
    auto config = new wxConfig(APPLICATION_NAME);
    if (config != nullptr)
    {
        m_ctrl_clone_in_new_dir->SetValue(config->ReadBool("/build/clone_in_new_dir", true));
        m_ctrl_clonecmd->SetValue(config->Read("/build/clonecmd"));
        m_ctrl_clonetag->SetValue(config->Read("/build/clonetag"));
        m_ctrl_cloneurl->SetValue(config->Read("/build/cloneurl"));
        m_ctrl_build_on_save->SetValue(config->ReadBool("/build/build_on_save", true));
        m_ctrl_assembler->SetValue(config->Read("/build/assembler"));
        m_ctrl_asmargs->SetValue(config->Read("/build/asmargs"));
        m_ctrl_baseasm->SetValue(config->Read("/build/baseasm"));
        m_ctrl_outname->SetValue(config->Read("/build/outname"));
        m_ctrl_run_after_build->SetValue(config->ReadBool("/build/run_after_build", true));
        m_ctrl_emulator->SetValue(config->Read("/build/emulator"));
        delete config;
    }
}

void PreferencesDialog::Commit()
{
    auto config = new wxConfig(APPLICATION_NAME);
    if (config != nullptr)
    {
        config->Write("/build/clone_in_new_dir", m_ctrl_clone_in_new_dir->GetValue());
        config->Write("/build/clonecmd", m_ctrl_clonecmd->GetValue());
        config->Write("/build/clonetag", m_ctrl_clonetag->GetValue());
        config->Write("/build/cloneurl", m_ctrl_cloneurl->GetValue());
        config->Write("/build/build_on_save", m_ctrl_build_on_save->GetValue());
        config->Write("/build/assembler", m_ctrl_assembler->GetValue());
        config->Write("/build/asmargs", m_ctrl_asmargs->GetValue());
        config->Write("/build/baseasm", m_ctrl_baseasm->GetValue());
        config->Write("/build/outname", m_ctrl_outname->GetValue());
        config->Write("/build/run_after_build", m_ctrl_run_after_build->GetValue());
        config->Write("/build/emulator", m_ctrl_emulator->GetValue());

        delete config;
        AssemblyBuilderDialog::InitConfig(APPLICATION_NAME);
    }
}

void PreferencesDialog::OnOK(wxCommandEvent& evt)
{
    Commit();
    EndModal(wxID_OK);
}

void PreferencesDialog::OnCancel(wxCommandEvent& evt)
{
    EndModal(wxID_CANCEL);
}
