#include <behaviours/BehaviourScriptEditorFrame.h>
#include <landstalker/behaviours/BehaviourYamlConverter.h>

#include <algorithm>
#include <filesystem>
#include <wx/wx.h>
#include <wx/textdlg.h>
#include <wx/hyperlink.h>
#include <landstalker/misc/Labels.h>

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

	// Left pane: the behaviour script list - same layout as the entity editor's entity list.
	wxPanel* left = new wxPanel(this, wxID_ANY);
	wxBoxSizer* lv = new wxBoxSizer(wxVERTICAL);
	m_script_list = new wxListBox(left, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	m_script_list->SetToolTip("Behaviour scripts in id order.");
	lv->Add(m_script_list, 1, wxEXPAND | wxALL, 3);
	m_rename = new wxButton(left, wxID_ANY, "Rename...");
	lv->Add(m_rename, 0, wxEXPAND | wxALL, 3);
	left->SetSizer(lv);
	m_script_list->Bind(wxEVT_LISTBOX, [this](wxCommandEvent&) { OnScriptSelected(); });
	m_rename->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnRenameScript(); });

	// Bottom pane: the "Used By" list - a scrolled panel of clickable room hyperlinks, rebuilt
	// per script (see RefreshUsage()).
	m_usage_pane = new wxScrolledWindow(this, wxID_ANY);
	m_usage_pane->SetScrollRate(0, 8);
	m_usage_pane->SetSizer(new wxBoxSizer(wxVERTICAL));

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.5, 0.5);
	m_mgr.AddPane(left, wxAuiPaneInfo().Left().Caption("Scripts").MinSize(wxSize(200, -1))
		.BestSize(wxSize(240, -1)).CloseButton(false).Floatable(false).Resizable());
	m_mgr.AddPane(m_usage_pane, wxAuiPaneInfo().Bottom().Caption("Used By").MinSize(wxSize(-1, 80))
		.BestSize(wxSize(-1, 120)).CloseButton(false).Floatable(false).Resizable());
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
		SelectScriptInList(script_id);
		RefreshUsage();
		Update();
		return true;
	}
	return false;
}

void BehaviourScriptEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
	RefreshScriptList();
	RefreshUsage();
}

void BehaviourScriptEditorFrame::ClearGameData()
{
	m_script_id = -1;
	m_editor->ClearGameData();
	m_gd.reset();
	RefreshScriptList();
	RefreshUsage();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
}

bool BehaviourScriptEditorFrame::Show(bool show)
{
	if (show)
	{
		// See the header comment: names can change while this editor is hidden.
		RefreshScriptList();
	}
	return EditorFrame::Show(show);
}

void BehaviourScriptEditorFrame::RefreshScriptList()
{
	m_script_list->Freeze();
	m_script_list->Clear();
	m_script_ids.clear();
	if (m_gd)
	{
		for (const auto& script : m_gd->GetSpriteData()->GetScriptNames())
		{
			m_script_ids.push_back(script.first);
			m_script_list->Append(StrWPrintf(L"[%03d] %ls", script.first,
				SpriteData::GetBehaviourDisplayName(script.first).c_str()));
		}
	}
	m_script_list->Enable(m_gd != nullptr);
	m_script_list->Thaw();
	SelectScriptInList(GetOpenScriptId());
}

void BehaviourScriptEditorFrame::SelectScriptInList(int script_id)
{
	const auto it = std::find(m_script_ids.cbegin(), m_script_ids.cend(), script_id);
	if (it != m_script_ids.cend())
	{
		const int row = static_cast<int>(std::distance(m_script_ids.cbegin(), it));
		if (m_script_list->GetSelection() != row)
		{
			m_script_list->SetSelection(row);
			m_script_list->EnsureVisible(row);
		}
	}
	else
	{
		m_script_list->SetSelection(wxNOT_FOUND);
	}
}

void BehaviourScriptEditorFrame::OnScriptSelected()
{
	const int row = m_script_list->GetSelection();
	if (row >= 0 && row < static_cast<int>(m_script_ids.size()))
	{
		Open(m_script_ids[row]);
	}
}

void BehaviourScriptEditorFrame::OnRenameScript()
{
	if (!m_gd)
	{
		return;
	}
	const int row = m_script_list->GetSelection();
	if (row < 0 || row >= static_cast<int>(m_script_ids.size()))
	{
		return;
	}
	const int id = m_script_ids[row];
	const auto old_name = SpriteData::GetBehaviourDisplayName(id);
	wxTextEntryDialog dlg(this, "Display name for this behaviour script (used only in the editor):",
		"Rename Behaviour Script", wxString(old_name));
	// Same validate-and-retry loop as EntityViewerFrame::OnRenameEntity().
	while (dlg.ShowModal() == wxID_OK)
	{
		const auto new_name = dlg.GetValue().ToStdWstring();
		if (new_name == old_name)
		{
			return;
		}
		if (!Labels::IsValid(new_name, Labels::C_BEHAVIOURS, id))
		{
			wxMessageBox("The name must not be empty, must be unique, and must not contain "
				"non-printable characters.", "Rename Behaviour Script", wxOK | wxICON_ERROR, this);
			continue;
		}
		Labels::Update(Labels::C_BEHAVIOURS, id, new_name);
		RefreshScriptList();
		return;
	}
}

