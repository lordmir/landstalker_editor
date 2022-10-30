#include "EditorFrame.h"
#include <set>

wxDEFINE_EVENT(EVT_STATUSBAR_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_MENU_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_MENU_CLEAR, wxCommandEvent);

EditorFrame::EditorFrame(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, parent->GetSize()),
	  m_props_init(false)
{
	this->Connect(wxEVT_AUI_PANE_CLOSE, wxAuiManagerEventHandler(EditorFrame::OnPaneClose), nullptr, this);
}

EditorFrame::~EditorFrame()
{
}

void EditorFrame::InitStatusBar(wxStatusBar& status) const
{
}

void EditorFrame::UpdateStatusBar(wxStatusBar& status) const
{
}

void EditorFrame::ClearStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(1);
	status.SetStatusText("");
}

void EditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	m_props_init = true;
}

void EditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
}

void EditorFrame::ClearProperties(wxPropertyGridManager& props) const
{
	if(m_props_init == true)
	{
		props.GetGrid()->Clear();
		m_props_init = false;
	}
}

void EditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
}

void EditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
}

void EditorFrame::ClearMenu(wxMenuBar& menu) const
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
		auto* menu = mnu.second.second;
		auto pos = parent->FindMenu(menu->GetTitle());
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

void EditorFrame::OnMenuClick(wxMenuEvent& evt)
{
}

bool EditorFrame::Show(bool show)
{
	if (show == true)
	{
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

void EditorFrame::UpdateUI() const
{
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

void EditorFrame::FireEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(this);
	wxPostEvent(this->GetParent(), evt);
}

void EditorFrame::FireEvent(const wxEventType& e)
{
	wxCommandEvent evt(e);
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
	if (parent.GetTitle() == "File")
	{
		position += 4;
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

void EditorFrame::OnPaneClose(wxAuiManagerEvent& event)
{
	auto* pane = event.GetPane();
	pane->Hide();
	UpdateUI();
	event.Skip();
}
