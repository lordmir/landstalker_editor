#ifndef _ASSEMBLY_BUILDER_DIALOG_H_
#define _ASSEMBLY_BUILDER_DIALOG_H_

#include <memory>
#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/filepicker.h>
#include <wx/msgqueue.h>
#include <wx/thread.h>
#include <wx/process.h>
#include <wx/config.h>
#include <GameData.h>
#include <ExecutorThread.h>

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

    AssemblyBuilderDialog(wxWindow* parent, const wxString& dir, std::shared_ptr<GameData> gd, Func fn = Func::SAVE_ASM, std::shared_ptr<Rom> rom = nullptr);
    virtual ~AssemblyBuilderDialog();

    wxString GetBuiltRomName();

private:
    void InitConfig();
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
    std::shared_ptr<GameData> m_gd;
    wxString m_dir;
    std::shared_ptr<Rom> m_rom;
    Func m_fn;
    wxConfig* m_config;

    wxString m_clonecmd;
    wxString m_cloneurl;
    wxString m_clonetag;
    wxString m_asmargs;
    wxString m_assembler;
    wxString m_baseasm;
    wxString m_outname;
    wxString m_emulator;
    bool m_run_after_build;
    bool m_build_on_save;
    bool m_clone_in_new_dir;
};

#endif // _ASSEMBLY_BUILDER_DIALOG_H_
