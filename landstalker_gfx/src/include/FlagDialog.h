#ifndef _FLAG_DIALOG_H_
#define _FLAG_DIALOG_H_

#include <cstdint>
#include <memory>
#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <GameData.h>
#include <FlagDataViewModel.h>

class FlagDialog : public wxDialog
{
public:
	FlagDialog(wxWindow* parent, uint16_t room, std::shared_ptr<GameData> gd);
	virtual ~FlagDialog();
private:
	void InitSpriteVisibleFlags();

	std::shared_ptr<GameData> m_gd;
	uint16_t m_roomnum;
	wxDataViewCtrl* m_dvc_sprite_visible;
	wxNotebook* m_tabs;
	wxStdDialogButtonSizer* m_button_sizer;
	wxButton* m_ok;
	wxButton* m_cancel;
	EntityVisiblityFlagDataViewModel* m_model;
};

#endif // _FLAG_DIALOG_H_
