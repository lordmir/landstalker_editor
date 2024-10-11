#include <user_interface/script/include/ScriptEditorFrame.h>

ScriptEditorFrame::ScriptEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_editor = new ScriptEditorCtrl(this);

	// add the panes to the manager
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

ScriptEditorFrame::~ScriptEditorFrame()
{
}

bool ScriptEditorFrame::Open()
{
	if (m_gd)
	{
		m_editor->Open();
		return true;
	}
	return false;
}

void ScriptEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
}

void ScriptEditorFrame::ClearGameData()
{
	m_editor->ClearGameData();
	m_gd.reset();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void ScriptEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void ScriptEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
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

void ScriptEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd != nullptr)
	{
		props.GetGrid()->Freeze();
		props.GetGrid()->Thaw();
	}
}

void ScriptEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
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
