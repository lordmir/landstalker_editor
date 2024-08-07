#include <user_interface/behaviours/include/BehaviourScriptEditorFrame.h>

#include <wx/wx.h>


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
	return false;
}

void BehaviourScriptEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
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
