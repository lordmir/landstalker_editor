#ifndef _FLAG_DIALOG_H_
#define _FLAG_DIALOG_H_

#include <cstdint>
#include <memory>
#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <ImageList.h>
#include <GameData.h>
#include <FlagDataViewModel.h>
#include <Flags.h>
#include <map>

class FlagDialog : public wxDialog
{
public:
	FlagDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd);
	virtual ~FlagDialog();

	void CommitAll();
	void AddToCurrentList();
	void DeleteFromCurrentList();
	void MoveSelectedUpCurrentList();
	void MoveSelectedDownCurrentList();
private:
	void AddPage(FlagType type, const std::string& name);
	void InitEntityVisibleFlags();
	void InitOneTimeFlags();
	void InitRoomClearFlags();
	void InitLockedDoorFlags();
	void InitPermanentSwitchFlags();
	void InitSacredTreeFlags();

	FlagType GetSelectedTab();

	void OnOK(wxCommandEvent& evt);
	void OnCancel(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnKeyPress(wxKeyEvent& evt);

	std::shared_ptr<GameData> m_gd;
	ImageList* m_imglst;
	uint16_t m_roomnum;
	std::map<FlagType, wxDataViewCtrl*> m_dvc_ctrls;
	wxNotebook* m_tabs;
	wxStdDialogButtonSizer* m_button_sizer;
	wxButton* m_ok;
	wxButton* m_cancel;
	wxBitmapButton* m_ctrl_add;
	wxBitmapButton* m_ctrl_delete;
	wxBitmapButton* m_ctrl_move_up;
	wxBitmapButton* m_ctrl_move_down;
	std::map<FlagType, BaseFlagDataViewModel*> m_models;
};

#endif // _FLAG_DIALOG_H_
