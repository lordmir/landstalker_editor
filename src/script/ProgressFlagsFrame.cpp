#include <script/ProgressFlagsFrame.h>
#include <script/ProgressFlagsCtrl.h>
#include <main/BrowserTreeCtrl.h>

#include <wx/propgrid/advprops.h>

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
	// No frame-level toolbar items to enable/disable - ProgressFlagsEditorCtrl's own
	// UpdateButtonStates() drives its embedded buttons' enabled state directly.
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

void ProgressFlagsEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	// No frame-level toolbar - ProgressFlagsEditorCtrl's own embedded Append/Insert/Delete/Move Up/
	// Down/New Quest/Delete Quest buttons already cover these actions directly on the dataview they
	// act on.
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Flag Mapping as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YML, "Import Flag Mapping from YAML...");

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

