#ifndef _ITEM_USE_EDITOR_FRAME_H_
#define _ITEM_USE_EDITOR_FRAME_H_

#include <string>
#include <vector>

#include <wx/aui/aui.h>

#include <landstalker/main/GameData.h>
#include <main/EditorFrame.h>

class wxListCtrl;
class wxListEvent;
class wxButton;
class CutsceneCodeEditor;

// Global editor for the item pre-use / post-use handlers (itemuse1/2.asm + itempostuse.asm, dispatched
// by item id through the pre/post-use tables). Each row is one item's pre-use or post-use handler; the
// right pane edits that handler's raw m68k in a CutsceneCodeEditor. Add binds a new (stub) handler to
// an item; Remove unbinds it. Reachable only under Assembly (no other UI linkage).
class ItemUseEditorFrame : public EditorFrame
{
public:
	ItemUseEditorFrame(wxWindow* parent, ImageList* imglst);
	virtual ~ItemUseEditorFrame();

	bool Open();
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	virtual void ClearGameData();
	virtual void CommitPendingEdits();

private:
	struct Row { bool pre; int item; std::string handler; };

	void BuildUI();
	void RefreshList();
	void ShowRow(long row);
	void CommitCurrentEdit();
	void UpdateButtons();
	void OnAdd(bool pre);
	void OnRemove();
	long SelectedRow() const;
	void OnRowSelected(wxListEvent& evt);

	wxAuiManager m_mgr;
	wxListCtrl* m_list = nullptr;
	CutsceneCodeEditor* m_code = nullptr;
	wxButton* m_add_pre = nullptr;
	wxButton* m_add_post = nullptr;
	wxButton* m_remove = nullptr;

	std::vector<Row> m_rows;
	long m_current_row = -1;
	std::string m_current_handler;
};

#endif // _ITEM_USE_EDITOR_FRAME_H_
