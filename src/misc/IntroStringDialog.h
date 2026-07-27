#ifndef _INTRO_STRING_DIALOG_H_
#define _INTRO_STRING_DIALOG_H_

#include <memory>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>

class StringEditorFrame;

// A resizable dialog that shows the string editor focused on one intro string, launched from a
// <ShowIntroString $id> token. It embeds the whole StringEditorFrame in intro mode (which commits
// cell edits live), so OK just closes. See [[cutscene-two-layer-architecture]].
class IntroStringDialog : public wxDialog
{
public:
	IntroStringDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int index);

private:
	StringEditorFrame* m_editor = nullptr;
};

#endif // _INTRO_STRING_DIALOG_H_
