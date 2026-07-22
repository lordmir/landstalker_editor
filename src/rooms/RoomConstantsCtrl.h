#ifndef _ROOM_CONSTANTS_CTRL_H_
#define _ROOM_CONSTANTS_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/GameData.h>
#include <main/ImageList.h>
#include <rooms/RoomConstantsDataViewModel.h>

class RoomConstantsEditorCtrl : public wxPanel
{
public:
	RoomConstantsEditorCtrl(wxWindow* parent, ImageList* imglst);
	virtual ~RoomConstantsEditorCtrl();

	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	void Open(int row = -1);
	void RefreshData();
private:
	void UpdateUI();
	void OnAdd(wxCommandEvent& evt);
	void OnDelete(wxCommandEvent& evt);
	void OnSelectionChange(wxDataViewEvent& evt);

	wxDataViewCtrl* m_dvc_ctrl;
	wxBitmapButton* m_ctrl_add;
	wxBitmapButton* m_ctrl_delete;
	RoomConstantsDataViewModel* m_model;

	std::shared_ptr<Landstalker::GameData> m_gd;
};

#endif // _ROOM_CONSTANTS_CTRL_H_
