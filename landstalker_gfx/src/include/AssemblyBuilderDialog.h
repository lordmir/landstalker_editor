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
        RUN,
        INJECT,
        PATCH
    };

    AssemblyBuilderDialog(wxWindow* parent, const wxString& dir, std::shared_ptr<GameData> gd, Func fn = Func::SAVE_ASM, std::shared_ptr<Rom> rom = nullptr);
    virtual ~AssemblyBuilderDialog();

private:
    void OnInit(wxInitDialogEvent& evt);
    void OnClose(wxCloseEvent& evt);
    void OnOK(wxCommandEvent& evt);
    void OnProcessComplete(wxProcessEvent& evt);
    void OnThreadInput(wxThreadEvent& evt);

    void Assemble();
    void Build();
    void Inject();
    void MakePatch();
    void Run();

    void Log(const wxString& str, const wxColor& colour = *wxBLACK);

    bool JoinThread();
    void DoClone();
    void DoSave();
    void DoBuild();
    void DoFixChecksum();
    void DoRun(const wxString& fname);
    void DoMakePatch();
    void DoSaveToRom();

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
};

#endif // _ASSEMBLY_BUILDER_DIALOG_H_
