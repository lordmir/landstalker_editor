#include <AssemblyBuilderDialog.h>

const char* args = "/p /o ae-,e+,w+,c+,op+,os+,ow+,oz+,l_ /e EXPANDED=0";
const char* url = "https://github.com/lordmir/landstalker_disasm.git";
const char* tag = "0.2";
#ifdef __WXMSW__
const char* workdir = "c:\\projects\\lsdisasm\\";
const char* assembler = ".\\tools\\build\\asm68k.exe";
const char* emulator = "C:\\Megadrive\\Fusion364\\Fusion.exe";
#else
const char* workdir = "~/projects/lsdisasm/";
const char* assembler = "wine ./tools/build/asm68k.exe";
const char* emulator = "kega-fusion";
#endif

AssemblyBuilderDialog::AssemblyBuilderDialog(wxWindow* parent, const wxString& dir, std::shared_ptr<GameData> gd, Func fn, std::shared_ptr<Rom> rom)
    : wxDialog(parent, wxID_ANY, "Assemble", wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_execThread(nullptr),
      m_step(Step::IDLE),
      m_gd(gd),
      m_dir(dir),
      m_rom(rom),
      m_fn(fn)
{

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

    sizer->Add(m_logctrl,
        wxSizerFlags(1).Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));
    sizer->Add(m_ok,
        wxSizerFlags().Border(wxLEFT | wxRIGHT | wxBOTTOM).Right());

    panel->SetSizer(sizer);
    panel->Layout();

    m_ok->Bind(wxEVT_BUTTON, &AssemblyBuilderDialog::OnOK, this);
    Bind(wxEVT_CLOSE_WINDOW, &AssemblyBuilderDialog::OnClose, this);
    Bind(wxEVT_INIT_DIALOG, &AssemblyBuilderDialog::OnInit, this);
    Bind(wxEVT_END_PROCESS, &AssemblyBuilderDialog::OnProcessComplete, this);
    Bind(wxEVT_THREAD_STDIN, &AssemblyBuilderDialog::OnThreadInput, this);
    Bind(wxEVT_THREAD_STDERR, &AssemblyBuilderDialog::OnThreadInput, this);
}

AssemblyBuilderDialog::~AssemblyBuilderDialog()
{

}

