#include <misc/AssemblyBuilderDialog.h>
#include <wxresource/wxcrafter.h>

#include <future>
#include <wx/progdlg.h>

static wxString init_clonecmd = "git clone {URL} --branch {TAG} --single-branch .";
static wxString init_cloneurl = "https://github.com/lordmir/landstalker_disasm.git";
static wxString init_clonetag = "0.3";
static wxString init_asmargs = "/p /o ae-,e+,w+,c+,op+,os+,ow+,oz+,l_ /e EXPANDED=0";
#ifdef __WXMSW__
static wxString init_assembler = ".\\tools\\build\\asm68k.exe";
#else
static wxString init_assembler = "wine ./tools/build/asm68k.exe";
#endif
static wxString init_baseasm = "landstalker_us.asm";
static wxString init_outname = "landstalker.bin";
#ifdef __WXMSW__
static wxString init_emulator = "fusion.exe";
#else
static wxString init_emulator = "kega-fusion";
#endif
static bool init_run_after_build = true;
static bool init_build_on_save = true;
static bool init_clone_in_new_dir = true;


wxString AssemblyBuilderDialog::clonecmd;
wxString AssemblyBuilderDialog::cloneurl;
wxString AssemblyBuilderDialog::clonetag;
wxString AssemblyBuilderDialog::asmargs;
wxString AssemblyBuilderDialog::assembler;
wxString AssemblyBuilderDialog::baseasm;
wxString AssemblyBuilderDialog::outname;
wxString AssemblyBuilderDialog::emulator;
bool AssemblyBuilderDialog::run_after_build;
bool AssemblyBuilderDialog::build_on_save;
bool AssemblyBuilderDialog::clone_in_new_dir;


AssemblyBuilderDialog::AssemblyBuilderDialog(wxWindow* parent, const wxString& dir, std::shared_ptr<Landstalker::GameData> gd, Func fn, std::shared_ptr<Landstalker::Rom> rom)
    : wxDialog(parent, wxID_ANY, "Build", wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_execThread(nullptr),
      m_step(Step::IDLE),
      m_gd(gd),
      m_dir(dir),
      m_rom(rom),
      m_fn(fn),
      m_operation_succeeded(false)
{
    m_logctrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_WORDWRAP | wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    auto font = m_logctrl->GetFont();
    font.SetFamily(wxFONTFAMILY_TELETYPE);
    m_logctrl->SetBackgroundColour(*wxWHITE);
    m_logctrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
    m_ok = new wxButton(this, wxID_OK, "OK");

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxStdDialogButtonSizer* btnszr = new wxStdDialogButtonSizer();

    sizer->Add(m_logctrl,
        wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));
    sizer->Add(btnszr, 0, wxALL, 5);
    m_ok->SetDefault();
    btnszr->AddButton(m_ok);
    btnszr->Realize();

    this->SetSizer(sizer);
    this->Layout();
    MakeBusy();

    m_ok->Bind(wxEVT_BUTTON, &AssemblyBuilderDialog::OnOK, this);
    Bind(wxEVT_CLOSE_WINDOW, &AssemblyBuilderDialog::OnClose, this);
    Bind(wxEVT_END_PROCESS, &AssemblyBuilderDialog::OnProcessComplete, this);
    Bind(wxEVT_THREAD_STDIN, &AssemblyBuilderDialog::OnThreadInput, this);
    Bind(wxEVT_THREAD_STDERR, &AssemblyBuilderDialog::OnThreadInput, this);
    CallAfter(&AssemblyBuilderDialog::OnInit);
}

AssemblyBuilderDialog::~AssemblyBuilderDialog()
{
    Abandon();
}

wxString AssemblyBuilderDialog::GetBuiltRomName()
{
    return outname;
}

bool AssemblyBuilderDialog::DidOperationSucceed()
{
    return m_operation_succeeded;
}

