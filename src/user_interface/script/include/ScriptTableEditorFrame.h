#ifndef _SCRIPT_TABLE_EDITOR_FRAME_H_
#define _SCRIPT_TABLE_EDITOR_FRAME_H_

#include <landstalker/main/include/GameData.h>
#include <user_interface/main/include/EditorFrame.h>
#include <user_interface/script/include/ScriptTableEditorCtrl.h>


class ScriptTableEditorFrame : public EditorFrame
{
public:

	ScriptTableEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~ScriptTableEditorFrame();

	bool Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index = 0);
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
	void OnInsert();
	void OnDelete();
	void OnMoveUp();
	void OnMoveDown();

	mutable bool m_reset_props = false;
	ScriptTableDataViewModel::Mode m_mode = ScriptTableDataViewModel::Mode::CUTSCENE;
	unsigned int m_index = 0;

	mutable wxPGChoices m_rooms;
	mutable wxPGChoices m_rooms_plus_empty;
	mutable wxPGChoices m_items;

	mutable wxAuiManager m_mgr;
	ScriptTableEditorCtrl* m_editor = nullptr;
};

#endif // _SCRIPT_TABLE_EDITOR_FRAME_H_
