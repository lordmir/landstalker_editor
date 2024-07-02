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

bool EntityViewerFrame::Open(int entity)
{
	if (m_gd && entity >= 0 && entity < 256)
	{
		auto frame = m_gd->GetSpriteData()->GetDefaultEntityFrame(entity);
		auto palette_idxs = m_gd->GetSpriteData()->GetSpritePaletteIdxs(entity);
		m_palette = m_gd->GetSpriteData()->GetSpritePalette(palette_idxs.first, palette_idxs.second);

		m_entity_ctrl->Open(frame->GetData(), m_palette);
		return true;
	}
	return false;
}

void EntityViewerFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_entity_ctrl->SetGameData(gd);
}

void EntityViewerFrame::ClearGameData()
{
	m_entity_ctrl->ClearGameData();
	m_gd.reset();
}
