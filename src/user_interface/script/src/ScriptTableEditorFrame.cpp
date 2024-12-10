#include <user_interface/script/include/ScriptTableEditorFrame.h>

enum TOOL_IDS
{
	ID_INSERT = 30000,
	ID_DELETE,
	ID_MOVE_UP,
	ID_MOVE_DOWN
};

enum MENU_IDS
{
	ID_FILE_EXPORT_YML = 20000,
	ID_FILE_IMPORT_YML
};

ScriptTableEditorFrame::ScriptTableEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
}

ScriptTableEditorFrame::~ScriptTableEditorFrame()
{
}

bool ScriptTableEditorFrame::Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index)
{
	return false;
}

void ScriptTableEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
}

void ScriptTableEditorFrame::ClearGameData()
{
}

void ScriptTableEditorFrame::UpdateUI() const
{
}

void ScriptTableEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
}

void ScriptTableEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
}

void ScriptTableEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
}

void ScriptTableEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
}

void ScriptTableEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
}

void ScriptTableEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
}

void ScriptTableEditorFrame::ClearMenu(wxMenuBar& menu) const
{
}

void ScriptTableEditorFrame::OnExportYml()
{
}

void ScriptTableEditorFrame::OnImportYml()
{
}

void ScriptTableEditorFrame::OnInsert()
{
}

void ScriptTableEditorFrame::OnDelete()
{
}

void ScriptTableEditorFrame::OnMoveUp()
{
}

void ScriptTableEditorFrame::OnMoveDown()
{
}
