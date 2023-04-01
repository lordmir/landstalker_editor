#include <Map2DEditorFrame.h>

wxBEGIN_EVENT_TABLE(Map2DEditorFrame, wxWindow)
EVT_SLIDER(wxID_ANY, Map2DEditorFrame::OnZoomChange)
EVT_TOOL(wxID_ANY, Map2DEditorFrame::OnButtonClicked)
EVT_COMMAND(wxID_ANY, EVT_MAP_SELECT, Map2DEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_MAP_HOVER, Map2DEditorFrame::OnTileHovered)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, Map2DEditorFrame::OnTileSelect)
EVT_COMMAND(wxID_ANY, EVT_MAP_EDIT_REQUEST, Map2DEditorFrame::OnTileEditRequested)
wxEND_EVENT_TABLE()

enum TOOL_IDS
{
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_TILE_NUMBERS,
	ID_TOGGLE_ALPHA,
	ID_INSERT_ROW_BEFORE,
	ID_INSERT_ROW_AFTER,
	ID_DELETE_ROW,
	ID_INSERT_COLUMN_BEFORE,
	ID_INSERT_COLUMN_AFTER,
	ID_DELETE_COLUMN,
	ID_HFLIP_TILE,
	ID_VFLIP_TILE,
	ID_TOGGLE_TILE_PRIORITY,
	ID_SELECT,
	ID_PENCIL,
	ID_ZOOM_SLIDER,
	ID_TILESET_TOGGLE_GRIDLINES,
	ID_TILESET_ALPHA,
	ID_TILESET_TILE_NUMBERS,
	ID_TILESET_ZOOM,
	ID_TILESET_SELECT,
	ID_PALETTE_SELECT
};

enum MENU_IDS
{
	ID_FILE_EXPORT_BIN = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_BIN,
	ID_FILE_IMPORT_CSV,
	ID_VIEW,
	ID_VIEW_TOGGLE_GRIDLINES,
	ID_VIEW_TOGGLE_TILE_NOS,
	ID_VIEW_TOGGLE_ALPHA,
	ID_TOOLS,
	ID_TOOLS_TILES,
	ID_TOOLS_TILEMAP_TOOLBAR,
	ID_TOOLS_TILESET_TOOLBAR
};

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

void Map2DEditorFrame::SetActivePalette(const std::string& name)
{
	if (m_gd != nullptr && m_gd->GetPalette(name) != nullptr)
	{
		m_mapedit->SetActivePalette(name);
	}
}

void Map2DEditorFrame::SetTileset(const std::string& name)
{
	if (m_gd != nullptr && m_gd->GetTileset(name) != nullptr)
	{
		m_map->SetTileset(name);
		m_mapedit->SetTileset(name);
	}
}

void Map2DEditorFrame::SetDrawTile(const Tile& tile)
{
	m_mapedit->SetDrawTile(tile);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void Map2DEditorFrame::Redraw()
{
	m_mapedit->RedrawTiles();
	m_tileset->RedrawTiles();
}

void Map2DEditorFrame::RedrawTiles(int index) const
{
	m_mapedit->RedrawTiles(index);
	m_tileset->RedrawTiles(index);
}

void Map2DEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	evt.Skip();
}

void Map2DEditorFrame::OnButtonClicked(wxCommandEvent& evt)
{
	evt.Skip();
}

