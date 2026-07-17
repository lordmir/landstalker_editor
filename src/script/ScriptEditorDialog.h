#ifndef _SCRIPT_EDITOR_DIALOG_H_
#define _SCRIPT_EDITOR_DIALOG_H_

#include <memory>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>

class ScriptEditorCtrl;

// Modal popup hosting the Main Script Editor control, scoped to the contiguous script segment
// containing a given script line: from the line after the previous segment's terminator through
// this segment's own terminating line. A terminator is an entry flagged End or a cutscene
// trigger (initiating a cutscene ends the script - see Script::GetScriptAtLine()'s walk).
// Edits apply directly to the shared script data; closing the dialog just dismisses the view.
class ScriptEditorDialog : public wxDialog
{
public:
	ScriptEditorDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int script_id);

private:
	ScriptEditorCtrl* m_editor;
};

#endif // _SCRIPT_EDITOR_DIALOG_H_
