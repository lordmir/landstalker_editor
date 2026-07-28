#include <misc/AssemblyBuilderDialog.h>
#include <wxresource/wxcrafter.h>

#include <algorithm>
#include <future>
#include <wx/progdlg.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <yaml-cpp/yaml.h>

static const wxString default_region = "US";

// Assembler options shared by every build in build.yaml, used when the
// assembly source predates build.yaml.
static const wxString standard_buildopts = "/p /d /o ae-,e+,w+,c+,op+,os+,ow+,oz+,l_";

// Z80 sound driver and music bank build steps, as per build.bat/build.sh in
// the disassembly. Sources are assembled with asw and located with p2bin.
struct Z80BuildStep
{
    const char* name;
    const char* source;
    const char* object;
    const char* binary;
    const char* link_opts;
};
static const Z80BuildStep z80_build_steps[] = {
    {"Sound Bank 3", "code/audio/soundbank3.asm", "soundbank3.p", "soundbank3.bin", "-l 0xff -r 0x8000-0xdfff -k"},
    {"Sound Bank 4", "code/audio/soundbank4.asm", "soundbank4.p", "soundbank4.bin", "-l 0xff -r 0x8000-0xffff -k"},
    {"Cube/Iwadare Driver", "code/audio/main.asm", "cube.p", "cube.bin", "-l 0xff -r 0x0000-0x1f7f -k"},
};

// Per-region 68k defines, mirroring build.yaml. Used when the assembly
// source has no build.yaml (or it lacks the selected build).
struct RegionDefaults
{
    const char* region;
    int values[14]; // Order matches default_define_names below, minus EXPANDED.
};
static const char* default_define_names[] = {
    "REGION", "NTSC", "REGION_CHECK", "FIX_COLL_1", "FIX_COLL_2", "FIX_ARMLET_SKIP",
    "FIX_WHISTLE_CHECK", "FIX_SPRITE_HIDE", "ENABLE_GOLD_COUNT", "FIX_GOLA_BUG",
    "FIX_GOLD_CAP", "FIX_END_CREDS", "REFRESH_GOLD_CTR", "FIX_TS_GLITCH"
};
static const RegionDefaults region_defaults[] = {
    {"US",   {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1}},
    {"JP",   {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0}},
    {"EUR",  {2, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},
    {"FR",   {3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1}},
    {"DE",   {4, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1}},
    {"BETA", {5, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0}},
};


wxString AssemblyBuilderDialog::clonecmd;
wxString AssemblyBuilderDialog::cloneurl;
wxString AssemblyBuilderDialog::clonetag;
wxString AssemblyBuilderDialog::assembler;
wxString AssemblyBuilderDialog::z80assembler;
wxString AssemblyBuilderDialog::z80linker;
wxString AssemblyBuilderDialog::emulator;
bool AssemblyBuilderDialog::run_after_build;
bool AssemblyBuilderDialog::build_on_save;
bool AssemblyBuilderDialog::clone_in_new_dir;
wxString AssemblyBuilderDialog::project_region = default_region;
bool AssemblyBuilderDialog::project_expanded = false;
wxString AssemblyBuilderDialog::project_dir;
wxConfig* AssemblyBuilderDialog::config_store = nullptr;


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
    return m_built_rom_name;
}

bool AssemblyBuilderDialog::DidOperationSucceed()
{
    return m_operation_succeeded;
}

const wxArrayString& AssemblyBuilderDialog::GetRegions()
{
    static wxArrayString regions;
    if (regions.empty())
    {
        for (const auto& defaults : region_defaults)
        {
            regions.Add(defaults.region);
        }
    }
    return regions;
}

