#include <script/ScriptTableTreeEditorDialog.h>

#include <script/ScriptEditorDialog.h>

#include <wx/sizer.h>

#include <landstalker/misc/Utils.h>

namespace
{
// Mirrors the entry naming AddChrMapping()/AddCsMapping() use for the main editor's entry list,
// so the popup's title reads the same as the entry it was opened from.
wxString MakeTitle(const std::shared_ptr<Landstalker::GameData>& gd, ScriptTableTreeCategory category, int entry)
{
	switch (category)
	{
	case ScriptTableTreeCategory::CHARACTER:
		// Hex, to match the character script tree and every character picker.
		return wxString(Landstalker::StrWPrintf("%ls (%03X)",
			gd->GetStringData()->GetCharacterDisplayName(entry).c_str(), entry));
	case ScriptTableTreeCategory::CUTSCENE:
		return wxString(Landstalker::StrWPrintf("Cutscene Script %03d", entry));
	case ScriptTableTreeCategory::SHOP:
		return wxString::Format("Shop %d", entry);
	case ScriptTableTreeCategory::ITEM:
		return wxString::Format("Item Script %d", entry);
	default:
		return "Script";
	}
}
}

ScriptTableTreeEditorDialog::ScriptTableTreeEditorDialog(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
	ScriptTableTreeCategory category, int entry)
	: wxDialog(parent, wxID_ANY, MakeTitle(gd, category, entry), wxDefaultPosition, wxSize(900, 650),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	// No entry list pane in the popup, so no ImageList needed (only the entry-list buttons use it).
	m_editor = new ScriptTableTreeEditorCtrl(this, nullptr, false);
	m_editor->SetGameData(gd);
	m_editor->Open(category, entry);
	sizer->Add(m_editor, 1, wxEXPAND | wxALL, 4);
	sizer->Add(CreateSeparatedButtonSizer(wxCLOSE), 0, wxEXPAND | wxALL, 4);
	SetSizer(sizer);
	// wxID_CLOSE isn't one of the IDs wxDialog ends itself for by default.
	SetAffirmativeId(wxID_CLOSE);
	SetMinSize(FromDIP(wxSize(560, 360)));
	CentreOnParent();
}

void ScriptTableTreeEditorDialog::EndModal(int retCode)
{
	// Flush any in-flight tree cell edit before the dialog (and the model the cell editor
	// belongs to) is torn down, or the typed value would be silently lost. EndModal covers every
	// dismissal path - the Close button, Esc and programmatic closes alike (none of which raise
	// wxEVT_CLOSE_WINDOW on a modal dialog).
	m_editor->CommitTreeEditing();
	wxDialog::EndModal(retCode);
}

bool ScriptEntryPopup::Open(wxWindow* parent, std::shared_ptr<Landstalker::GameData> gd,
	const ScriptTreeDataViewModel* model, const ScriptTreeNode& node)
{
	if (!gd || !gd->GetScriptData())
	{
		return false;
	}
	const auto script = gd->GetScriptData()->GetScript();
	if (!script || node.numeric_value >= script->GetScriptLineCount())
	{
		return false;
	}
	// Resolved through the model so the click target always agrees with the hyperlink styling
	// its GetAttr() applies from the same function.
	std::optional<std::pair<ScriptTreeLinkType, int>> target;
	if (model)
	{
		target = model->GetScriptEntryLinkTarget(node);
	}
	if (target)
	{
		const ScriptTableTreeCategory category = target->first == ScriptTreeLinkType::CUTSCENE
			? ScriptTableTreeCategory::CUTSCENE : ScriptTableTreeCategory::CHARACTER;
		ScriptTableTreeEditorDialog dlg(parent, gd, category, target->second);
		dlg.ShowModal();
	}
	else
	{
		ScriptEditorDialog dlg(parent, gd, node.numeric_value);
		dlg.ShowModal();
	}
	return true;
}
