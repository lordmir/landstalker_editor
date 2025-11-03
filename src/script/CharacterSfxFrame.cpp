#include <script/CharacterSfxFrame.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_YML = 20000,
	ID_FILE_IMPORT_YML
};

CharacterSfxEditorFrame::CharacterSfxEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_editor = new CharacterSfxEditorCtrl(this);

	// add the panes to the manager
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

CharacterSfxEditorFrame::~CharacterSfxEditorFrame()
{
}

bool CharacterSfxEditorFrame::Open(int chr)
{
	if (m_gd)
	{
		m_editor->Open(chr);
		m_reset_props = true;
		FireEvent(EVT_PROPERTIES_UPDATE);
		UpdateUI();
		return true;
	}
	return false;
}

void CharacterSfxEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
}

void CharacterSfxEditorFrame::ClearGameData()
{
	m_editor->ClearGameData();
	m_gd.reset();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
	UpdateUI();
}

void CharacterSfxEditorFrame::UpdateUI() const
{
}

void CharacterSfxEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void CharacterSfxEditorFrame::RefreshLists() const
{
}

void CharacterSfxEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
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

void CharacterSfxEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd != nullptr)
	{
		props.GetGrid()->Freeze();
		props.GetGrid()->Thaw();
	}
}

void CharacterSfxEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
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

void CharacterSfxEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Sound Effects as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_IMPORT_YML, "Import Sound Effects from YAML...");
}

void CharacterSfxEditorFrame::OnMenuClick(wxMenuEvent& evt)
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

void CharacterSfxEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}

void CharacterSfxEditorFrame::OnExportYml()
{
	if (m_gd)
	{
		wxFileDialog fd(this, _("Export Flags as YAML"), "", "char_sfx.yaml",
			"YAML file (*.yaml, *.yml)|*.yaml;*.yml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			std::ofstream ofs(fd.GetPath().ToStdString(), std::ios::binary | std::ios::out);
			ofs << m_gd->GetStringData()->ExportSpecialCharTalkSfxYaml();
		}
	}
}

void CharacterSfxEditorFrame::OnImportYml()
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
				m_gd->GetStringData()->ImportSpecialCharTalkSfxYaml(yaml.str());
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
