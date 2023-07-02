#include <AssemblyBuilderDialog.h>
#include <wxcrafter.h>

AssemblyBuilderDialog::AssemblyBuilderDialog(wxWindow* parent, const wxString& dir, std::shared_ptr<GameData> gd, Func fn, std::shared_ptr<Rom> rom)
    : wxDialog(parent, wxID_ANY, "Build", wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_execThread(nullptr),
      m_step(Step::IDLE),
      m_gd(gd),
      m_dir(dir),
      m_rom(rom),
      m_fn(fn)
{
    m_config = new wxConfig(APPLICATION_NAME);

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    m_logctrl = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_WORDWRAP | wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    auto font = m_logctrl->GetFont();
    font.SetFamily(wxFONTFAMILY_TELETYPE);
    m_logctrl->SetBackgroundColour(*wxWHITE);
    m_logctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
    m_ok = new wxButton(panel, wxID_OK, "OK");

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStdDialogButtonSizer* btnszr = new wxStdDialogButtonSizer();

    sizer->Add(m_logctrl,
        wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));
    sizer->Add(btnszr, 0, wxALL, 5);
    m_ok->SetDefault();
    btnszr->AddButton(m_ok);
    btnszr->Realize();

    panel->SetSizer(sizer);
    panel->Layout();
    MakeBusy();
    InitConfig();

    m_ok->Bind(wxEVT_BUTTON, &AssemblyBuilderDialog::OnOK, this);
    Bind(wxEVT_CLOSE_WINDOW, &AssemblyBuilderDialog::OnClose, this);
    Bind(wxEVT_END_PROCESS, &AssemblyBuilderDialog::OnProcessComplete, this);
    Bind(wxEVT_THREAD_STDIN, &AssemblyBuilderDialog::OnThreadInput, this);
    Bind(wxEVT_THREAD_STDERR, &AssemblyBuilderDialog::OnThreadInput, this);
    CallAfter(&AssemblyBuilderDialog::OnInit);
}

AssemblyBuilderDialog::~AssemblyBuilderDialog()
{

}

wxString AssemblyBuilderDialog::GetBuiltRomName()
{
    return m_outname;
}

void AssemblyBuilderDialog::InitConfig()
{
    if (m_config != nullptr)
    {
        m_clonecmd = m_config->Read("/build/clonecmd");
        m_cloneurl = m_config->Read("/build/cloneurl");
        m_clonetag = m_config->Read("/build/clonetag");
        m_assembler = m_config->Read("/build/assembler");
        m_asmargs = m_config->Read("/build/asmargs");
        m_baseasm = m_config->Read("/build/baseasm");
        m_outname = m_config->Read("/build/outname");
        m_emulator = m_config->Read("/build/emulator");
        m_run_after_build = m_config->ReadBool("/build/run_after_build", true);
        m_build_on_save = m_config->ReadBool("/build/build_on_save", true);
        m_clone_in_new_dir = m_config->ReadBool("/build/clone_in_new_dir", true);
    }
}

void AssemblyBuilderDialog::OnInit()
{
    switch (m_fn)
    {
    case Func::BUILD:
        if (!Assemble(false))
        {
            MakeIdle();
        }
        break;
    case Func::SAVE_ASM:
        if (!Assemble(true))
        {
            MakeIdle();
        }
        break;
    case Func::INJECT:
        Inject(true);
        MakeIdle();
        break;
    case Func::RBUILD:
        Inject(false);
        MakeIdle();
        break;
    case Func::RUN:
        Run();
        MakeIdle();
        break;
    }
}

void AssemblyBuilderDialog::OnClose(wxCloseEvent& evt)
{
    if (m_execThread != nullptr && m_execThread->IsRunning())
    {
        m_msgQueue.Post(ExecutorThread::ExitThread);
        m_execThread->Wait();
        delete m_execThread;
    }

    Destroy();
}

void AssemblyBuilderDialog::OnOK(wxCommandEvent& evt)
{
    EndModal(wxID_OK);
}

void AssemblyBuilderDialog::OnProcessComplete(wxProcessEvent& evt)
{
    bool retval = false;
    if (evt.GetExitCode() != 0)
    {
        Log("Command failed with code " + std::to_string(evt.GetExitCode()), *wxRED);
        Abandon();
        MakeIdle();
        return;
    }
    switch (m_step)
    {
    case Step::IDLE:
        MakeIdle();
        break;
    case Step::CLONE:
        if (JoinThread())
        {
            retval = DoSave() && DoBuild();
            if (retval)
            {
                m_step = Step::BUILD;
            }
            else
            {
                Abandon();
                MakeIdle();
            }
        }
        break;
    case Step::BUILD:
        if (JoinThread())
        {
            auto f = wxFileName(m_dir, "");
            f.SetFullName(m_outname);
            retval = DoFixChecksum() && DoRun(f.GetFullPath(), true);
            if (retval)
            {
                m_step = Step::BUILD;
            }
            else
            {
                Abandon();
            }
            MakeIdle();
            m_step = Step::IDLE;
        }
        break;
    }
}