const AssemblyBuilderDialog::ConfigDefaults& AssemblyBuilderDialog::GetDefaults()
{
    static const ConfigDefaults defaults = {
        /* clonecmd */         "git clone {URL} --branch {TAG} --single-branch .",
        /* cloneurl */         "https://github.com/lordmir/landstalker_disasm.git",
        /* clonetag */         "v0.4",
#ifdef __WXMSW__
        /* assembler */        ".\\tools\\build\\asm68k.exe",
        // Blank flags/output mean "derive from build.yaml / the project region".
        /* asmargs */          "",
        /* z80assembler */     ".\\tools\\build\\asw\\asw.exe",
        /* z80linker */        ".\\tools\\build\\asw\\p2bin.exe",
        /* outname */          "",
        /* emulator */         "fusion.exe",
#else
        /* assembler */        "wine ./tools/build/asm68k.exe",
        /* asmargs */          "",
        /* z80assembler */     "wine ./tools/build/asw/asw.exe",
        /* z80linker */        "wine ./tools/build/asw/p2bin.exe",
        /* outname */          "",
        /* emulator */         "kega-fusion",
#endif
        /* run_after_build */  true,
        /* build_on_save */    true,
        /* clone_in_new_dir */ true
    };
    return defaults;
}

void AssemblyBuilderDialog::SetProjectRegionFromAsm(const wxString& asm_path)
{
    // The top-level assembly documents its region and build options in
    // comments near the top of the file, e.g.:
    //   ;; BUILDOPTS = /p /o ... /e REGION=0;NTSC=1;EXPANDED=0;...
    //   ;; REGION = US
    wxString detected_region = default_region;
    bool detected_expanded = false;
    bool region_found = false;
    bool expanded_found = false;
    wxTextFile file(asm_path);
    if (file.Open())
    {
        std::size_t lines_to_scan = std::min<std::size_t>(file.GetLineCount(), 100);
        for (std::size_t i = 0; i < lines_to_scan && !(region_found && expanded_found); ++i)
        {
            wxString line = file.GetLine(i);
            line.Trim(false);
            if (!line.StartsWith(";"))
            {
                continue;
            }
            while (line.StartsWith(";"))
            {
                line.Remove(0, 1);
            }
            line.Trim(false);
            if (!expanded_found && line.StartsWith("BUILDOPTS"))
            {
                int pos = line.Find("EXPANDED=");
                if (pos != wxNOT_FOUND && static_cast<std::size_t>(pos) + 9 < line.length())
                {
                    detected_expanded = line[pos + 9] != '0';
                    expanded_found = true;
                }
                continue;
            }
            if (region_found || !line.StartsWith("REGION"))
            {
                continue;
            }
            line = line.Mid(6);
            line.Trim(false);
            if (!line.StartsWith("="))
            {
                continue; // e.g. REGION_CHECK
            }
            wxString value = line.Mid(1).Trim(false).BeforeFirst(' ').Trim().Upper();
            if (GetRegions().Index(value) != wxNOT_FOUND)
            {
                detected_region = value;
                region_found = true;
            }
        }
        file.Close();
    }
    project_region = detected_region;
    project_expanded = detected_expanded;
    project_dir = wxFileName(asm_path).GetPath();
}

void AssemblyBuilderDialog::SetProjectRegion(const wxString& new_region)
{
    project_region = (GetRegions().Index(new_region) != wxNOT_FOUND) ? new_region : default_region;
    project_expanded = false;
    project_dir.clear();
}

wxString AssemblyBuilderDialog::GetProjectRegion()
{
    return project_region;
}

bool AssemblyBuilderDialog::GetProjectExpanded()
{
    return project_expanded;
}

wxString AssemblyBuilderDialog::BuildName(const wxString& build_region, bool build_expanded)
{
    return build_region + (build_expanded ? "_EXPANDED" : "");
}

wxString AssemblyBuilderDialog::GetDefaultBuildOpts(const wxString& build_name)
{
    if (!project_dir.empty())
    {
        const wxString build_yaml = project_dir + wxFileName::GetPathSeparator() + "build.yaml";
        if (wxFileExists(build_yaml))
        {
            try
            {
                YAML::Node root = YAML::LoadFile(build_yaml.ToStdString());
                YAML::Node build = root["Builds"][build_name.ToStdString()];
                if (build && build["Buildopts"])
                {
                    return wxString(build["Buildopts"].as<std::string>());
                }
            }
            catch (const std::exception&)
            {
                // Fall through to the standard options.
            }
        }
    }
    return standard_buildopts;
}

