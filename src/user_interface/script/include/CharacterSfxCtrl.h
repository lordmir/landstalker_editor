#ifndef _CHARACTER_SFX_CTRL_H_
#define _CHARACTER_SFX_CTRL_H_

#include <memory>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/include/GameData.h>
#include <user_interface/script/include/CharacterSfxDataViewModel.h>

class CharacterSfxEditorCtrl : public wxPanel
{
public:

	CharacterSfxEditorCtrl(wxWindow* parent);
	virtual ~CharacterSfxEditorCtrl();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void Open(int chr = -1);
	void RefreshData();
private:
	void UpdateUI();

	wxDataViewCtrl* m_dvc_ctrl;
	CharacterSfxDataViewModel* m_model;

	std::shared_ptr<GameData> m_gd;
};

#endif // _CHARACTER_SFX_CTRL_H_
