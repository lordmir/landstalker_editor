#include <rooms/RoomConstantsFrame.h>

RoomConstantsEditorFrame::RoomConstantsEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_editor = new RoomConstantsEditorCtrl(this, imglst);

	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());
	m_mgr.Update();
}

RoomConstantsEditorFrame::~RoomConstantsEditorFrame()
{
}

bool RoomConstantsEditorFrame::Open(int row)
{
	if (m_gd)
	{
		// Rooms can be added while this page is closed, so refresh before showing to
		// pick up any new entries in the room dropdown.
		m_editor->RefreshData();
		m_editor->Open(row);
		m_reset_props = true;
		FireEvent(EVT_PROPERTIES_UPDATE);
		UpdateUI();
		return true;
	}
	return false;
}

void RoomConstantsEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	m_editor->SetGameData(gd);
}

void RoomConstantsEditorFrame::ClearGameData()
{
	m_editor->ClearGameData();
	m_gd.reset();
	m_reset_props = true;
	FireEvent(EVT_PROPERTIES_UPDATE);
	UpdateUI();
}

void RoomConstantsEditorFrame::UpdateUI() const
{
}

void RoomConstantsEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (m_gd && ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		EditorFrame::InitProperties(props);
		RefreshProperties(props);
	}
}

void RoomConstantsEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
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

void RoomConstantsEditorFrame::RefreshProperties(wxPropertyGridManager& /*props*/) const
{
}

void RoomConstantsEditorFrame::OnPropertyChange(wxPropertyGridEvent& /*evt*/)
{
}
