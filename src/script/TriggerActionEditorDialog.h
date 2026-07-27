#ifndef _TRIGGER_ACTION_EDITOR_DIALOG_H_
#define _TRIGGER_ACTION_EDITOR_DIALOG_H_

#include <memory>
#include <string>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>

class CutsceneCodeEditor;
class wxTextCtrl;

// A resizable dialog showing one triggeractions handler - the trigger action at a given index -
// with the same editing/highlighting/linking as the cutscene handler editor (it reuses
// CutsceneCodeEditor, since the handler bodies share the same call idioms). Launched from a
// behaviour WaitForCondition command, whose Condition operand is the trigger index. See
// [[cutscene-two-layer-architecture]].
class TriggerActionEditorDialog : public wxDialog
{
public:
	TriggerActionEditorDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int trigger_index);

private:
	void OnOk(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	int m_index;
	std::string m_label; // the handler label of this slot, empty if external / unavailable
	CutsceneCodeEditor* m_code = nullptr;
	wxTextCtrl* m_name = nullptr; // editable C_TRIGGER name for this trigger index
};

#endif // _TRIGGER_ACTION_EDITOR_DIALOG_H_
