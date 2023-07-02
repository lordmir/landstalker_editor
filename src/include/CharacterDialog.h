#ifndef _CHARACTER_DIALOG_H_
#define _CHARACTER_DIALOG_H_

#include <memory>
#include <wx/wx.h>
#include <wx/dataview.h>
#include <FlagDataViewModel.h>
#include <GameData.h>
#include <ImageList.h>

class CharacterDialog : public wxDialog
{
public:
	CharacterDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd);
	virtual ~CharacterDialog();

	void CommitAll();
private:
	void UpdateUI();
	void AddRow();
	void DeleteRow();

	void OnOK(wxCommandEvent& evt);
	void OnCancel(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
	void OnKeyPress(wxKeyEvent& evt);

	std::shared_ptr<GameData> m_gd;
	ImageList* m_imglst;
	uint16_t m_roomnum;

	wxDataViewCtrl* m_ctrl_list;
	RoomDialogueFlagViewModel* m_model;
	wxStdDialogButtonSizer* m_button_sizer;
	wxButton* m_ok;
	wxButton* m_cancel;
	wxBitmapButton* m_add;
	wxBitmapButton* m_delete;
};

#endif // _CHARACTER_DIALOG_H_
