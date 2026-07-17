#ifndef _SCRIPT_EDITOR_CTRL_H_
#define _SCRIPT_EDITOR_CTRL_H_

#include <memory>
#include <optional>

#include <wx/wx.h>
#include <wx/dataview.h>

#include <landstalker/main/GameData.h>
#include <landstalker/script/ScriptTableEntry.h>
#include <script/ScriptDataViewModel.h>
#include <script/ScriptEntryLink.h>

class ScriptEditorCtrl : public wxPanel
{
public:
	ScriptEditorCtrl(wxWindow* parent);
	virtual ~ScriptEditorCtrl();

	// start/count scope the editor to one contiguous script segment (see ScriptDataViewModel's
	// windowed constructor); the defaults show the whole script table.
	virtual void SetGameData(std::shared_ptr<Landstalker::GameData> gd, int start = -1, int count = -1);
	virtual void ClearGameData();

	void Open(int row = -1);
	void RefreshData();

	void AppendRow();
	void InsertRow();
	void DeleteRow();
	void MoveRowUp();
	void MoveRowDown();

	bool IsRowSelected() const;
	bool IsSelTop() const;
	bool IsSelBottom() const;

private:
	void UpdateUI();
	void UpdateButtonStates();

	void OnSelectionChange(wxDataViewEvent& evt);
	void OnContextMenu(wxDataViewEvent& evt);

	// Hyperlink support: cutscene/character names in the Value column are links to that entry's
	// own script tree popup - single click follows them, hovering shows a hand cursor.
	// Applies/clears the hand cursor (only on transitions, not every motion event).
	void SetHandCursor(bool over_link);
	// Opens the linked entry's script-tree popup (deferred via CallAfter - see the .cpp comment).
	void OpenLinkPopup(const ScriptEntryLink::Target& link);
	// The link target for the given view row when cell_pos (cell-relative) falls within the
	// renderer's recorded hyperlink hit rect - the platform-independent core of the hit test.
	std::optional<ScriptEntryLink::Target> ResolveLinkForCell(int row, const wxPoint& cell_pos) const;
#ifdef __WXGTK__
	// On GTK the native treeview consumes left-clicks before wx can raise wxEVT_LEFT_DOWN, and
	// wx motion events arrive in bin-window coordinates that don't match GetItemRect()'s widget
	// coordinates - so mouse input is intercepted at the GtkTreeView signal level instead (in
	// bin-window coordinates, which gtk_tree_view_get_path_at_pos() resolves natively).
	std::optional<ScriptEntryLink::Target> ResolveLinkAtBinPos(int x, int y) const;
	bool OnTreeViewLeftDown(int x, int y);
	void OnTreeViewMotion(int x, int y);
#else
	// Non-GTK: wx-level mouse events on the dataview's main window, whose coordinate space is
	// consistent with HitTest()/GetItemRect() there.
	void OnMouseMove(wxMouseEvent& evt);
	void OnLeftDown(wxMouseEvent& evt);
	// The link target under the given main-window position, if it's over a hyperlinked name.
	std::optional<ScriptEntryLink::Target> ResolveLinkAt(const wxPoint& pos) const;
#endif

	// Row-index-parameterized operations backing the context menu, which acts on whichever row was
	// right-clicked rather than the current selection (right-click doesn't necessarily change it).
	void AddRowAt(int row, bool below, Landstalker::ScriptTableEntryType type);
	void DeleteRowAt(int row);
	void MoveRowAt(int row, int direction);
	void EditRowAt(int row);
	void ChangeRowTypeAt(int row, Landstalker::ScriptTableEntryType type);

	int RowFromItem(const wxDataViewItem& item) const;
	wxDataViewItem ItemFromRow(int row) const;

	wxDataViewCtrl* m_dvc_ctrl;
	ScriptDataViewModel* m_model;

	wxButton* m_append_button = nullptr;
	wxButton* m_insert_button = nullptr;
	wxButton* m_delete_button = nullptr;
	wxButton* m_move_up_button = nullptr;
	wxButton* m_move_down_button = nullptr;

	std::shared_ptr<Landstalker::GameData> m_gd;

	// Tracks whether the hand cursor is currently applied, so mouse-move only swaps the
	// dataview's cursor on transitions rather than on every motion event.
	bool m_hand_cursor_active = false;

	wxDECLARE_EVENT_TABLE();
};

#endif // _BEHAVIOUR_SCRIPT_EDITOR_CTRL_H_