void Map2DEditorFrame::OnTileChanged(wxCommandEvent& evt)
{
	m_mapedit->SetSelectedTile(m_tile);
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void Map2DEditorFrame::OnTileHovered(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void Map2DEditorFrame::OnTileSelect(wxCommandEvent& evt)
{
	int tileId = std::stoi(evt.GetString().ToStdString());
	if (tileId >= 0)
	{
		m_tile = tileId;
		m_mapedit->SetMode(Map2DEditor::Mode::PENCIL);
		m_mapedit->SetDrawTile(m_tile);
	}
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void Map2DEditorFrame::OnTileEditRequested(wxCommandEvent& evt)
{
	evt.Skip();
}

std::string Map2DEditorFrame::PrettyPrintTile(const Map2DEditor::TilePosition& tp) const
{
	std::ostringstream ss;
	auto t = m_mapedit->GetTileAtPosition(tp);
	ss << "(" << tp.x << ", " << tp.y << ") [" << t.GetIndex() << "]";
	return ss.str();
}

std::string Map2DEditorFrame::PrettyPrintMode() const
{
	std::ostringstream ss;
	switch (m_mapedit->GetMode())
	{
	case Map2DEditor::Mode::SELECT:
		ss << "SELECT";
		break;
	case Map2DEditor::Mode::PENCIL:
		ss << "DRAW - Tile " << m_mapedit->GetDrawTile().GetTileValue();
		break;
	default:
		ss << "?";
	}
	return ss.str();
}

void Map2DEditorFrame::UpdateStatusBar(wxStatusBar& status) const
{
	std::ostringstream ss;
	if (m_mapedit->IsSelectionValid())
	{
		ss << "Tile: " << PrettyPrintTile(m_mapedit->GetSelection());
		if (m_mapedit->IsHoverValid())
		{
			ss << " (Cursor at tile " << PrettyPrintTile(m_mapedit->GetHover()) << ")";
		}
	}
	else if (m_mapedit->IsHoverValid())
	{
		ss << "Cursor at tile " << PrettyPrintTile(m_mapedit->GetHover());
	}
	status.SetStatusText(ss.str(), 0);
	status.SetStatusText(PrettyPrintMode(), 1);
	status.SetStatusText(wxString::Format("Zoom: %d%%", m_zoom), 2);
	//m_editor->SetPixelSize(m_zoomslider->GetValue());
}

void Map2DEditorFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 2);
}

void Map2DEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
}

void Map2DEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
}

void Map2DEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
}

