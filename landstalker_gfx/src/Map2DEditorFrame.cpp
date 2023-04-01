#include <Map2DEditorFrame.h>

Map2DEditorFrame::Map2DEditorFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_title("")
{
	m_mgr.SetManagedWindow(this);

	m_mapedit = new Map2DEditor(this);
	m_tileset = new TilesetEditor(this);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.4, 0.4);
	m_mgr.AddPane(m_tileset, wxAuiPaneInfo().Right().Layer(1).MinSize(100, 100).BestSize(450, 450).FloatingSize(450, 450).Caption("Tiles"));
	m_mgr.AddPane(m_mapedit, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

Map2DEditorFrame::~Map2DEditorFrame()
{
}

void Map2DEditorFrame::Open(const std::string& map_name)
{
	if (m_gd == nullptr)
	{
		return;
	}
	m_map = m_gd->GetTilemap(map_name);
	m_tiles = m_gd->GetTileset(m_map->GetTileset());
	m_palette = m_gd->GetPalette(m_tiles->GetDefaultPalette());

	m_mapedit->Open(m_map);
	m_tileset->Open(m_tiles->GetData());
	m_tileset->SetActivePalette(m_tiles->GetDefaultPalette());
	m_tile = 0;
	m_tileset->SelectTile(m_tile.GetIndex());
}

void Map2DEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_tileset->SetGameData(gd);
	m_mapedit->SetGameData(gd);
}

void Map2DEditorFrame::ClearGameData()
{
	m_gd = nullptr;
	m_tileset->SetGameData(nullptr);
	m_mapedit->ClearGameData();
}
