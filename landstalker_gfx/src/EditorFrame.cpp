#include "EditorFrame.h"

wxDEFINE_EVENT(EVT_STATUSBAR_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUSBAR_CLEAR, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_INIT, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_PROPERTIES_CLEAR, wxCommandEvent);

EditorFrame::EditorFrame(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id)
{
	FireEvent(EVT_STATUSBAR_INIT);
	FireEvent(EVT_PROPERTIES_INIT);
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

bool EditorFrame::Show(bool show)
{
	if (show == true)
	{
		FireEvent(EVT_STATUSBAR_INIT);
		FireEvent(EVT_PROPERTIES_INIT);
	}
	else
	{
		FireEvent(EVT_STATUSBAR_CLEAR);
		FireEvent(EVT_PROPERTIES_CLEAR);
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
