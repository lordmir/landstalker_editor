#ifndef _PREFERENCES_DIALOG_H_
#define _PREFERENCES_DIALOG_H_

#include <map>
#include <wx/wx.h>
#include <wx/config.h>

// Build toolchain and output settings. Kept in this legacy filename to avoid
// disrupting existing project layouts; the user-facing name is Build Options.
class BuildOptionsDialog : public wxDialog
{
public:
	BuildOptionsDialog(wxWindow* parent, wxConfig* config);
	virtual ~BuildOptionsDialog();
private:
	// Settings stored once per build variant (combined region and expanded
	// state, e.g. "US_EXPANDED").
	struct VariantSettings
	{
		wxString asmargs;
		wxString defines;
		wxString outname;
	};

	void Init();
	void Commit();

	wxString SelectedVariant() const;
	VariantSettings LoadVariantSettings(const wxString& variant) const;
	void ShowVariantSettings(const wxString& region, bool expanded);
	void StoreDisplayedVariantSettings();

	void OnVariantChange(wxCommandEvent& evt);
	void OnResetDefaults(wxCommandEvent& evt);
	void OnOK(wxCommandEvent& evt);
	void OnCancel(wxCommandEvent& evt);

	wxTextCtrl* m_ctrl_clonecmd;
	wxTextCtrl* m_ctrl_cloneurl;
	wxTextCtrl* m_ctrl_clonetag;
	wxChoice* m_ctrl_region;
	wxCheckBox* m_ctrl_expanded;
	wxTextCtrl* m_ctrl_asmargs;
	wxTextCtrl* m_ctrl_defines;
	wxTextCtrl* m_ctrl_assembler;
	wxTextCtrl* m_ctrl_z80assembler;
	wxTextCtrl* m_ctrl_z80linker;
	wxTextCtrl* m_ctrl_outname;
	wxTextCtrl* m_ctrl_emulator;
	wxCheckBox* m_ctrl_run_after_build;
	wxCheckBox* m_ctrl_build_on_save;
	wxCheckBox* m_ctrl_clone_in_new_dir;

	wxButton* m_reset;
	wxButton* m_ok;
	wxButton* m_cancel;

	wxConfig* m_config;
	// Edits to each build variant's settings, committed together on OK.
	std::map<wxString, VariantSettings> m_variant_settings;
	wxString m_displayed_variant;
};

#endif // _PREFERENCES_DIALOG_H_
