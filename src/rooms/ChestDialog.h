#ifndef _CHEST_DIALOG_H_
#define _CHEST_DIALOG_H_

#include <memory>
#include <wx/wx.h>
#include <wx/dataview.h>
#include <rooms/FlagDataViewModel.h>
#include <main/ImageList.h>
#include <landstalker/main/GameData.h>

class ChestDialog : public wxDialog
{
public:
	ChestDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<Landstalker::GameData> gd);
	virtual ~ChestDialog();

	void CommitAll();
private:
	void UpdateUI();

	void OnCheck(wxCommandEvent& evt);
	void OnOK(wxCommandEvent& evt);
	void OnCancel(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	ImageList* m_imglst;
	uint16_t m_roomnum;

	wxCheckBox* m_ctrl_chests_enabled;
	wxDataViewCtrl* m_ctrl_chest_list;
	ChestFlagViewModel* m_model;
	wxStdDialogButtonSizer* m_button_sizer;
	wxButton* m_ok;
	wxButton* m_cancel;
};

#endif // _CHEST_DIALOG_H_
