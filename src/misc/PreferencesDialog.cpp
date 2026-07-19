#include <misc/PreferencesDialog.h>
#include <wx/config.h>
#include <wx/statline.h>
#include <wxresource/wxcrafter.h>
#include <misc/AssemblyBuilderDialog.h>

BuildOptionsDialog::BuildOptionsDialog(wxWindow* parent, wxConfig* config)
    : wxDialog(parent, wxID_ANY, "Build Options", wxDefaultPosition, wxSize(620, 720)),
      m_config(config)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    auto add_row = [](wxFlexGridSizer* gsizer, const wxString& label, wxWindow* ctrl)
    {
        gsizer->Add(new wxStaticText(ctrl->GetParent(), wxID_ANY, label), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
        gsizer->Add(ctrl, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    };
    auto add_check_row = [](wxFlexGridSizer* gsizer, wxCheckBox* ctrl)
    {
        gsizer->Add(ctrl, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
        gsizer->Add(new wxStaticText(ctrl->GetParent(), wxID_ANY, wxEmptyString), 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
    };
    auto make_group = [this, sizer](const wxString& label) -> wxFlexGridSizer*
    {
        wxStaticBoxSizer* group = new wxStaticBoxSizer(wxVERTICAL, this, label);
        wxFlexGridSizer* gsizer = new wxFlexGridSizer(2, 0, 0);
        gsizer->AddGrowableCol(1);
        group->Add(gsizer, 1, wxEXPAND, 5);
        sizer->Add(group, 0, wxEXPAND | wxALL, 5);
        return gsizer;
    };

    wxFlexGridSizer* repo_sizer = make_group("Assembly Repository");
    m_ctrl_clone_in_new_dir = new wxCheckBox(this, wxID_ANY, "Clone When Saving Assembly to Empty Directory");
    m_ctrl_clonecmd = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_cloneurl = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_clonetag = new wxTextCtrl(this, wxID_ANY);
    add_check_row(repo_sizer, m_ctrl_clone_in_new_dir);
    add_row(repo_sizer, "Clone Command:", m_ctrl_clonecmd);
    add_row(repo_sizer, "Clone URL:", m_ctrl_cloneurl);
    add_row(repo_sizer, "Clone Tag/Branch:", m_ctrl_clonetag);

    wxFlexGridSizer* build_sizer = make_group("ROM Build");
    m_ctrl_build_on_save = new wxCheckBox(this, wxID_ANY, "Build After Saving Assembly");
    m_ctrl_assembler = new wxTextCtrl(this, wxID_ANY);
    add_check_row(build_sizer, m_ctrl_build_on_save);
    add_row(build_sizer, "Assembler Location:", m_ctrl_assembler);

    wxFlexGridSizer* z80_sizer = make_group("Sound Driver (Z80)");
    m_ctrl_z80assembler = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_z80linker = new wxTextCtrl(this, wxID_ANY);
    add_row(z80_sizer, "Z80 Assembler Location:", m_ctrl_z80assembler);
    add_row(z80_sizer, "Z80 Linker Location:", m_ctrl_z80linker);

    // The build region and expanded state come from the opened assembly: the
    // controls here only select which build variant's stored settings are
    // being edited.
    wxFlexGridSizer* region_sizer = make_group("Region Build Settings");
    m_ctrl_region = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, AssemblyBuilderDialog::GetRegions());
    m_ctrl_expanded = new wxCheckBox(this, wxID_ANY, "Expanded ROM");
    m_ctrl_asmargs = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_asmargs->SetToolTip("Pre-populated with the options from the assembly's build.yaml");
    m_ctrl_defines = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_defines->SetToolTip("NAME=VALUE pairs separated by semicolons, pre-populated from the assembly's build.yaml");
    m_ctrl_outname = new wxTextCtrl(this, wxID_ANY);
    m_ctrl_outname->SetToolTip("Leave blank to use the build's default ROM filename");
    add_row(region_sizer, "Settings for Region:", m_ctrl_region);
    add_check_row(region_sizer, m_ctrl_expanded);
    add_row(region_sizer, "Assembler Flags Override:", m_ctrl_asmargs);
    add_row(region_sizer, "Preprocessor Defines:", m_ctrl_defines);
    add_row(region_sizer, "Output ROM Filename Override:", m_ctrl_outname);

    wxFlexGridSizer* emu_sizer = make_group("Emulator");
    m_ctrl_run_after_build = new wxCheckBox(this, wxID_ANY, "Run Emulator Following Build");
    m_ctrl_emulator = new wxTextCtrl(this, wxID_ANY);
    add_check_row(emu_sizer, m_ctrl_run_after_build);
    add_row(emu_sizer, "Emulator Command:", m_ctrl_emulator);

    wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxStdDialogButtonSizer* btnszr = new wxStdDialogButtonSizer();
    m_reset = new wxButton(this, wxID_ANY, "Reset to Defaults");
    m_ok = new wxButton(this, wxID_OK, "OK");
    m_cancel = new wxButton(this, wxID_CANCEL, "Cancel");

    sizer->AddStretchSpacer();
    btn_sizer->Add(m_reset, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    btn_sizer->AddStretchSpacer();
    btn_sizer->Add(btnszr, 0, 0, 0);
    sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
    m_ok->SetDefault();
    btnszr->AddButton(m_ok);
    btnszr->AddButton(m_cancel);
    btnszr->Realize();

    this->SetSizer(sizer);
    this->Layout();

    m_ctrl_region->Bind(wxEVT_CHOICE, &BuildOptionsDialog::OnVariantChange, this);
    m_ctrl_expanded->Bind(wxEVT_CHECKBOX, &BuildOptionsDialog::OnVariantChange, this);
    m_reset->Bind(wxEVT_BUTTON, &BuildOptionsDialog::OnResetDefaults, this);
    m_ok->Bind(wxEVT_BUTTON, &BuildOptionsDialog::OnOK, this);
    m_cancel->Bind(wxEVT_BUTTON, &BuildOptionsDialog::OnCancel, this);

    Init();
}

BuildOptionsDialog::~BuildOptionsDialog()
{
}

void BuildOptionsDialog::Init()
{
    if (m_config != nullptr)
    {
        m_ctrl_clone_in_new_dir->SetValue(m_config->ReadBool("/build/clone_in_new_dir", true));
        m_ctrl_clonecmd->SetValue(m_config->Read("/build/clonecmd"));
        m_ctrl_clonetag->SetValue(m_config->Read("/build/clonetag"));
        m_ctrl_cloneurl->SetValue(m_config->Read("/build/cloneurl"));
        m_ctrl_build_on_save->SetValue(m_config->ReadBool("/build/build_on_save", true));
        m_ctrl_assembler->SetValue(m_config->Read("/build/assembler"));
        m_ctrl_z80assembler->SetValue(m_config->Read("/build/z80assembler"));
        m_ctrl_z80linker->SetValue(m_config->Read("/build/z80linker"));
        m_ctrl_run_after_build->SetValue(m_config->ReadBool("/build/run_after_build", true));
        m_ctrl_emulator->SetValue(m_config->Read("/build/emulator"));
    }
    // Start on the opened project's build variant.
    ShowVariantSettings(AssemblyBuilderDialog::GetProjectRegion(), AssemblyBuilderDialog::GetProjectExpanded());
}

void BuildOptionsDialog::Commit()
{
    if (m_config != nullptr)
    {
        StoreDisplayedVariantSettings();
        m_config->Write("/build/clone_in_new_dir", m_ctrl_clone_in_new_dir->GetValue());
        m_config->Write("/build/clonecmd", m_ctrl_clonecmd->GetValue());
        m_config->Write("/build/clonetag", m_ctrl_clonetag->GetValue());
        m_config->Write("/build/cloneurl", m_ctrl_cloneurl->GetValue());
        m_config->Write("/build/build_on_save", m_ctrl_build_on_save->GetValue());
        m_config->Write("/build/assembler", m_ctrl_assembler->GetValue());
        m_config->Write("/build/z80assembler", m_ctrl_z80assembler->GetValue());
        m_config->Write("/build/z80linker", m_ctrl_z80linker->GetValue());
        m_config->Write("/build/run_after_build", m_ctrl_run_after_build->GetValue());
        m_config->Write("/build/emulator", m_ctrl_emulator->GetValue());
        for (const auto& variant : m_variant_settings)
        {
            // Flags and defines matching the build.yaml defaults are stored
            // blank so the build keeps tracking the assembly's own options.
            wxString asmargs = variant.second.asmargs;
            if (asmargs == AssemblyBuilderDialog::GetDefaultBuildOpts(variant.first))
            {
                asmargs.clear();
            }
            wxString defines = variant.second.defines;
            if (defines == AssemblyBuilderDialog::GetDefaultDefines(variant.first))
            {
                defines.clear();
            }
            m_config->Write("/build/" + variant.first + "/asmargs", asmargs);
            m_config->Write("/build/" + variant.first + "/defines", defines);
            m_config->Write("/build/" + variant.first + "/outname", variant.second.outname);
        }
        m_config->Flush();
        AssemblyBuilderDialog::InitConfig(m_config);
    }
}

wxString BuildOptionsDialog::SelectedVariant() const
{
    return AssemblyBuilderDialog::BuildName(m_ctrl_region->GetStringSelection(), m_ctrl_expanded->GetValue());
}

BuildOptionsDialog::VariantSettings BuildOptionsDialog::LoadVariantSettings(const wxString& variant) const
{
    auto cached = m_variant_settings.find(variant);
    if (cached != m_variant_settings.end())
    {
        return cached->second;
    }
    VariantSettings settings;
    if (m_config != nullptr)
    {
        settings.asmargs = m_config->Read("/build/" + variant + "/asmargs");
        settings.defines = m_config->Read("/build/" + variant + "/defines");
        settings.outname = m_config->Read("/build/" + variant + "/outname");
    }
    return settings;
}

void BuildOptionsDialog::ShowVariantSettings(const wxString& region, bool expanded)
{
    int selection = m_ctrl_region->FindString(region);
    m_ctrl_region->SetSelection(selection == wxNOT_FOUND ? 0 : selection);
    m_ctrl_expanded->SetValue(expanded);
    VariantSettings settings = LoadVariantSettings(SelectedVariant());
    if (settings.asmargs.empty())
    {
        // No custom flags stored: show the variant's build.yaml defaults.
        settings.asmargs = AssemblyBuilderDialog::GetDefaultBuildOpts(SelectedVariant());
    }
    if (settings.defines.empty())
    {
        settings.defines = AssemblyBuilderDialog::GetDefaultDefines(SelectedVariant());
    }
    m_ctrl_asmargs->SetValue(settings.asmargs);
    m_ctrl_defines->SetValue(settings.defines);
    m_ctrl_outname->SetValue(settings.outname);
    m_displayed_variant = SelectedVariant();
}

void BuildOptionsDialog::StoreDisplayedVariantSettings()
{
    if (m_displayed_variant.empty())
    {
        return;
    }
    VariantSettings settings;
    settings.asmargs = m_ctrl_asmargs->GetValue();
    settings.defines = m_ctrl_defines->GetValue();
    settings.outname = m_ctrl_outname->GetValue();
    m_variant_settings[m_displayed_variant] = settings;
}

void BuildOptionsDialog::OnVariantChange(wxCommandEvent& /*evt*/)
{
    StoreDisplayedVariantSettings();
    ShowVariantSettings(m_ctrl_region->GetStringSelection(), m_ctrl_expanded->GetValue());
}

void BuildOptionsDialog::OnResetDefaults(wxCommandEvent& /*evt*/)
{
    const auto& defaults = AssemblyBuilderDialog::GetDefaults();
    m_ctrl_clone_in_new_dir->SetValue(defaults.clone_in_new_dir);
    m_ctrl_clonecmd->SetValue(defaults.clonecmd);
    m_ctrl_clonetag->SetValue(defaults.clonetag);
    m_ctrl_cloneurl->SetValue(defaults.cloneurl);
    m_ctrl_build_on_save->SetValue(defaults.build_on_save);
    m_ctrl_assembler->SetValue(defaults.assembler);
    m_ctrl_z80assembler->SetValue(defaults.z80assembler);
    m_ctrl_z80linker->SetValue(defaults.z80linker);
    m_ctrl_run_after_build->SetValue(defaults.run_after_build);
    m_ctrl_emulator->SetValue(defaults.emulator);
    // Reset every build variant's settings, not just the displayed one.
    for (const auto& region : AssemblyBuilderDialog::GetRegions())
    {
        VariantSettings settings;
        settings.asmargs = defaults.asmargs;
        settings.outname = defaults.outname;
        m_variant_settings[AssemblyBuilderDialog::BuildName(region, false)] = settings;
        m_variant_settings[AssemblyBuilderDialog::BuildName(region, true)] = settings;
    }
    ShowVariantSettings(m_ctrl_region->GetStringSelection(), m_ctrl_expanded->GetValue());
}

void BuildOptionsDialog::OnOK(wxCommandEvent& /*evt*/)
{
    Commit();
    EndModal(wxID_OK);
}

void BuildOptionsDialog::OnCancel(wxCommandEvent& /*evt*/)
{
    EndModal(wxID_CANCEL);
}
