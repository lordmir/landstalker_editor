#include <script/ProgressFlagsFrame.h>
#include <script/ProgressFlagsCtrl.h>
#include <main/BrowserTreeCtrl.h>

#include <wx/propgrid/advprops.h>

enum TOOL_IDS
{
	ID_APPEND = 30000,
	ID_INSERT,
	ID_DELETE,
	ID_MOVE_UP,
	ID_MOVE_DOWN,
	ID_NEW_QUEST,
	ID_DELETE_QUEST
};

enum MENU_IDS
{
	ID_FILE_EXPORT_YML = 20000,
	ID_FILE_IMPORT_YML
};

ProgressFlagsEditorFrame::ProgressFlagsEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_editor = new ProgressFlagsEditorCtrl(this);

	// add the panes to the manager
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

ProgressFlagsEditorFrame::~ProgressFlagsEditorFrame()
{
}

bool ProgressFlagsEditorFrame::Open(int quest, int row)
{
	if (m_gd)
	{
		m_editor->Open(quest, row);
		m_quest = quest;
		m_index = row;
		m_reset_props = true;
		FireEvent(EVT_PROPERTIES_UPDATE);
		UpdateUI();
		return true;
	}
	return false;
}

void ProgressFlagsEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
}

void ProgressFlagsEditorFrame::ClearGameData()
{
	m_editor->ClearGameData();
	m_gd.reset();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
	UpdateUI();
}

void ProgressFlagsEditorFrame::UpdateUI() const
{
	auto [p, q] = m_editor->GetSelectedQuestProgress();
	EnableToolbarItem("Flags", ID_APPEND, m_gd != nullptr && m_editor->GetModel() != nullptr && m_gd->GetScriptData()->HasTables() && m_editor->IsRowSelected() && m_editor->GetModel()->GetTotalProgressInQuest(q) < 255);
	EnableToolbarItem("Flags", ID_INSERT, m_gd != nullptr && m_editor->GetModel() != nullptr && m_gd->GetScriptData()->HasTables() && m_editor->IsRowSelected() && m_editor->GetModel()->GetTotalProgressInQuest(q) < 255);
	EnableToolbarItem("Flags", ID_DELETE, m_gd != nullptr && m_editor->GetModel() != nullptr && m_gd->GetScriptData()->HasTables() && m_editor->IsRowSelected() && m_editor->GetModel()->GetRowCount() > 1);
	EnableToolbarItem("Flags", ID_MOVE_UP, m_gd != nullptr && m_editor->GetModel() != nullptr && m_gd->GetScriptData()->HasTables() && m_editor->IsRowSelected() && !m_editor->IsSelBottom());
	EnableToolbarItem("Flags", ID_MOVE_DOWN, m_gd != nullptr && m_editor->GetModel() != nullptr && m_gd->GetScriptData()->HasTables() && m_editor->IsRowSelected() && !m_editor->IsSelTop());
	EnableToolbarItem("Flags", ID_NEW_QUEST, m_gd != nullptr && m_editor->GetModel() != nullptr && m_gd->GetScriptData()->HasTables() && m_editor->GetModel()->GetTotalQuests() < 255);
	EnableToolbarItem("Flags", ID_DELETE_QUEST, m_gd != nullptr && m_editor->GetModel() != nullptr && m_gd->GetScriptData()->HasTables() && m_editor->IsRowSelected() && m_editor->IsRowSelected() && m_editor->GetModel()->GetTotalQuests() > 1);
}

void ProgressFlagsEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void ProgressFlagsEditorFrame::RefreshLists() const
{
}

void ProgressFlagsEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
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

void ProgressFlagsEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd != nullptr)
	{
		props.GetGrid()->Freeze();
		props.GetGrid()->Thaw();
	}
}

void ProgressFlagsEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
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

void ProgressFlagsEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Flag Mapping as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YML, "Import Flag Mapping from YAML...");

	wxAuiToolBar* script_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	script_tb->AddTool(ID_APPEND, "Append Entry", ilist.GetImage("append_tile"), "Append Progress Marker");
	script_tb->AddTool(ID_INSERT, "Insert Entry", ilist.GetImage("plus"), "Insert Progress Marker");
	script_tb->AddTool(ID_DELETE, "Delete Entry", ilist.GetImage("minus"), "Delete Progress Marker");
	script_tb->AddSeparator();
	script_tb->AddTool(ID_MOVE_UP, "Move Up", ilist.GetImage("up"), "Move Up");
	script_tb->AddTool(ID_MOVE_DOWN, "Move Down", ilist.GetImage("down"), "Move Down");
	script_tb->AddSeparator();
	script_tb->AddTool(ID_NEW_QUEST, "New Quest", ilist.GetImage("new"), "New Quest");
	script_tb->AddTool(ID_DELETE_QUEST, "Delete Quest", ilist.GetImage("delete"), "Delete Quest");
	AddToolbar(m_mgr, *script_tb, "Flags", "Progress Flags Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1).CloseButton(false).Movable(false).DockFixed(true));

	m_mgr.Update();
	UpdateUI();
}

void ProgressFlagsEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_YML:
		OnExportYml();
		break;
	case ID_FILE_IMPORT_YML:
		OnImportYml();
		break;
	case ID_APPEND:
		OnAppend();
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
	case ID_NEW_QUEST:
		OnNewCollection();
		break;
	case ID_DELETE_QUEST:
		OnDeleteCollection();
		break;
	}
	UpdateUI();
}

void ProgressFlagsEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void ProgressFlagsEditorFrame::OnExportYml()
{
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		wxFileDialog fd(this, _("Export Flags as YAML"), "", "progress_flags.yaml",
			"YAML file (*.yaml, *.yml)|*.yaml;*.yml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			m_editor->RefreshData();
			std::ofstream ofs(fd.GetPath().ToStdString(), std::ios::binary | std::ios::out);
			ofs << Landstalker::ProgressFlags::ToYaml(*m_gd->GetScriptData()->GetProgressFlagsFuncs());
		}
	}
}

void ProgressFlagsEditorFrame::OnImportYml()
{
	if (m_gd && m_gd->GetScriptData()->HasTables())
	{
		wxFileDialog fd(this, _("Import Flags from YAML"), "", "", "YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			try
			{
				std::ifstream ifs(fd.GetPath().ToStdString());
				std::stringstream yaml;
				yaml << ifs.rdbuf();
				m_gd->GetScriptData()->SetProgressFlagsFuncs(Landstalker::ProgressFlags::FromYaml(yaml.str()));
				FireEvent(EVT_PROPERTIES_UPDATE);
				m_editor->RefreshData();
			}
			catch (std::exception& e)
			{
				wxMessageBox(std::string("Error when parsing YAML:\n") + e.what());
			}
		}
	}
}

void ProgressFlagsEditorFrame::OnAppend()
{
	m_editor->AppendRow();
}

void ProgressFlagsEditorFrame::OnInsert()
{
	m_editor->InsertRow();
}

void ProgressFlagsEditorFrame::OnDelete()
{
	m_editor->DeleteRow();
}

void ProgressFlagsEditorFrame::OnMoveUp()
{
	m_editor->MoveRowUp();
}

void ProgressFlagsEditorFrame::OnMoveDown()
{
	m_editor->MoveRowDown();
}

void ProgressFlagsEditorFrame::OnNewCollection()
{
	m_editor->AddQuest();
}

void ProgressFlagsEditorFrame::OnDeleteCollection()
{
	int answer = wxMessageBox("Really delete quest?", "Confirm", wxYES_NO, this);
	if (answer == wxYES)
	{
		m_editor->DeleteQuest();
	}
}
