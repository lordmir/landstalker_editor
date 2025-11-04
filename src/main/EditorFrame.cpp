#include <main/EditorFrame.h>
#include <set>

wxDEFINE_EVENT(EVT_STATUSBAR_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_MENU_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_MENU_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_GO_TO_NAV_ITEM, wxCommandEvent);
wxDEFINE_EVENT(EVT_RENAME_NAV_ITEM, wxCommandEvent);
wxDEFINE_EVENT(EVT_DELETE_NAV_ITEM, wxCommandEvent);
wxDEFINE_EVENT(EVT_ADD_NAV_ITEM, wxCommandEvent);

std::unordered_map<int, std::pair<wxMenuBar*, wxMenu*>> EditorFrame::m_menus;
std::unordered_map<int, std::pair<wxMenu*, wxMenuItem*>> EditorFrame::m_menuitems;

EditorFrame::EditorFrame(wxWindow* parent, wxWindowID id, ImageList* imglst)
	: wxWindow(parent, id, wxDefaultPosition, parent->GetSize(), wxWANTS_CHARS),
	  m_imglst(imglst),
	  m_props_init(false)
{
	this->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(EditorFrame::OnPaneClose), nullptr, this);
}

EditorFrame::~EditorFrame()
{
}

void EditorFrame::InitStatusBar(wxStatusBar& /*status*/) const
{
}

void EditorFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& evt) const
{
	if (status.GetFieldsCount() > evt.GetInt())
	{
		status.SetStatusText(evt.GetString(), evt.GetInt());
	}
}

void EditorFrame::ClearStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(1);
	status.SetStatusText("");
}

void EditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	m_props_init = true;

	if (props.GetGrid())
	{
		props.GetGrid()->SetSplitterPosition(props.GetClientSize().GetWidth() * 3 / 5);
	}
}

void EditorFrame::UpdateProperties(wxPropertyGridManager& /*props*/) const
{
}

void EditorFrame::ClearProperties(wxPropertyGridManager& props) const
{
	if(m_props_init == true)
	{
		m_props_init = false;
		props.GetGrid()->Clear();
	}
}

void EditorFrame::OnPropertyChange(wxPropertyGridEvent& /*evt*/)
{
}

void EditorFrame::InitMenu(wxMenuBar& menu, ImageList& /*ilist*/) const
{
	ClearMenu(menu);
}

void EditorFrame::ClearMenu(wxMenuBar& /*menu*/) const
{
	for (const auto& itm : m_menuitems)
	{
		auto* parent = itm.second.first;
		auto* item = itm.second.second;
		delete parent->Remove(item);
	}
	m_menuitems.clear();
	for (const auto& mnu : m_menus)
	{
		auto* parent = mnu.second.first;
		auto* pmenu = mnu.second.second;
		auto pos = parent->FindMenu(pmenu->GetTitle());
		delete parent->Remove(pos);
	}
	m_menus.clear();
	std::set<wxAuiManager*> mgrs;
	for (const auto& tb : m_toolbars)
	{
		tb.second->Hide();
		auto* mgr = wxAuiManager::GetManager(tb.second->GetParent());
		mgr->DetachPane(tb.second);
		mgrs.insert(mgr);
		tb.second->Destroy();
	}
	m_toolbars.clear();
	for(auto* mgr : mgrs)
	{
		mgr->Update();
	}
	this->GetParent()->Refresh(true);
}

void EditorFrame::OnMenuClick(wxMenuEvent&)
{
}

bool EditorFrame::Show(bool show)
{
	if(show != IsShown())
	{
		if (show == true)
		{
			m_props_init = false;
			FireEvent(EVT_STATUSBAR_INIT);
			FireEvent(EVT_PROPERTIES_INIT);
			FireEvent(EVT_MENU_INIT);
		}
		else
		{
			FireEvent(EVT_STATUSBAR_CLEAR);
			FireEvent(EVT_PROPERTIES_CLEAR);
			FireEvent(EVT_MENU_CLEAR);
		}
		return wxWindow::Show(show);
	}
	return false;
}

void EditorFrame::UpdateUI() const
{
}

void EditorFrame::SetImageList(ImageList* imglst)
{
	m_imglst = imglst;
}

void EditorFrame::CheckMenuItem(int id, bool checked) const
{
	auto* menu = GetMenuItem(id);
	if (menu != nullptr)
	{
		menu->Check(checked);
	}
}

void EditorFrame::CheckToolbarItem(const std::string& name, int id, bool checked) const
{
	auto* tb = GetToolbar(name);
	if (tb != nullptr)
	{
		tb->ToggleTool(id, checked);
		tb->Refresh();
	}
}

void EditorFrame::EnableMenuItem(int id, bool enabled) const
{
	auto* menu = GetMenuItem(id);
	if (menu != nullptr)
	{
		menu->Enable(enabled);
	}
}

void EditorFrame::EnableToolbarItem(const std::string& name, int id, bool enabled) const
{
	auto* tb = GetToolbar(name);
	if (tb != nullptr)
	{
		tb->EnableTool(id, enabled);
		tb->Refresh();
	}
}

