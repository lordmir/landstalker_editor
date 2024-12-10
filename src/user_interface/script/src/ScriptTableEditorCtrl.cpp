#include <user_interface/script/include/ScriptTableEditorCtrl.h>

wxBEGIN_EVENT_TABLE(ScriptTableEditorCtrl, wxPanel)
EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, ScriptTableEditorCtrl::OnSelectionChange)
wxEND_EVENT_TABLE()

ScriptTableEditorCtrl::ScriptTableEditorCtrl(wxWindow* parent)
	: wxPanel(parent)
{
}

ScriptTableEditorCtrl::~ScriptTableEditorCtrl()
{
}

void ScriptTableEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
}

void ScriptTableEditorCtrl::ClearGameData()
{
}

void ScriptTableEditorCtrl::Open(const ScriptTableDataViewModel::Mode& mode, int index)
{
}

void ScriptTableEditorCtrl::RefreshData()
{
}

void ScriptTableEditorCtrl::InsertRow()
{
}

void ScriptTableEditorCtrl::DeleteRow()
{
}

void ScriptTableEditorCtrl::MoveRowUp()
{
}

void ScriptTableEditorCtrl::MoveRowDown()
{
}

bool ScriptTableEditorCtrl::IsRowSelected() const
{
	return false;
}

bool ScriptTableEditorCtrl::IsSelTop() const
{
	return false;
}

bool ScriptTableEditorCtrl::IsSelBottom() const
{
	return false;
}

void ScriptTableEditorCtrl::UpdateUI()
{
}

void ScriptTableEditorCtrl::OnSelectionChange(wxDataViewEvent& evt)
{
}