void AssemblyBuilderDialog::OnThreadInput(wxThreadEvent& evt)
{
    Log(evt.GetString());
}

bool AssemblyBuilderDialog::Assemble(bool post_save)
{
    if (!wxDir::Exists(m_dir))
    {
        Log("Directory \"" + m_dir + "\" does not exist.\n", *wxRED);
        return false;
    }
    auto dir = wxDir(m_dir);
    if (!dir.IsOpened())
    {
        Log("Unable to write to directory \"" + m_dir + "\".\n", *wxRED);
        return false;
    }
    wxSetWorkingDirectory(m_dir);
    if (dir.HasFiles() || dir.HasSubDirs())
    {
        m_step = Step::BUILD;
        if (!post_save || m_build_on_save)
        {
            return DoSave() && DoBuild();
        }
        else
        {
            return DoSave();
        }
    }
    else
    {
        m_step = Step::CLONE;
        return DoClone();
    }
}

bool AssemblyBuilderDialog::Build(bool post_save)
{
    if (!wxDir::Exists(m_dir))
    {
        Log("Directory \"" + m_dir + "\" does not exist.\n", *wxRED);
        return false;
    }
    auto dir = wxDir(m_dir);
    if (!dir.IsOpened())
    {
        Log("Unable to write to directory \"" + m_dir + "\".\n", *wxRED);
        return false;
    }
    if (post_save && !m_build_on_save)
    {
        return true;
    }
    wxSetWorkingDirectory(m_dir);
    if (dir.HasFiles(m_baseasm) && dir.HasSubDirs())
    {
        m_step = Step::BUILD;
        return DoSave() && DoBuild();
    }
    else
    {
        Log("No top-level assembly file exists in \"" + m_dir + "\"!", *wxRED);
        return false;
    }
}

bool AssemblyBuilderDialog::Inject(bool post_save)
{
    if (post_save && !m_build_on_save)
    {
        return true;
    }
    auto file = wxFileName(m_dir);
    if (!file.IsFileWritable() && file.Exists())
    {
        Log("Unable to write to file \"" + m_dir + "\".\n", *wxRED);
        return false;
    }
    return DoSaveToRom() && DoRun(m_dir, true);
}

bool AssemblyBuilderDialog::Run()
{
    DoRun(m_dir, false);
    EndModal(wxID_OK);
    return true;
}

void AssemblyBuilderDialog::Log(const wxString& str, const wxColor& colour)
{
    m_logctrl->SetDefaultStyle(wxTextAttr(colour));
    m_logctrl->AppendText(str);
    m_logctrl->SetDefaultStyle(wxTextAttr(*wxBLACK));
}

bool AssemblyBuilderDialog::JoinThread()
{
    if (m_execThread != nullptr && m_execThread->IsRunning())
    {
        m_msgQueue.Post(ExecutorThread::ProcessComplete);
        m_execThread->Wait();
        delete m_execThread;
        m_execThread = nullptr;
        return true;
    }
    return false;
}

void AssemblyBuilderDialog::Abandon()
{
    if (m_execThread != nullptr && m_execThread->IsRunning())
    {
        m_msgQueue.Post(ExecutorThread::ExitThread);
        m_execThread->Wait();
        delete m_execThread;
        m_execThread = nullptr;
    }
}

bool AssemblyBuilderDialog::DoClone()
{
    // Create a process and and encoder thread.
    wxProcess* process = new wxProcess(this);
    process->Redirect();

    m_msgQueue.Clear();
    m_execThread = new ExecutorThread(this, process, m_msgQueue);
    m_execThread->Run();

    if (!m_execThread->IsRunning())
    {
        Log("Unable to launch thread.\n", *wxRED);
        delete m_execThread;
        m_execThread = nullptr;
        return false;
    }

    wxString cmd(m_clonecmd);
    cmd.Replace("{TAG}", m_clonetag, true);
    cmd.Replace("{URL}", m_cloneurl, true);
    Log(cmd + "\n", *wxBLUE);

    if (wxExecute(cmd, wxEXEC_ASYNC, process) < 1)
    {
        Log("Command execution failed!", *wxRED);
        return false;
    }
    return true;
}

bool AssemblyBuilderDialog::DoSave()
{
    Log("Updating assembly...\n", *wxBLUE);
    try
    {
        if (m_gd->Save(m_dir.ToStdString()))
        {
            Log("Done!\n", *wxGREEN);
            return true;
        }
        else
        {
            Log("ASM Generation failed.\n", *wxRED);
        }
    }
    catch (const std::exception& e)
    {
        Log(wxString("ASM Generation Error: ") + e.what(), *wxRED);
    }
    return false;
}

