#ifndef _CHARACTER_SFX_FRAME_H_
#define _CHARACTER_SFX_FRAME_H_

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>
#include <script/CharacterSfxCtrl.h>


class CharacterSfxEditorFrame : public EditorFrame
{
public:
	CharacterSfxEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~CharacterSfxEditorFrame();

	bool Open(int chr = -1);
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();

	void UpdateUI() const;
private:
	virtual void InitProperties(wxPropertyGridManager& props) const;
	void RefreshLists() const;
	virtual void UpdateProperties(wxPropertyGridManager& props) const;
	void RefreshProperties(wxPropertyGridManager& props) const;
	virtual void OnPropertyChange(wxPropertyGridEvent& evt);
	virtual void InitMenu(wxMenuBar& menu, ImageList& ilist) const;
	virtual void OnMenuClick(wxMenuEvent& evt);
	virtual void ClearMenu(wxMenuBar& menu) const;

	void OnExportYml();
	void OnImportYml();

	mutable bool m_reset_props = false;
	mutable wxAuiManager m_mgr;
	CharacterSfxEditorCtrl* m_editor = nullptr;
};


#endif // _CHARACTER_SFX_FRAME_H_
