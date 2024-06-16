#ifndef _TILE_SWAP_DIALOG_H_
#define _TILE_SWAP_DIALOG_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <ImageList.h>
#include <GameData.h>
#include <TileSwapDataViewModel.h>
#include <DoorDataViewModel.h>

class TileSwapDialog: public wxDialog
{
public:
	TileSwapDialog(wxWindow* parent, ImageList* imglst, uint16_t room, std::shared_ptr<GameData> gd);
	virtual ~TileSwapDialog();

	void CommitAll();
	void AddToCurrentList();
	void DeleteFromCurrentList();
	void MoveSelectedUpCurrentList();
	void MoveSelectedDownCurrentList();

private:
	enum class PageType
	{
		SWAPS,
		DOORS
	};

	void AddPage(const PageType type, const std::string& name, BaseDataViewModel* model);

	void UpdateUI();

	PageType GetSelectedTab() const;

	void OnTabChange(wxBookCtrlEvent& e);
	void OnOK(wxCommandEvent& evt);
	void OnCancel(wxCommandEvent& evt);
	void OnAdd(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
	void OnMoveUp(wxCommandEvent& evt);
	void OnMoveDown(wxCommandEvent& evt);
	void OnKeyPress(wxKeyEvent& evt);

	std::shared_ptr<GameData> m_gd;
	std::map<PageType, wxDataViewCtrl*> m_dvc_ctrls;
	std::vector<PageType> m_pages;
	std::map<PageType, BaseDataViewModel*> m_models;
	ImageList* m_imglst;
	uint16_t m_roomnum;
	wxNotebook* m_tabs;
	wxStdDialogButtonSizer* m_button_sizer;
	wxButton* m_ok;
	wxButton* m_cancel;
	wxBitmapButton* m_ctrl_add;
	wxBitmapButton* m_ctrl_delete;
	wxBitmapButton* m_ctrl_move_up;
	wxBitmapButton* m_ctrl_move_down;
};

#endif // _TILE_SWAP_DIALOG_H_
