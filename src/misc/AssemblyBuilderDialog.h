#ifndef _ASSEMBLY_BUILDER_DIALOG_H_
#define _ASSEMBLY_BUILDER_DIALOG_H_

#include <deque>
#include <memory>
#include <utility>
#include <vector>
#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/filepicker.h>
#include <wx/msgqueue.h>
#include <wx/thread.h>
#include <wx/process.h>
#include <wx/config.h>
#include <landstalker/main/GameData.h>
#include <misc/ExecutorThread.h>

class AssemblyBuilderDialog : public wxDialog
{
public:

    enum class Step
    {
        IDLE,
        CLONE,
        BUILD
    };
    enum class Func
    {
        SAVE_ASM,
        BUILD,
        RBUILD,
        RUN,
        INJECT
    };

    // Built-in defaults for every configurable build setting, used for
    // initial configuration and the preferences dialog's reset button.
    struct ConfigDefaults
    {
        wxString clonecmd;
        wxString cloneurl;
        wxString clonetag;
        wxString assembler;
        wxString asmargs;
        wxString z80assembler;
        wxString z80linker;
        wxString outname;
        wxString emulator;
        bool run_after_build;
        bool build_on_save;
        bool clone_in_new_dir;
    };

    AssemblyBuilderDialog(wxWindow* parent, const wxString& dir, std::shared_ptr<Landstalker::GameData> gd,
        Func fn = Func::SAVE_ASM, std::shared_ptr<Landstalker::Rom> rom = nullptr);
    virtual ~AssemblyBuilderDialog();

    wxString GetBuiltRomName();
    bool DidOperationSucceed();

    static const wxArrayString& GetRegions();
    static const ConfigDefaults& GetDefaults();

    // The build region and expanded state are properties of the opened
    // project, not stored preferences: they are read from the "REGION" and
    // "BUILDOPTS" (EXPANDED=n) comments in the top-level assembly file,
    // defaulting to a non-expanded US build. Together they name the build
    // variant (e.g. "US_EXPANDED") that keys the stored build settings.
    static void SetProjectRegionFromAsm(const wxString& asm_path);
    static void SetProjectRegion(const wxString& new_region);
    static wxString GetProjectRegion();
    static bool GetProjectExpanded();
    static wxString BuildName(const wxString& build_region, bool build_expanded);
    // The assembler options for a build variant from the opened assembly's
    // build.yaml, or the standard options if unavailable.
    static wxString GetDefaultBuildOpts(const wxString& build_name);
    // The preprocessor defines for a build variant from the opened assembly's
    // build.yaml (or the built-in defaults), as a "NAME=VALUE;..." string.
    static wxString GetDefaultDefines(const wxString& build_name);

    static void InitConfig(wxConfig* config);
private:
    // Parameters for one ROM build, resolved from the project region, the
    // region's stored preferences and the assembly's build.yaml (with
    // built-in fallbacks).
    struct BuildSettings
    {
        wxString source;
        wxString target;
        wxString symbol;
        wxString listing;
        wxString buildopts;
        bool expanded = false;
        std::vector<std::pair<wxString, wxString>> defines;
    };

    static void InitConfigVar(wxConfig* cfg, const wxString& path, wxString& var, const wxString& defval);
    static void InitConfigVar(wxConfig* cfg, const wxString& path, bool& var, bool defval);
    static std::vector<std::pair<wxString, wxString>> BuiltInDefines(const wxString& build_region, bool build_expanded);
    static std::vector<std::pair<wxString, wxString>> ParseDefines(const wxString& defines);
    static wxString FormatDefines(const std::vector<std::pair<wxString, wxString>>& defines);
    void OnInit();
    void OnClose(wxCloseEvent& evt);
    void OnOK(wxCommandEvent& evt);
    void OnProcessComplete(wxProcessEvent& evt);
    void OnThreadInput(wxThreadEvent& evt);

    bool Assemble(bool post_save);
    bool Build(bool post_save);
    bool Inject(bool post_save);
    bool Run();

    void Log(const wxString& str, const wxColor& colour = *wxBLACK);

    bool JoinThread();
    void Abandon();

    BuildSettings ResolveBuildSettings();
    void QueueZ80Commands(bool expanded_rom);
    bool RunNextBuildCommand();

    bool DoClone();
    bool DoSave();
    bool DoBuild();
    bool DoFixChecksum();
    bool DoRun(const wxString& fname, bool post_build);
    bool DoSaveToRom();

    void MakeBusy();
    void MakeIdle();

    wxThread* m_execThread;
    wxMessageQueue<ExecutorThread::ThreadMessage> m_msgQueue;
    wxTextCtrl* m_logctrl;
    wxButton* m_ok;
    Step m_step;
    std::shared_ptr<Landstalker::GameData> m_gd;
    wxString m_dir;
    std::shared_ptr<Landstalker::Rom> m_rom;
    Func m_fn;
    wxConfig* m_config;
    bool m_operation_succeeded;
    std::deque<wxString> m_build_queue;
    wxString m_built_rom_name;

    static wxString clonecmd;
    static wxString cloneurl;
    static wxString clonetag;
    static wxString assembler;
    static wxString z80assembler;
    static wxString z80linker;
    static wxString emulator;
    static bool run_after_build;
    static bool build_on_save;
    static bool clone_in_new_dir;
    static wxString project_region;
    static bool project_expanded;
    static wxString project_dir;
    static wxConfig* config_store;
};

#endif // _ASSEMBLY_BUILDER_DIALOG_H_