void AssemblyBuilderDialog::InitConfig(wxConfig* config)
{
    config_store = config;
    if (config != nullptr)
    {
        const ConfigDefaults& defaults = GetDefaults();
        // Migrate defaults from before the disassembly gained regions and the
        // Z80 sound driver build: the old flags hardcoded EXPANDED=0 and the
        // old output name hid the region-specific ROM naming.
        if (config->Read("/build/asmargs") == "/p /o ae-,e+,w+,c+,op+,os+,ow+,oz+,l_ /e EXPANDED=0")
        {
            config->Write("/build/asmargs", "");
        }
        if (config->Read("/build/outname") == "landstalker.bin")
        {
            config->Write("/build/outname", "");
        }
        if (config->Read("/build/clonetag") == "0.3")
        {
            config->Write("/build/clonetag", defaults.clonetag);
        }
        // Migrate the briefly-used global region settings to per-region storage.
        if (config->Exists("/build/region") || config->Exists("/build/expanded") ||
            config->Exists("/build/asmargs") || config->Exists("/build/outname"))
        {
            wxString old_region = config->Read("/build/region", default_region);
            if (GetRegions().Index(old_region) == wxNOT_FOUND)
            {
                old_region = default_region;
            }
            if (config->Exists("/build/asmargs"))
            {
                config->Write("/build/" + old_region + "/asmargs", config->Read("/build/asmargs"));
            }
            if (config->Exists("/build/outname"))
            {
                config->Write("/build/" + old_region + "/outname", config->Read("/build/outname"));
            }
            config->DeleteEntry("/build/region");
            config->DeleteEntry("/build/expanded");
            config->DeleteEntry("/build/asmargs");
            config->DeleteEntry("/build/outname");
        }
        // The expanded state is derived from the opened assembly and forms
        // part of the storage key rather than being stored itself.
        for (const auto& region_name : GetRegions())
        {
            config->DeleteEntry("/build/" + region_name + "/expanded");
        }
        InitConfigVar(config, "/build/cloneurl", cloneurl, defaults.cloneurl);
        InitConfigVar(config, "/build/clonetag", clonetag, defaults.clonetag);
        InitConfigVar(config, "/build/clonecmd", clonecmd, defaults.clonecmd);
        InitConfigVar(config, "/build/assembler", assembler, defaults.assembler);
        InitConfigVar(config, "/build/z80assembler", z80assembler, defaults.z80assembler);
        InitConfigVar(config, "/build/z80linker", z80linker, defaults.z80linker);
        InitConfigVar(config, "/build/emulator", emulator, defaults.emulator);
        InitConfigVar(config, "/build/run_after_build", run_after_build, defaults.run_after_build);
        InitConfigVar(config, "/build/build_on_save", build_on_save, defaults.build_on_save);
        InitConfigVar(config, "/build/clone_in_new_dir", clone_in_new_dir, defaults.clone_in_new_dir);
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
    const int exit_code = evt.GetExitCode();
    if (!JoinThread())
    {
        Log("Unable to collect command output.\n", *wxRED);
        Abandon();
        MakeIdle();
        return;
    }

    // ExecutorThread queues its final output before JoinThread() returns. Defer
    // the state transition so those events are appended before the failure
    // message or the next command line, rather than appearing after them.
    CallAfter([this, exit_code]() { HandleProcessComplete(exit_code); });
}

void AssemblyBuilderDialog::HandleProcessComplete(int exit_code)
{
    bool retval = false;
    if (exit_code != 0)
    {
        Log("Command failed with code " + std::to_string(exit_code) + "\n", *wxRED);
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
        break;
    case Step::BUILD:
        if (!m_build_queue.empty())
        {
            if (!RunNextBuildCommand())
            {
                Abandon();
                MakeIdle();
                m_step = Step::IDLE;
            }
            break;
        }
        {
            auto f = wxFileName(m_dir, "");
            f.SetFullName(m_built_rom_name);
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

    if (dir.HasFiles() && dir.HasSubDirs())
    {
        m_step = Step::BUILD;
        return DoSave() && DoBuild();
    }
    else
    {
        Log("No assembly source exists in \"" + m_dir + "\"!", *wxRED);
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
    if (m_execThread != nullptr)
    {
        if (m_execThread->IsRunning())
        {
            m_msgQueue.Post(ExecutorThread::ProcessComplete);
        }
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
    m_build_queue.clear();
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

std::vector<std::pair<wxString, wxString>> AssemblyBuilderDialog::BuiltInDefines(const wxString& build_region, bool build_expanded)
{
    std::vector<std::pair<wxString, wxString>> defines;
    for (const auto& region_default : region_defaults)
    {
        if (build_region == region_default.region)
        {
            for (std::size_t i = 0; i < sizeof(default_define_names) / sizeof(default_define_names[0]); ++i)
            {
                defines.emplace_back(default_define_names[i], wxString::Format("%d", region_default.values[i]));
                if (defines.back().first == "NTSC")
                {
                    defines.emplace_back("EXPANDED", build_expanded ? "1" : "0");
                }
            }
            break;
        }
    }
    return defines;
}

std::vector<std::pair<wxString, wxString>> AssemblyBuilderDialog::ParseDefines(const wxString& defines)
{
    std::vector<std::pair<wxString, wxString>> parsed;
    wxStringTokenizer tokenizer(defines, ";");
    while (tokenizer.HasMoreTokens())
    {
        wxString token = tokenizer.GetNextToken().Trim().Trim(false);
        wxString name = token.BeforeFirst('=').Trim();
        wxString value = token.AfterFirst('=').Trim().Trim(false);
        if (!name.empty() && token.Contains("="))
        {
            parsed.emplace_back(name, value);
        }
    }
    return parsed;
}

wxString AssemblyBuilderDialog::FormatDefines(const std::vector<std::pair<wxString, wxString>>& defines)
{
    wxString formatted;
    for (const auto& define : defines)
    {
        formatted << (formatted.empty() ? "" : ";") << define.first << "=" << define.second;
    }
    return formatted;
}

wxString AssemblyBuilderDialog::GetDefaultDefines(const wxString& build_name)
{
    if (!project_dir.empty())
    {
        const wxString build_yaml = project_dir + wxFileName::GetPathSeparator() + "build.yaml";
        if (wxFileExists(build_yaml))
        {
            try
            {
                YAML::Node root = YAML::LoadFile(build_yaml.ToStdString());
                YAML::Node build = root["Builds"][build_name.ToStdString()];
                if (build && build["Defines"])
                {
                    std::vector<std::pair<wxString, wxString>> defines;
                    for (const auto& define : build["Defines"])
                    {
                        defines.emplace_back(define.first.as<std::string>(), define.second.as<std::string>());
                    }
                    return FormatDefines(defines);
                }
            }
            catch (const std::exception&)
            {
                // Fall through to the built-in defaults.
            }
        }
    }
    const bool build_expanded = build_name.EndsWith("_EXPANDED");
    const wxString build_region = build_expanded ? build_name.BeforeLast('_') : build_name;
    return FormatDefines(BuiltInDefines(build_region, build_expanded));
}

AssemblyBuilderDialog::BuildSettings AssemblyBuilderDialog::ResolveBuildSettings()
{
    BuildSettings settings;
    const ConfigDefaults& defaults = GetDefaults();
    wxString asmargs = defaults.asmargs;
    wxString outname = defaults.outname;
    wxString custom_defines;
    settings.expanded = project_expanded;
    const wxString build_name = BuildName(project_region, project_expanded);
    if (config_store != nullptr)
    {
        asmargs = config_store->Read("/build/" + build_name + "/asmargs", defaults.asmargs);
        outname = config_store->Read("/build/" + build_name + "/outname", defaults.outname);
        custom_defines = config_store->Read("/build/" + build_name + "/defines");
    }
    const wxString stem = "landstalker_" + project_region.Lower() + (settings.expanded ? "_expanded" : "");
    settings.source = stem + ".asm";
    settings.target = stem + ".bin";
    settings.buildopts = standard_buildopts;
    settings.defines = BuiltInDefines(project_region, project_expanded);

    const wxString build_yaml = m_dir + wxFileName::GetPathSeparator() + "build.yaml";
    if (wxFileExists(build_yaml))
    {
        try
        {
            YAML::Node root = YAML::LoadFile(build_yaml.ToStdString());
            YAML::Node build = root["Builds"][build_name.ToStdString()];
            if (build)
            {
                if (build["Source"])
                {
                    settings.source = build["Source"].as<std::string>();
                }
                if (build["Target"])
                {
                    settings.target = build["Target"].as<std::string>();
                }
                if (build["Buildopts"])
                {
                    settings.buildopts = build["Buildopts"].as<std::string>();
                }
                if (build["Defines"])
                {
                    settings.defines.clear();
                    for (const auto& define : build["Defines"])
                    {
                        settings.defines.emplace_back(define.first.as<std::string>(), define.second.as<std::string>());
                    }
                }
            }
            else
            {
                Log("Build \"" + build_name + "\" not found in build.yaml, using built-in defaults.\n", *wxRED);
            }
        }
        catch (const std::exception& e)
        {
            Log(wxString("Failed to parse build.yaml (") + e.what() + "), using built-in defaults.\n", *wxRED);
        }
    }
    else
    {
        Log("No build.yaml found in assembly directory, using built-in defaults.\n");
    }

    if (!asmargs.empty())
    {
        settings.buildopts = asmargs;
    }
    if (!outname.empty())
    {
        settings.target = outname;
    }
    if (!custom_defines.empty())
    {
        settings.defines = ParseDefines(custom_defines);
    }
    wxFileName source_name(settings.source);
    source_name.SetExt("sym");
    settings.symbol = source_name.GetFullName();
    source_name.SetExt("lst");
    settings.listing = source_name.GetFullName();
    return settings;
}

void AssemblyBuilderDialog::QueueZ80Commands(bool expanded_rom)
{
    // The Z80 sound driver and music banks were introduced to the disassembly
    // after v0.3: skip them for older sources that lack the audio code.
    if (!wxFileExists(m_dir + wxFileName::GetPathSeparator() + z80_build_steps[0].source))
    {
        Log("No Z80 audio sources found, skipping sound driver build.\n");
        return;
    }
    const wxString z80defines = wxString(" -D EXPANDED=") + (expanded_rom ? "1" : "0");
    for (const auto& step : z80_build_steps)
    {
        m_build_queue.push_back(z80assembler + " " + step.source + " -o " + step.object + z80defines);
        m_build_queue.push_back(z80linker + " " + wxString(step.object) + " " + step.binary + " " + step.link_opts);
    }
}

bool AssemblyBuilderDialog::DoBuild()
{
    m_build_queue.clear();
    BuildSettings settings = ResolveBuildSettings();
    m_built_rom_name = settings.target;

    if (!wxFileExists(m_dir + wxFileName::GetPathSeparator() + settings.source))
    {
        Log("Assembly file \"" + settings.source + "\" does not exist in \"" + m_dir + "\"!", *wxRED);
        return false;
    }

    QueueZ80Commands(settings.expanded);

    wxString defines;
    for (const auto& define : settings.defines)
    {
        defines << (defines.empty() ? "/e " : ";") << define.first << "=" << define.second;
    }
    wxString cmd = assembler;
    cmd << " " << settings.buildopts;
    if (!defines.empty())
    {
        cmd << " " << defines;
    }
    cmd << " \"" << settings.source << "\",\"" << settings.target << "\",\""
        << settings.symbol << "\",\"" << settings.listing << "\"";
    m_build_queue.push_back(cmd);

    return RunNextBuildCommand();
}

bool AssemblyBuilderDialog::RunNextBuildCommand()
{
    if (m_build_queue.empty())
    {
        return false;
    }
    wxString cmd = m_build_queue.front();
    m_build_queue.pop_front();

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

    Log(cmd + "\n", *wxBLUE);

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
    if (wxFileName(m_built_rom_name).Exists())
    {
        auto r = Landstalker::Rom(m_built_rom_name.ToStdString());
        r.writeFile(m_built_rom_name.ToStdString());

        Log(Landstalker::StrPrintf("Done! Checksum is 0x%04X.\n", r.read_checksum()), wxColor(0, 128, 0));
        isSuccess = true;
    }
    else
    {
        Log(wxString("Failed to read ROM file \"") + m_built_rom_name + "\" while checking checksum. ", *wxRED);
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
