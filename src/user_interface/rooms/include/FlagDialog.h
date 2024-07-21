#ifndef _FLAG_DIALOG_H_
#define _FLAG_DIALOG_H_

#include <cstdint>
#include <memory>
#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <map>
#include <landstalker/main/include/GameData.h>
#include <landstalker/rooms/include/Flags.h>
#include <user_interface/main/include/ImageList.h>
#include <user_interface/rooms/include/FlagDataViewModel.h>

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
	struct PageProperties
	{
		bool add_enabled;
		bool delete_enabled;
		bool rearrange_enabled;

		PageProperties(bool add, bool del, bool rearrange)
			: add_enabled(add), delete_enabled(del), rearrange_enabled(rearrange)
		{}
		PageProperties()
			: add_enabled(false), delete_enabled(false), rearrange_enabled(false)
		{}
	};

	void AddPage(FlagType type, const std::string& name, BaseDataViewModel* model, const FlagDialog::PageProperties& props);
	void UpdateUI();

	FlagType GetSelectedTab();

	void OnTabChange(wxBookCtrlEvent& e);
	void OnOK(wxCommandEvent& evt);
	void OnCancel(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnKeyPress(wxKeyEvent& evt);


	std::vector<FlagType> m_pages;
	std::map<FlagType, PageProperties> m_page_properties;
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
	std::map<FlagType, BaseDataViewModel*> m_models;
};

#endif // _FLAG_DIALOG_H_
