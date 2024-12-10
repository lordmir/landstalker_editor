#include <user_interface/script/include/ScriptTableEditorFrame.h>
#include <user_interface/script/include/ScriptTableEditorCtrl.h>

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
	m_mgr.SetManagedWindow(this);
	m_editor = new ScriptTableEditorCtrl(this);

	// add the panes to the manager
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

ScriptTableEditorFrame::~ScriptTableEditorFrame()
{
}

bool ScriptTableEditorFrame::Open(const ScriptTableDataViewModel::Mode& mode, unsigned int index)
{
	if (m_gd)
	{
		m_editor->Open(mode, index);
		m_mode = mode;
		m_index = index;
		m_reset_props = true;
		FireEvent(EVT_PROPERTIES_UPDATE);
		UpdateUI();
		return true;
	}
	return false;
}

void ScriptTableEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
}

void ScriptTableEditorFrame::ClearGameData()
{
	m_editor->ClearGameData();
	m_gd.reset();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
	UpdateUI();
}

void ScriptTableEditorFrame::UpdateUI() const
{
	EnableToolbarItem("Script", ID_INSERT, m_gd != nullptr && m_mode != ScriptTableDataViewModel::Mode::SHOP);
	EnableToolbarItem("Script", ID_DELETE, m_gd != nullptr && m_mode != ScriptTableDataViewModel::Mode::SHOP && m_editor->IsRowSelected());
	EnableToolbarItem("Script", ID_MOVE_UP, m_gd != nullptr && m_editor->IsRowSelected() && !m_editor->IsSelTop());
	EnableToolbarItem("Script", ID_MOVE_DOWN, m_gd != nullptr && m_editor->IsRowSelected() && !m_editor->IsSelBottom());
}

void ScriptTableEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void ScriptTableEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		if (m_reset_props)
		{
			props.GetGrid()->ClearModifiedStatus();
			m_reset_props = false;
		}
		RefreshProperties(props);
	}
}

void ScriptTableEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd != nullptr)
	{
		props.GetGrid()->Freeze();
		props.GetGrid()->Thaw();
	}
}

void ScriptTableEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	auto* ctrl = static_cast<wxPropertyGridManager*>(evt.GetEventObject());
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr || m_gd == nullptr)
	{
		return;
	}
	ctrl->GetGrid()->Freeze();
	ctrl->GetGrid()->Thaw();
}

void ScriptTableEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Script as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YML, "Import Script from YAML...");

	wxAuiToolBar* script_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	script_tb->AddTool(ID_INSERT, "Insert Entry", ilist.GetImage("plus"), "Insert Entry");
	script_tb->AddTool(ID_DELETE, "Delete Entry", ilist.GetImage("minus"), "Delete Entry");
	script_tb->AddSeparator();
	script_tb->AddTool(ID_MOVE_UP, "Move Up", ilist.GetImage("up"), "Move Up");
	script_tb->AddTool(ID_MOVE_DOWN, "Move Down", ilist.GetImage("down"), "Move Down");
	AddToolbar(m_mgr, *script_tb, "Script", "Script Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1).CloseButton(false).Movable(false).DockFixed(true));

	m_mgr.Update();
	UpdateUI();
}

void ScriptTableEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_YML:
		OnExportYml();
		break;
	case ID_FILE_IMPORT_YML:
		OnImportYml();
		break;
	case ID_INSERT:
		OnInsert();
		break;
	case ID_DELETE:
		OnDelete();
		break;
	case ID_MOVE_UP:
		OnMoveUp();
		break;
	case ID_MOVE_DOWN:
		OnMoveDown();
		break;
	}
	UpdateUI();
}

void ScriptTableEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void ScriptTableEditorFrame::OnExportYml()
{
	if (m_gd)
	{
		const wxString default_file = "script.yml";
		wxFileDialog fd(this, _("Export Script as YAML"), "", default_file, "YAML file (*.yml)|*.yml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			std::wstring yaml(m_gd->GetScriptData()->GetScript()->ToYaml(m_gd));
			std::ofstream ofs(fd.GetPath().ToStdString(), std::ios::binary | std::ios::out);
			ofs << wstr_to_utf8(yaml);
		}
	}
}

void ScriptTableEditorFrame::OnImportYml()
{
	if (m_gd)
	{
		wxFileDialog fd(this, _("Import Script from YAML"), "", "", "YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			try
			{
				std::ifstream ifs(fd.GetPath().ToStdString());
				std::stringstream yaml;
				yaml << ifs.rdbuf();
				m_gd->GetScriptData()->GetScript()->FromYaml(m_gd, yaml.str());
				m_editor->RefreshData();
			}
			catch (std::exception& e)
			{
				wxMessageBox(std::string("Error when parsing YAML:\n") + e.what());
			}
		}
	}
}

void ScriptTableEditorFrame::OnInsert()
{
	m_editor->InsertRow();
}

void ScriptTableEditorFrame::OnDelete()
{
	m_editor->DeleteRow();
}

void ScriptTableEditorFrame::OnMoveUp()
{
	m_editor->MoveRowUp();
}

void ScriptTableEditorFrame::OnMoveDown()
{
	m_editor->MoveRowDown();
}
