#include "EditorFrame.h"

wxDEFINE_EVENT(EVT_STATUSBAR_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_MENU_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_MENU_CLEAR, wxCommandEvent);

EditorFrame::EditorFrame(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id)
{
	FireEvent(EVT_STATUSBAR_INIT);
	FireEvent(EVT_PROPERTIES_INIT);
	FireEvent(EVT_MENU_INIT);
}

EditorFrame::~EditorFrame()
{
	FireEvent(EVT_STATUSBAR_CLEAR);
	FireEvent(EVT_PROPERTIES_CLEAR);
	FireEvent(EVT_MENU_CLEAR);
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
}

void EditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
}

void EditorFrame::ClearProperties(wxPropertyGridManager& props) const
{
	props.GetGrid()->Clear();
}

void EditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
}

void EditorFrame::InitMenu(wxMenuBar& menu) const
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
	parent.Insert(position, menuItem);
	m_menuitems.insert({ id, {&parent, menuItem} });
	return *menuItem;
}

wxMenu& EditorFrame::GetMenu(int id) const
{
	return *m_menus[id].second;
}

wxMenuItem& EditorFrame::GetMenuItem(int id) const
{
	return *m_menuitems[id].second;
}