void Map2DEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_BIN, "Export Tilemap as Binary...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_CSV, "Export Tilemap as CSV...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_PNG, "Export Tileset as PNG...");
	AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_BIN, "Import Tileset from Binary...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_CSV, "Import Tileset from CSV...");
	AddMenuItem(fileMenu, 5, wxID_ANY, "", wxITEM_SEPARATOR);
	auto& viewMenu = AddMenu(menu, 1, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_TILE_NOS, "Tile Numbers", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	auto& toolsMenu = AddMenu(menu, 2, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_TILES, "Tiles", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_TILEMAP_TOOLBAR, "Tilemap Toolbar", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, ID_TOOLS_TILESET_TOOLBAR, "Tileset Toolbar", wxITEM_CHECK);

	wxBitmap bmp(16, 16);

	wxAuiToolBar* tilemap_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);

	tilemap_tb->AddTool(ID_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines");
	tilemap_tb->AddTool(ID_TOGGLE_TILE_NUMBERS, "Toggle Tile Numbers", ilist.GetImage("tile_nums"), "Toggle Tile Numbers");
	tilemap_tb->AddTool(ID_TOGGLE_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Alpha");
	tilemap_tb->AddSeparator();
	tilemap_tb->AddTool(ID_INSERT_ROW_BEFORE, "Insert Row Above", ilist.GetImage("insert_row_before"), "Insert Row Above");
	tilemap_tb->AddTool(ID_INSERT_ROW_AFTER, "Insert Row Below", ilist.GetImage("insert_row_after"), "Insert Row Below");
	tilemap_tb->AddTool(ID_DELETE_ROW, "Delete Row", ilist.GetImage("delete_row"), "Delete Row");
	tilemap_tb->AddTool(ID_INSERT_COLUMN_BEFORE, "Insert Column To Left", ilist.GetImage("insert_column_before"), "Insert Column To Left");
	tilemap_tb->AddTool(ID_INSERT_COLUMN_AFTER, "Insert Column To Right", ilist.GetImage("insert_column_after"), "Insert Column To Right");
	tilemap_tb->AddTool(ID_DELETE_COLUMN, "Delete Column", ilist.GetImage("delete_column"), "Column Row");
	tilemap_tb->AddSeparator();
	tilemap_tb->AddTool(ID_HFLIP_TILE, "Horizontally Flip Tile", bmp, "Horizontally Flip Tile");
	tilemap_tb->AddTool(ID_VFLIP_TILE, "Vertically Flip Tile", bmp, "Vertically Flip Tile");
	tilemap_tb->AddTool(ID_TOGGLE_TILE_PRIORITY, "Toggle Tile Priority", bmp, "Toggle Tile Priority");
	tilemap_tb->AddSeparator();
	tilemap_tb->AddTool(ID_SELECT, "Select", bmp, "Select", wxITEM_RADIO);
	tilemap_tb->AddTool(ID_PENCIL, "Pencil", ilist.GetImage("pencil"), "Pencil", wxITEM_RADIO);
	tilemap_tb->AddSeparator();
	tilemap_tb->AddLabel(wxID_ANY, "Zoom:");
	m_zoomslider = new wxSlider(tilemap_tb, ID_ZOOM_SLIDER, 8, 1, 16, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	tilemap_tb->AddControl(m_zoomslider, "Zoom");
	AddToolbar(m_mgr, *tilemap_tb, "Tilemap", "Tilemap Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));
	wxAuiToolBar* tileset_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	auto tileset_dropdown = new wxComboBox(tileset_tb, ID_TILESET_SELECT, "");
	auto palette_dropdown = new wxComboBox(tileset_tb, ID_PALETTE_SELECT, "");
	auto tileset_zoom = new wxSlider(tileset_tb, ID_TILESET_ZOOM, 8, 1, 16, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	tileset_tb->SetToolBitmapSize(wxSize(16, 16));
	tileset_tb->AddLabel(wxID_ANY, "Tileset:");
	tileset_tb->AddControl(tileset_dropdown, "Tileset");
	tileset_tb->AddLabel(wxID_ANY, "Palette:");
	tileset_tb->AddControl(palette_dropdown, "Palette");
	tileset_tb->AddSeparator();
	tileset_tb->AddTool(ID_TILESET_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	tileset_tb->AddTool(ID_TILESET_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Gridlines", wxITEM_CHECK);
	tileset_tb->AddTool(ID_TILESET_TILE_NUMBERS, "Toggle Tile Numbers", ilist.GetImage("tile_nums"), "Toggle Gridlines", wxITEM_CHECK);
	tileset_tb->AddLabel(wxID_ANY, "Zoom:");
	tileset_tb->AddControl(tileset_zoom, "Zoom");
	AddToolbar(m_mgr, *tileset_tb, "Draw", "Drawing Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(2));
	UpdateUI();

	m_mgr.Update();
}

void Map2DEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
}

void Map2DEditorFrame::UpdateUI() const
{
	CheckMenuItem(ID_TOOLS_TILES, IsPaneVisible(m_tileset));
	CheckMenuItem(ID_TOOLS_TILEMAP_TOOLBAR, IsToolbarVisible("Tilemap"));
	CheckMenuItem(ID_TOOLS_TILESET_TOOLBAR, IsToolbarVisible("Tileset"));
	if (m_mapedit != nullptr)
	{
		CheckMenuItem(ID_VIEW_TOGGLE_ALPHA, !m_mapedit->GetAlphaEnabled());
		CheckToolbarItem("Tilemap", ID_TOGGLE_ALPHA, !m_mapedit->GetAlphaEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_GRIDLINES, m_mapedit->GetBordersEnabled());
		CheckToolbarItem("Tilemap", ID_TOGGLE_GRIDLINES, m_mapedit->GetBordersEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_TILE_NOS, m_mapedit->GetTileNumbersEnabled());
		CheckToolbarItem("Tilemap", ID_TOGGLE_TILE_NUMBERS, m_mapedit->GetTileNumbersEnabled());
	}
	if (m_tileset != nullptr)
	{
		CheckToolbarItem("Tileset", ID_TILESET_ALPHA, !m_tileset->GetAlphaEnabled());
		CheckToolbarItem("Tileset", ID_TILESET_TOGGLE_GRIDLINES, m_tileset->GetBordersEnabled());
		CheckToolbarItem("Tileset", ID_TILESET_TILE_NUMBERS, m_tileset->GetTileNumbersEnabled());
	}
}

void Map2DEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
}