wxControl* EditorFrame::GetToolbarItem(const std::string& name, int id)
{
	auto* tb = GetToolbar(name);
	if (tb != nullptr)
	{
		return tb->FindControl(id);
	}
	return nullptr;
}

wxControl* EditorFrame::GetToolbarItem(const std::string& name, int id) const
{
	auto* tb = GetToolbar(name);
	if (tb != nullptr)
	{
		return tb->FindControl(id);
	}
	return nullptr;
}

void EditorFrame::SetPaneVisibility(wxWindow* pane, bool visible)
{
	if (pane == nullptr)
	{
		return;
	}
	auto* mgr = wxAuiManager::GetManager(pane->GetParent());
	auto& paneInfo = mgr->GetPane(pane);
	bool currentlyVisible = paneInfo.IsShown();
	if (visible != currentlyVisible)
	{
		paneInfo.Show(visible);
		mgr->Update();
	}
}

bool EditorFrame::IsPaneVisible(wxWindow* pane) const
{
	if (pane == nullptr)
	{
		return false;
	}
	auto* mgr = wxAuiManager::GetManager(pane->GetParent());
	auto& paneInfo = mgr->GetPane(pane);
	return paneInfo.IsShown();
}

void EditorFrame::InsertPane(wxWindow* pane, const wxAuiPaneInfo& insert_location, int insert_level)
{
	auto* mgr = wxAuiManager::GetManager(pane->GetParent());
	mgr->InsertPane(pane, insert_location, insert_level);
}

void EditorFrame::SetToolbarVisibility(const std::string& name, bool visible)
{
	auto* tb = GetToolbar(name);
	if (tb == nullptr)
	{
		return;
	}
	auto* mgr = wxAuiManager::GetManager(tb->GetParent());
	auto& pane = mgr->GetPane(tb);
	bool currentlyVisible = pane.IsShown();
	if (visible != currentlyVisible)
	{
		pane.Show(visible);
		mgr->Update();
	}
}

bool EditorFrame::IsToolbarVisible(const std::string& name) const
{
	auto* tb = GetToolbar(name);
	if (tb == nullptr)
	{
		return false;
	}
	auto* mgr = wxAuiManager::GetManager(tb->GetParent());
	auto& pane = mgr->GetPane(tb);
	return pane.IsShown();
}

bool EditorFrame::ArePropsInitialised() const
{
	return m_props_init;
}

void EditorFrame::FireEvent(const wxEventType& e, const wxString& data, long numeric_data, long extra_numeric_data, long extra_extra_numeric_data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetInt(numeric_data);
	evt.SetExtraLong(extra_numeric_data);
	evt.SetId(extra_extra_numeric_data);
	evt.SetClientData(this);
	wxPostEvent(this->GetParent(), evt);
}

wxMenu& EditorFrame::AddMenu(wxMenuBar& parent, int position, int id, const std::string& name) const
{
	wxMenu* menu = new wxMenu();
	parent.Insert(position, menu, name);
	m_menus.insert({ id, {&parent, menu } });
	return *menu;
}

wxMenuItem& EditorFrame::AddMenuItem(wxMenu& parent, int position, int id, const std::string& name, wxItemKind kind, const std::string& help) const
{
	wxMenuItem* menuItem = new wxMenuItem(&parent, id, name, help, kind);
	assert(id != wxID_ANY);
	assert(m_menuitems.find(id) == m_menuitems.end());
	if (parent.GetTitle() == "&File")
	{
		if (position == 0)
		{
			auto sep = new wxMenuItem(&parent, wxID_ANY, wxEmptyString, wxEmptyString, wxITEM_SEPARATOR);
			parent.Insert(position + 6, sep);
			m_menuitems.insert({ wxID_ANY, {&parent, sep} });
		}
		position += 7;
	}
	parent.Insert(position, menuItem);
	m_menuitems.insert({ id, {&parent, menuItem} });
	return *menuItem;
}

wxAuiToolBar& EditorFrame::AddToolbar(wxAuiManager& mgr, wxAuiToolBar& tb, const std::string& name, const std::string title, wxAuiPaneInfo& position) const
{
	mgr.AddPane(&tb, position.Name(name).Caption(title));
	tb.Realize();
	m_toolbars.insert({ name, &tb });
	return tb;
}

wxMenu* EditorFrame::GetMenu(int id) const
{
	auto result = m_menus.find(id);
	if (result == m_menus.end())
	{
		return nullptr;
	}
	return result->second.second;
}

wxMenuItem* EditorFrame::GetMenuItem(int id) const
{
	auto result = m_menuitems.find(id);
	if (result == m_menuitems.end())
	{
		return nullptr;
	}
	return result->second.second;
}

wxAuiToolBar* EditorFrame::GetToolbar(const std::string name) const
{
	auto result = m_toolbars.find(name);
	if (result == m_toolbars.end())
	{
		return nullptr;
	}
	return result->second;
}

ImageList* EditorFrame::GetImageList()
{
	return m_imglst;
}

void EditorFrame::OnPaneClose(wxAuiManagerEvent& event)
{
	auto* pane = event.GetPane();
	pane->Hide();
	UpdateUI();
	event.Skip();
}
