#ifndef _PREFERENCES_DIALOG_H_
#define _PREFERENCES_DIALOG_H_

#include <wx/wx.h>

class PreferencesDialog : public wxDialog
{
public:
	PreferencesDialog(wxWindow* parent);
	virtual ~PreferencesDialog();
private:
	void Init();
	void Commit();

	void OnOK(wxCommandEvent& evt);
	void OnCancel(wxCommandEvent& evt);

	wxTextCtrl* m_ctrl_clonecmd;
	wxTextCtrl* m_ctrl_cloneurl;
	wxTextCtrl* m_ctrl_clonetag;
	wxTextCtrl* m_ctrl_asmargs;
	wxTextCtrl* m_ctrl_assembler;
	wxTextCtrl* m_ctrl_baseasm;
	wxTextCtrl* m_ctrl_outname;
	wxTextCtrl* m_ctrl_emulator;
	wxCheckBox* m_ctrl_run_after_build;
	wxCheckBox* m_ctrl_build_on_save;
	wxCheckBox* m_ctrl_clone_in_new_dir;

	wxButton* m_ok;
	wxButton* m_cancel;
};

#endif // _PREFERENCES_DIALOG_H_
