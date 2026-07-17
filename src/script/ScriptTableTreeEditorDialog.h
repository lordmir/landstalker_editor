#ifndef _SCRIPT_TABLE_TREE_EDITOR_DIALOG_H_
#define _SCRIPT_TABLE_TREE_EDITOR_DIALOG_H_

#include <memory>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>
#include <script/ScriptTableTreeEditorCtrl.h>

// Modal popup hosting the Script Function Tree editor for one specific entry of a category
// (e.g. a single cutscene's or character's script tree), without the entry-list pane. Opened by
// following a cutscene/character hyperlink from a script preview row. Edits sync straight into
// the shared function tables, exactly as in the main Script Function Editor.
class ScriptTableTreeEditorDialog : public wxDialog
{
public:
	ScriptTableTreeEditorDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
		ScriptTableTreeCategory category, int entry);

	// Commits any in-flight tree cell edit before dismissing - see the implementation comment.
	virtual void EndModal(int retCode) override;

private:
	ScriptTableTreeEditorCtrl* m_editor;
};

// Opens the popup a double-clicked script preview row (SCRIPT_ENTRY node) leads to: the linked
// cutscene/character's own script tree (ScriptTableTreeEditorDialog) when the row resolves to a
// hyperlink through `model`, otherwise the segment script editor (ScriptEditorDialog) for the
// row's script line. Shared by every view that displays a script tree (the Script Function
// Editor and the Entity Properties dialog). Returns true if a popup was shown and dismissed -
// both popups edit shared data, so callers should rebuild/refresh their own view afterwards.
namespace ScriptEntryPopup
{
	bool Open(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
		const ScriptTreeDataViewModel* model, const ScriptTreeNode& node);
}

#endif // _SCRIPT_TABLE_TREE_EDITOR_DIALOG_H_