void BehaviourScriptEditorFrame::RefreshUsage()
{
	m_usage_pane->Freeze();
	m_usage_pane->DestroyChildren();
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	const int script_id = GetOpenScriptId();
	if (!m_gd || script_id < 0)
	{
		sizer->Add(new wxStaticText(m_usage_pane, wxID_ANY, _("No script open.")), 0, wxALL, 6);
		m_usage_pane->SetSizer(sizer, true);
		m_usage_pane->FitInside();
		m_usage_pane->Thaw();
		return;
	}

	// Collect the matches first (cheap - GetRoomEntities is just a cached map lookup), then build
	// widgets only for a bounded number of them: a heavily-used behaviour can be placed by
	// hundreds of entities, and instantiating that many hyperlink/label controls is what made
	// this pane take seconds to appear. The cap keeps it instant; the rest collapse to a count.
	struct Use { uint16_t room; std::size_t entity_idx; uint8_t type; };
	std::vector<Use> uses;
	for (std::size_t room = 0; room < m_gd->GetRoomData()->GetRoomCount(); ++room)
	{
		const auto entities = m_gd->GetSpriteData()->GetRoomEntities(static_cast<uint16_t>(room));
		for (std::size_t entity_idx = 0; entity_idx < entities.size(); ++entity_idx)
		{
			if (entities[entity_idx].GetBehaviour() == script_id)
			{
				uses.push_back({ static_cast<uint16_t>(room), entity_idx, entities[entity_idx].GetType() });
			}
		}
	}

	if (uses.empty())
	{
		sizer->Add(new wxStaticText(m_usage_pane, wxID_ANY,
			_("No room entities use this behaviour.")), 0, wxALL, 6);
		m_usage_pane->SetSizer(sizer, true);
		m_usage_pane->FitInside();
		m_usage_pane->Thaw();
		return;
	}

	constexpr std::size_t kMaxVisible = 60;
	const std::size_t shown = std::min(uses.size(), kMaxVisible);
	for (std::size_t i = 0; i < shown; ++i)
	{
		const auto& use = uses[i];
		const auto rname = m_gd->GetRoomData()->GetRoomDisplayName(use.room);

		// The room name is a hyperlink back to that room (same nav mechanism and style as the
		// entity editor's stats panel); the entity detail follows as plain text.
		wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
		auto* link = new wxHyperlinkCtrl(m_usage_pane, wxID_ANY,
			StrWPrintf(L"[%03d] %ls", static_cast<int>(use.room), rname.c_str()), wxEmptyString);
		const wxString path = L"Rooms/" + rname;
		link->Bind(wxEVT_HYPERLINK, [this, path](wxHyperlinkEvent&) { NavigateTo(path); });
		row->Add(link, 0, wxALIGN_CENTER_VERTICAL);
		row->Add(new wxStaticText(m_usage_pane, wxID_ANY,
			StrWPrintf(L" - Entity %d, %ls", static_cast<int>(use.entity_idx + 1),
				SpriteData::GetEntityDisplayName(use.type).c_str())),
			0, wxALIGN_CENTER_VERTICAL);
		sizer->Add(row, 0, wxLEFT | wxRIGHT | wxTOP, 6);
	}
	if (uses.size() > shown)
	{
		sizer->Add(new wxStaticText(m_usage_pane, wxID_ANY,
			StrWPrintf(L"...and %d more", static_cast<int>(uses.size() - shown))),
			0, wxALL, 6);
	}
	sizer->AddSpacer(6);

	m_usage_pane->SetSizer(sizer, true);
	m_usage_pane->FitInside();
	m_usage_pane->Thaw();
}

void BehaviourScriptEditorFrame::NavigateTo(const wxString& path)
{
	if (path.IsEmpty())
	{
		return;
	}
	// Same navigation mechanism the other editors use - the event bubbles up to MainFrame, which
	// resolves the path (the Rooms node carries the room number) and opens that room.
	wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
	evt.SetString(path);
	evt.SetClientData(this);
	wxPostEvent(this, evt);
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
				// The import can rename the script (or introduce a new id) - rebuild the list
				// so its labels match before re-selecting.
				RefreshScriptList();
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
			// Imports can rename scripts or introduce new ids - see OnLoadFromYaml().
			RefreshScriptList();
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