void AssemblyBuilderDialog::InitConfig(wxConfig* config)
{
    if (config != nullptr)
    {
        InitConfigVar(config, "/build/cloneurl", cloneurl, init_cloneurl);
        InitConfigVar(config, "/build/clonetag", clonetag, init_clonetag);
        InitConfigVar(config, "/build/clonecmd", clonecmd, init_clonecmd);
        InitConfigVar(config, "/build/assembler", assembler, init_assembler);
        InitConfigVar(config, "/build/asmargs", asmargs, init_asmargs);
        InitConfigVar(config, "/build/baseasm", baseasm, init_baseasm);
        InitConfigVar(config, "/build/outname", outname, init_outname);
        InitConfigVar(config, "/build/emulator", emulator, init_emulator);
        InitConfigVar(config, "/build/run_after_build", run_after_build, init_run_after_build);
        InitConfigVar(config, "/build/build_on_save", build_on_save, init_build_on_save);
        InitConfigVar(config, "/build/clone_in_new_dir", clone_in_new_dir, init_clone_in_new_dir);
        config->Flush();
    }
}

void AssemblyBuilderDialog::InitConfigVar(wxConfig* cfg, const wxString& path, wxString& var, const wxString& defval)
{
    if (!cfg->Exists(path))
    {
        cfg->Write(path, defval);
        var = defval;
    }
    else
    {
        var = cfg->Read(path);
    }
}

void AssemblyBuilderDialog::InitConfigVar(wxConfig* cfg, const wxString& path, bool& var, bool defval)
{
    if (!cfg->Exists(path))
    {
        cfg->Write(path, defval);
        var = defval;
    }
    else
    {
        var = cfg->ReadBool(path, defval);
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
        m_operation_succeeded = Inject(true);
        MakeIdle();
        break;
    case Func::RBUILD:
        m_operation_succeeded = Inject(false);
        MakeIdle();
        break;
    case Func::RUN:
        m_operation_succeeded = Run();
        MakeIdle();
        break;
    }
}

void AssemblyBuilderDialog::OnClose(wxCloseEvent&)
{
    if (m_execThread != nullptr && m_execThread->IsRunning())
    {
        m_msgQueue.Post(ExecutorThread::ExitThread);
        m_execThread->Wait();
        delete m_execThread;
    }

    Destroy();
}

