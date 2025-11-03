#include <behaviours/BehaviourScriptEditorFrame.h>
#include <landstalker/behaviours/BehaviourYamlConverter.h>

#include <filesystem>
#include <wx/wx.h>

using namespace Landstalker;

enum MENU_IDS
{
	ID_FILE_EXPORT_YML = 20000,
	ID_FILE_EXPORT_ALL_YML,
	ID_FILE_IMPORT_YML,
	ID_FILE_IMPORT_ALL_YML
};

BehaviourScriptEditorFrame::BehaviourScriptEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_editor = new BehaviourScriptEditorCtrl(this);

	// add the panes to the manager
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

BehaviourScriptEditorFrame::~BehaviourScriptEditorFrame()
{
}

bool BehaviourScriptEditorFrame::Open(int script_id)
{
	if (m_gd && script_id >= 0)
	{
		m_script_id = script_id;
		m_editor->Open(m_script_id);
		Update();
		return true;
	}
	return false;
}

void BehaviourScriptEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
}

void BehaviourScriptEditorFrame::ClearGameData()
{
	m_script_id = -1;
	m_editor->ClearGameData();
	m_gd.reset();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void BehaviourScriptEditorFrame::OnSaveAsYaml()
{
	if (m_gd)
	{
		const wxString default_file = StrPrintf("behaviour%d.yaml", m_script_id);
		wxFileDialog fd(this, _("Export Script as YAML"), "", default_file, "YAML file (*.yml)|*.yml|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			SaveAsYaml(fd.GetPath().ToStdString(), m_script_id);
		}
	}
}

void BehaviourScriptEditorFrame::OnSaveAllAsYaml()
{
	if (m_gd)
	{
		wxDirDialog dd(this, "Select YAML Export Directory");
		if (dd.ShowModal() != wxID_CANCEL)
		{
			SaveAllAsYaml(dd.GetPath().ToStdString());
		}
	}
}

void BehaviourScriptEditorFrame::OnLoadFromYaml()
{
	if (m_gd)
	{
		wxFileDialog fd(this, _("Import Script from YAML"), "", "", "YAML Files (*.yml, *.yaml)|*.yml;*.yaml|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fd.ShowModal() != wxID_CANCEL)
		{
			try
			{
				int script_id = LoadFromYaml(fd.GetPath().ToStdString());
				if (script_id >= 0)
				{
					Open(script_id);
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(std::string("Error when parsing YAML:\n") + e.what());
			}
		}
	}
}

void BehaviourScriptEditorFrame::OnLoadAllFromYaml()
{
	if (m_gd)
	{
		wxDirDialog dd(this, "Select YAML Import Directory");
		if (dd.ShowModal() != wxID_CANCEL)
		{
			LoadAllFromYaml(dd.GetPath().ToStdString());
		}
		Open(m_script_id);
	}
}

bool BehaviourScriptEditorFrame::SaveAsYaml(const std::string& filename, int behaviour_id)
{
	std::string yaml(BehaviourYamlConverter::ToYaml(behaviour_id,
		wstr_to_utf8(m_gd->GetSpriteData()->GetBehaviourDisplayName(behaviour_id)),
		m_gd->GetSpriteData()->GetScript(behaviour_id).second));
	std::ofstream ofs(filename, std::ios::binary | std::ios::out);
	ofs << yaml;
	return true;
}

bool BehaviourScriptEditorFrame::SaveAllAsYaml(const std::string& dirname)
{
	std::filesystem::path dir(dirname);
	const std::map<int, std::string> scripts = m_gd->GetSpriteData()->GetScriptNames();
	for (const auto& script : scripts)
	{
		SaveAsYaml((dir / StrPrintf("behaviour%d.yaml", script.first)).string(), script.first);
	}
	return true;
}

int BehaviourScriptEditorFrame::LoadFromYaml(const std::string& filename)
{
	std::ifstream ifs(filename);
	std::stringstream yaml;
	yaml << ifs.rdbuf();
	int behaviour_id;
	std::string behaviour_name;
	auto result = BehaviourYamlConverter::FromYaml(yaml.str(), behaviour_id, behaviour_name);
	if (m_gd && behaviour_id >= 0)
	{
		m_gd->GetSpriteData()->SetScript(behaviour_id, StrPrintf("behaviour%d", behaviour_id), result);
		Labels::Update(Labels::C_BEHAVIOURS, behaviour_id, utf8_to_wstr(behaviour_name));
		return behaviour_id;
	}
	return -1;
}

bool BehaviourScriptEditorFrame::LoadAllFromYaml(const std::string& dirname)
{
	std::filesystem::path dir(dirname);
	for (const auto& path : std::filesystem::directory_iterator{ dir })
	{
		if (path.is_regular_file())
		{
			const std::string extension = str_to_lower(path.path().extension().string());
			if (extension == ".yml" || extension == ".yaml")
			{
				LoadFromYaml(path.path().string());
			}
		}
	}
	return true;
}

void BehaviourScriptEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && m_script_id != -1 && ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void BehaviourScriptEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
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

void BehaviourScriptEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd != nullptr && m_script_id != -1)
	{
		props.GetGrid()->Freeze();
		props.GetGrid()->Thaw();
	}
}

void BehaviourScriptEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	auto* ctrl = static_cast<wxPropertyGridManager*>(evt.GetEventObject());
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr || m_gd == nullptr || m_script_id == -1)
	{
		return;
	}
	ctrl->GetGrid()->Freeze();
	ctrl->GetGrid()->Thaw();
}

void BehaviourScriptEditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_YML, "Export Script as YAML...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_ALL_YML, "Export All Scripts as YAML...");
	AddMenuItem(fileMenu, 2, ID_FILE_IMPORT_YML, "Import Script from YAML...");
	AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_ALL_YML, "Import All Scripts from YAML...");

	m_mgr.Update();
	UpdateUI();
}

void BehaviourScriptEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_YML:
		OnSaveAsYaml();
		break;
	case ID_FILE_EXPORT_ALL_YML:
		OnSaveAllAsYaml();
		break;
	case ID_FILE_IMPORT_YML:
		OnLoadFromYaml();
		break;
	case ID_FILE_IMPORT_ALL_YML:
		OnLoadAllFromYaml();
		break;
	default:
		break;
	}
}

void BehaviourScriptEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
}