void AssemblyBuilderDialog::OnInit(wxInitDialogEvent& evt)
{
    switch (m_fn)
    {
    case Func::BUILD:
        Build();
        break;
    case Func::SAVE_ASM:
        Assemble();
        break;
    case Func::INJECT:
        Inject();
        break;
    case Func::PATCH:
        MakePatch();
        break;
    case Func::RUN:
        Run();
        break;
    }
    
    evt.Skip();
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
    switch (m_step)
    {
    case Step::IDLE:
        break;
    case Step::CLONE:
        if (JoinThread())
        {
            DoSave();
            DoBuild();
            m_step = Step::BUILD;
        }
        break;
    case Step::BUILD:
        if (JoinThread())
        {
            DoFixChecksum();
            auto f = wxFileName(m_dir, "");
            f.SetFullName("landstalker.bin");
            DoRun(f.GetFullPath());
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

void AssemblyBuilderDialog::Assemble()
{
    if (!wxDir::Exists(m_dir))
    {
        Log("directory does not exist.\n", *wxRED);
        return;
    }
    auto dir = wxDir(m_dir);
    if (!dir.IsOpened())
    {
        Log("Unable to write to directory.\n", *wxRED);
        return;
    }
    MakeBusy();
    wxSetWorkingDirectory(m_dir);
    if (dir.HasFiles() || dir.HasSubDirs())
    {
        m_step = Step::BUILD;
        DoSave();
        DoBuild();
    }
    else
    {
        m_step = Step::CLONE;
        DoClone();
    }
}

void AssemblyBuilderDialog::Build()
{
    if (!wxDir::Exists(m_dir))
    {
        Log("directory does not exist.\n", *wxRED);
        return;
    }
    auto dir = wxDir(m_dir);
    if (!dir.IsOpened())
    {
        Log("Unable to write to directory.\n", *wxRED);
        return;
    }
    MakeBusy();
    wxSetWorkingDirectory(m_dir);
    if (dir.HasFiles() || dir.HasSubDirs())
    {
        m_step = Step::BUILD;
        DoSave();
        DoBuild();
    }
    else
    {
        Log("No assembly exists!", *wxRED);
    }
}

void AssemblyBuilderDialog::Inject()
{
    if (!wxDir::Exists(m_dir))
    {
        Log("directory does not exist.\n", *wxRED);
        return;
    }
    auto file = wxFileName(m_dir);
    if (!file.IsFileWritable())
    {
        Log("Unable to write to file.\n", *wxRED);
        return;
    }
    MakeBusy();
    DoSaveToRom();
    DoRun(m_dir);
    MakeIdle();
}

void AssemblyBuilderDialog::MakePatch()
{
    if (!wxDir::Exists(m_dir))
    {
        Log("directory does not exist.\n", *wxRED);
        return;
    }
    auto file = wxFileName(m_dir);
    if (!file.IsFileWritable())
    {
        Log("Unable to write to file.\n", *wxRED);
        return;
    }
    MakeBusy();
    DoMakePatch();
    MakeIdle();
}

void AssemblyBuilderDialog::Run()
{
    DoRun(m_dir);
    EndModal(wxID_OK);
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

void AssemblyBuilderDialog::DoClone()
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
        return;
    }

    wxString cmd = "git clone ";
    cmd << url << " --branch " << tag << " --single-branch .";
    Log(cmd + "\n", *wxBLUE);

    wxExecute(cmd, wxEXEC_ASYNC, process);
}

void AssemblyBuilderDialog::DoSave()
{
    Log("Updating assembly...\n", *wxBLUE);
    m_gd->Save(m_dir.ToStdString());
    Log("Done!\n", *wxGREEN);
}

void AssemblyBuilderDialog::DoBuild()
{
    wxProcess* process = new wxProcess(this);
    process->Redirect();
    m_execThread = new ExecutorThread(this, process, m_msgQueue);
    m_execThread->Run();
    wxString cmd = assembler;
    cmd << " " << args << " landstalker_us.asm,landstalker.bin";
    Log(cmd + "\n", *wxBLUE);
    wxExecute(cmd, wxEXEC_ASYNC, process);
}

void AssemblyBuilderDialog::DoFixChecksum()
{
    Log("Fixing ROM checksum...\n", *wxBLUE);
    auto r = Rom("landstalker.bin");
    r.writeFile("landstalker.bin");
    Log("Done!\n", *wxGREEN);
}

void AssemblyBuilderDialog::DoRun(const wxString& fname)
{
    wxString cmd = emulator;
    cmd << " " << fname;
    Log(cmd + "\n", *wxBLUE);
    wxExecute(cmd, wxEXEC_ASYNC);
}

void AssemblyBuilderDialog::DoMakePatch()
{
}

void AssemblyBuilderDialog::DoSaveToRom()
{
    if (m_rom == nullptr)
    {
        return;
    }
    std::ostringstream message, details;
    m_gd->RefreshPendingWrites(*m_rom);
    auto result = m_gd->GetPendingWrites();
    bool warning = false;
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
    }
    message << std::endl;
    details << std::endl;
    Log(message.str(), warning ? *wxRED : *wxGREEN);
    Log(details.str());
    message << "Proceed?";
    int answer = wxMessageBox(message.str(), "Inject into ROM", wxYES_NO | (warning ? wxICON_EXCLAMATION : wxICON_INFORMATION));
    if (answer != wxID_YES)
    {
        m_gd->AbandomRomInjection();
        Log("ROM Injection abandoned.", *wxRED);
    }
    else
    {
        Rom output(*m_rom);
        m_gd->InjectIntoRom(output);
        output.writeFile(m_dir.ToStdString());
        Log("ROM Injection complete!", *wxGREEN);
    }
}

void AssemblyBuilderDialog::MakeBusy()
{
    m_ok->Enable(false);
    SetCursor(*wxHOURGLASS_CURSOR);
}

void AssemblyBuilderDialog::MakeIdle()
{
    m_ok->Enable();
    SetCursor(wxNullCursor);
}