bool AssemblyBuilderDialog::DoBuild()
{
    wxProcess* process = new wxProcess(this);
    process->Redirect();
    m_execThread = new ExecutorThread(this, process, m_msgQueue);
    m_execThread->Run();

    if (!m_execThread->IsRunning())
    {
        Log("Unable to launch thread.\n", *wxRED);
        delete m_execThread;
        m_execThread = nullptr;
        return false;
    }

    wxString cmd = m_assembler;
    cmd << " " << m_asmargs << " " << m_baseasm << "," << m_outname;
    Log(cmd + "\n", *wxBLUE);
    if (wxExecute(cmd, wxEXEC_ASYNC, process) < 1)
    {
        Log("Command execution failed.", *wxRED);
        return false;
    }
    return true;
}

bool AssemblyBuilderDialog::DoFixChecksum()
{
    if (wxFileName(m_outname).Exists())
    {
        Log("Fixing ROM checksum...\n", *wxBLUE);
        auto r = Rom(m_outname.ToStdString());
        r.writeFile(m_outname.ToStdString());

        Log(StrPrintf("Done! Checksum is 0x%04X.\n", r.read_checksum()), *wxGREEN);
        return true;
    }
    else
    {
        Log(wxString("Failed to read ROM file \"") + m_outname + "\".");
    }
    return false;
}

bool AssemblyBuilderDialog::DoRun(const wxString& fname, bool post_build)
{
    wxString cmd = m_emulator;
    if (cmd.empty())
    {
        return false;
    }
    if (!post_build || m_run_after_build)
    {
        cmd << " " << fname;
        Log(cmd + "\n", *wxBLUE);
        if (wxExecute(cmd, wxEXEC_ASYNC) < 1)
        {
            Log("Command execution failed.", *wxRED);
            return false;
        }
    }
    return true;
}

bool AssemblyBuilderDialog::DoSaveToRom()
{
    bool retval = false;
    if (m_rom == nullptr)
    {
        return false;
    }
    std::ostringstream message, details;
    m_gd->RefreshPendingWrites(*m_rom);
    auto result = m_gd->GetPendingWrites();
    bool warning = false;
    try
    {
        if (!m_gd->WillFitInRom(*m_rom))
        {
            message << "Warning: Data will not fit in ROM without overwriting existing structures!\n";
            message << "To avoid this issue, it is recommended to use a disassembly source.\n\n";
            warning = true;
        }
        else
        {
            message << "Success: Data will fit into ROM without overwriting existing structures.\n\n";
        }
    }
    catch (std::exception& e)
    {
        Log(_("Error encountered during data generation: ") + e.what(), *wxRED);
        return false;
    }
    for (const auto& w : result)
    {
        uint32_t addr = 0;
        uint32_t size = 0;
        if (m_rom->section_exists(w.first))
        {
            auto sec = m_rom->get_section(w.first);
            addr = sec.begin;
            size = sec.size();
        }
        else if (m_rom->address_exists(w.first))
        {
            addr = m_rom->get_address(w.first);
            size = sizeof(uint32_t);
        }
        details << w.first << " @ " << Hex(addr) << ": write " << w.second->size() << " bytes, available "
            << size << " bytes: " << ((w.second->size() <= size) ? "OK" : "BAD") << std::endl;
        Log(details.str(), (w.second->size() <= size) ? *wxBLACK : *wxRED);
        details.str(std::string());
    }
    message << std::endl;
    Log(message.str(), warning ? *wxRED : *wxGREEN);
    Refresh();
    Update();
    wxYieldIfNeeded();
    message << "Proceed?";
    int answer = wxMessageBox(message.str(), "Inject into ROM", wxYES_NO | (warning ? wxICON_EXCLAMATION : wxICON_INFORMATION));
    if (answer != wxYES)
    {
        m_gd->AbandomRomInjection();
        Log("ROM Injection abandoned.\n", *wxRED);
    }
    else
    {
        Rom output(*m_rom);
        m_gd->InjectIntoRom(output);
        output.writeFile(m_dir.ToStdString());
        Log("ROM Injection complete!\n", *wxGREEN);
        retval = true;
    }
    Refresh();
    Update();
    wxYieldIfNeeded();
    return retval;
}

void AssemblyBuilderDialog::MakeBusy()
{
    m_ok->Enable(false);
    SetCursor(*wxHOURGLASS_CURSOR);
}

void AssemblyBuilderDialog::MakeIdle()
{
    m_ok->Enable();
    m_ok->SetDefault();
    SetCursor(wxNullCursor);
}
