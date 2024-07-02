#include <EntityViewerFrame.h>

EntityViewerFrame::EntityViewerFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);
	m_entity_ctrl = new EntityViewerCtrl(this);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
	m_mgr.AddPane(m_entity_ctrl, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

EntityViewerFrame::~EntityViewerFrame()
{
}

bool EntityViewerFrame::Open(const std::string& entity)
{
	return false;
}
