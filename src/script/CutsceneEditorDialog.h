#ifndef _CUTSCENE_EDITOR_DIALOG_H_
#define _CUTSCENE_EDITOR_DIALOG_H_

#include <memory>
#include <string>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>

class CutsceneCodeEditor;
class wxTextCtrl;

// A resizable dialog showing one dialogueactions handler - the cutscene at a given index - with the
// same editing/highlighting/linking as the Cutscenes editor. Launched from the inbound cutscene
// references (script table PLAY_CUTSCENE, script function editor, behaviour StartCutscene) so a
// cutscene index resolves to its actual handler code (see [[cutscene-two-layer-architecture]]).
class CutsceneEditorDialog : public wxDialog
{
public:
	CutsceneEditorDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int cutscene_index);

private:
	void OnOk(wxCommandEvent& evt);

	std::shared_ptr<Landstalker::GameData> m_gd;
	int m_index;
	std::string m_label; // the handler label of this slot, empty if external / unavailable
	CutsceneCodeEditor* m_code = nullptr;
	wxTextCtrl* m_name = nullptr; // editable C_CUTSCENE name for this cutscene index
};

#endif // _CUTSCENE_EDITOR_DIALOG_H_