void AssemblyBuilderDialog::OnOK(wxCommandEvent&)
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
            retval = DoSave();
            if (retval)
            {
                if (build_on_save)
                {
                    retval = DoBuild();
                    m_step = Step::BUILD;
                }
                else
                {
                    MakeIdle();
                    m_operation_succeeded = true;
                }
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
            f.SetFullName(outname);
            retval = DoFixChecksum();
            if (retval)
            {
                m_operation_succeeded = true;
                DoRun(f.GetFullPath(), true);
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

    if (dir.HasFiles() || dir.HasSubDirs())
    {
        m_step = Step::BUILD;
        if (!post_save || build_on_save)
        {
            if (!DoSave())
            {
                return false;
            }
            return DoBuild();
        }
        else
        {
            m_operation_succeeded = DoSave();
            return false;
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
    if (post_save && !build_on_save)
    {
        return true;
    }

    if (dir.HasFiles(baseasm) && dir.HasSubDirs())
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
    auto file = wxFileName(m_dir);
    if (!file.IsFileWritable() && file.Exists())
    {
        Log("Unable to write to file \"" + m_dir + "\".\n", *wxRED);
        return false;
    }
    return DoSaveToRom() && DoRun(m_dir, post_save);
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

    wxString cmd(clonecmd);
    cmd.Replace("{TAG}", clonetag, true);
    cmd.Replace("{URL}", cloneurl, true);
    Log(cmd + "\n", *wxBLUE);
    if (cmd.empty())
    {
        Log("Clone command has not been set!", *wxRED);
        m_msgQueue.Post(ExecutorThread::ThreadMessage::ExitThread);
        return false;
    }

    wxExecuteEnv env;
    env.cwd = m_dir;
    if (wxExecute(cmd, wxEXEC_ASYNC, process, &env) < 1)
    {
        Log("Command execution failed!", *wxRED);
        m_msgQueue.Post(ExecutorThread::ThreadMessage::ExitThread);
        return false;
    }
    return true;
}

bool AssemblyBuilderDialog::DoSave()
{
    Log("Updating assembly...\n", *wxBLUE);
    try
    {
        auto future = std::async(std::launch::async, [this] { return m_gd->Save(m_dir.ToStdString()); });
        double prog_value = -1.0;
        do
        {
            wxYield();
            auto progress = m_gd->GetProgress();
            if (progress.second != prog_value)
            {
                prog_value = progress.second;
                Log(Landstalker::StrPrintf("%s... (%d%% complete)\n", progress.first.c_str(), static_cast<int>(prog_value * 100.0)));
            }

        } while (future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready);
        if (future.get())
        {
            Log("Done!\n", wxColor(0, 128, 0));
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

    wxString cmd = assembler;
    cmd << " " << asmargs << " \"" << baseasm << "\",\"" << outname << "\"";
    Log(cmd + "\n", *wxBLUE);
    if (cmd.empty())
    {
        Log("Assemble command has not been set!", *wxRED);
        Abandon();
        return false;
    }

    wxExecuteEnv env;
    env.cwd = m_dir;
    auto old_cwd = wxGetCwd();
    wxSetWorkingDirectory(m_dir);

    if (wxExecute(cmd, wxEXEC_ASYNC, process, &env) < 1)
    {
        Log("Command execution failed.", *wxRED);
        Abandon();
        wxSetWorkingDirectory(old_cwd);
        return false;
    }
    wxSetWorkingDirectory(old_cwd);
    return true;
}

bool AssemblyBuilderDialog::DoFixChecksum()
{
    wxString originalDir = wxGetCwd();
    wxSetWorkingDirectory(m_dir);

    Log("Fixing ROM checksum...\n", *wxBLUE);
    bool isSuccess;
    if (wxFileName(outname).Exists())
    {
        auto r = Landstalker::Rom(outname.ToStdString());
        r.writeFile(outname.ToStdString());

        Log(Landstalker::StrPrintf("Done! Checksum is 0x%04X.\n", r.read_checksum()), wxColor(0, 128, 0));
        isSuccess = true;
    }
    else
    {
        Log(wxString("Failed to read ROM file \"") + outname + "\" while checking checksum. ", *wxRED);
        isSuccess = false;
    }

    wxSetWorkingDirectory(originalDir);
    return isSuccess;
}

bool AssemblyBuilderDialog::DoRun(const wxString& fname, bool post_build)
{
    wxString cmd = emulator;
    if (cmd.empty())
    {
        return false;
    }
    if (!post_build || run_after_build)
    {
        cmd << " \"" << fname << "\"";
        Log(cmd + "\n", *wxBLUE);

        if (wxExecute(cmd, wxEXEC_ASYNC, nullptr) < 1)
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
    Landstalker::Rom output(*m_rom);
    {
        auto future = std::async(std::launch::async, [this, &output] { m_gd->RefreshPendingWrites(output); });
        double prog_value = -1.0;
        do
        {
            wxYield();
            auto progress = m_gd->GetProgress();
            Log(Landstalker::StrPrintf("%s... (%d%% complete)\n", progress.first.c_str(), static_cast<int>(prog_value * 100.0)));

        } while (future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready);
    }
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
        if (Landstalker::Rom::section_exists(w.first))
        {
            auto sec = m_rom->get_section(w.first);
            addr = sec.begin;
            size = sec.size();
        }
        else if (Landstalker::Rom::address_exists(w.first))
        {
            addr = m_rom->get_address(w.first);
            size = sizeof(uint32_t);
        }
        details << w.first << " @ " << Landstalker::Hex(addr) << ": write " << w.second->size() << " bytes, available "
            << size << " bytes: " << ((w.second->size() <= size) ? "OK" : "BAD") << std::endl;
        Log(details.str(), (w.second->size() <= size) ? *wxBLACK : *wxRED);
        details.str(std::string());
    }
    message << std::endl;
    Log(message.str(), warning ? *wxRED : wxColor(0, 128, 0));
    Refresh();
    Update();
    wxYieldIfNeeded();
    int answer = wxYES;
    if (warning)
    {
        message << "Proceed?";
        answer = wxMessageBox(message.str(), "Inject into ROM", wxYES_NO | (warning ? wxICON_EXCLAMATION : wxICON_INFORMATION));
    }
    if (answer != wxYES)
    {
        m_gd->AbandomRomInjection();
        Log("ROM Injection abandoned.\n", *wxRED);
    }
    else
    {
        m_gd->InjectIntoRom(output);
        output.writeFile(m_dir.ToStdString());
        Log("ROM Injection complete!\n", wxColor(0, 128, 0));
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
