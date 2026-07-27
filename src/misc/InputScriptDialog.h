#ifndef _INPUT_SCRIPT_DIALOG_H_
#define _INPUT_SCRIPT_DIALOG_H_

#include <memory>

#include <wx/dialog.h>

#include <landstalker/main/GameData.h>

class InputTableFrame;

// A resizable dialog that shows the scripted-input playback editor focused on one input sequence,
// launched from a <Playback $id> token or a PlaybackInput behaviour command. It embeds the whole
// InputTableFrame (which has live-commit semantics), so OK just flushes any trailing edit and
// closes. See [[cutscene-two-layer-architecture]].
class InputScriptDialog : public wxDialog
{
public:
	InputScriptDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int sequence);

private:
	void OnOk(wxCommandEvent& evt);

	InputTableFrame* m_editor = nullptr;
};

#endif // _INPUT_SCRIPT_DIALOG_H_
