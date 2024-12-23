#ifndef _PROGRESS_FLAGS_FRAME_H_
#define _PROGRESS_FLAGS_FRAME_H_

#include <landstalker/main/include/GameData.h>
#include <user_interface/main/include/EditorFrame.h>
#include <user_interface/script/include/ProgressFlagsCtrl.h>


class ProgressFlagsEditorFrame : public EditorFrame
{
public:

	ProgressFlagsEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~ProgressFlagsEditorFrame();

	bool Open(int index = -1, int row = -1);
	virtual void SetGameData(std::shared_ptr<GameData> gd);
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
	void OnAppend();
	void OnInsert();
	void OnDelete();
	void OnMoveUp();
	void OnMoveDown();
	void OnNewCollection();
	void OnDeleteCollection();

	mutable bool m_reset_props = false;
	int m_quest = -1;
	int m_index = -1;

	mutable wxPGChoices m_rooms;
	mutable wxPGChoices m_rooms_plus_empty;
	mutable wxPGChoices m_items;

	mutable wxAuiManager m_mgr;
	ProgressFlagsEditorCtrl* m_editor = nullptr;
};

#endif // _PROGRESS_FLAGS_FRAME_H_
