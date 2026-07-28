#include <script/ScriptEditorDialog.h>
#include <misc/MovableModalDialog.h>
#include <script/ScriptEditorCtrl.h>

#include <algorithm>

#include <wx/sizer.h>

#include <landstalker/script/ScriptTableEntry.h>

namespace
{
bool IsSegmentTerminator(const Landstalker::ScriptTableEntry& entry)
{
	return entry.GetEnd() || entry.GetType() == Landstalker::ScriptTableEntryType::PLAY_CUTSCENE;
}
}

ScriptEditorDialog::ScriptEditorDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd, int script_id)
	: wxDialog(parent, wxID_ANY, "Script", wxDefaultPosition, wxSize(1000, 600),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
{
	MakeModalDialogMovable(this);

	const auto script = gd->GetScriptData()->GetScript();
	const int line_count = static_cast<int>(script->GetScriptLineCount());
	int start = 0;
	int count = 0;
	if (line_count > 0)
	{
		start = std::clamp(script_id, 0, line_count - 1);
		int end = start;
		while (start > 0 && !IsSegmentTerminator(script->GetScriptLine(start - 1)))
		{
			--start;
		}
		while (end < line_count - 1 && !IsSegmentTerminator(script->GetScriptLine(end)))
		{
			++end;
		}
		count = end - start + 1;
		SetTitle(wxString::Format("Script %04X - %04X", start, end));
	}

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	m_editor = new ScriptEditorCtrl(this);
	m_editor->SetGameData(gd, start, count);
	sizer->Add(m_editor, 1, wxEXPAND | wxALL, 4);
	sizer->Add(CreateSeparatedButtonSizer(wxCLOSE), 0, wxEXPAND | wxALL, 4);
	SetSizer(sizer);
	// wxID_CLOSE isn't one of the IDs wxDialog ends itself for by default.
	SetAffirmativeId(wxID_CLOSE);
	SetMinSize(FromDIP(wxSize(640, 360)));
	CentreOnParent();
}
