#include <2d_maps/Map2DEditorFrame.h>


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

wxBEGIN_EVENT_TABLE(Map2DEditorFrame, wxWindow)
EVT_SLIDER(ID_ZOOM_SLIDER, Map2DEditorFrame::OnZoomChange)
EVT_SLIDER(ID_TILESET_ZOOM, Map2DEditorFrame::OnTilesetZoomChange)
EVT_COMMAND(wxID_ANY, EVT_MAP_SELECT, Map2DEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_MAP_HOVER, Map2DEditorFrame::OnTileHovered)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, Map2DEditorFrame::OnTileSelect)
EVT_COMMAND(wxID_ANY, EVT_MAP_EDIT_REQUEST, Map2DEditorFrame::OnTileEditRequested)
EVT_COMBOBOX(ID_TILESET_SELECT, Map2DEditorFrame::OnTilesetSelect)
EVT_COMBOBOX(ID_PALETTE_SELECT, Map2DEditorFrame::OnPaletteSelect)
wxEND_EVENT_TABLE()

using namespace Landstalker;

Map2DEditorFrame::Map2DEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_title(""),
	  m_tileset(nullptr),
	  m_mapedit(nullptr),
	  m_zoomslider(nullptr),
	  m_tileset_zoomslider(nullptr),
	  m_tileset_select(nullptr),
	  m_palette_select(nullptr),
	  m_map(nullptr),
	  m_tiles(nullptr),
	  m_palette(nullptr),
	  m_tile(0),
	  m_zoom(6)
{
	m_mgr.SetManagedWindow(this);

	m_mapedit = new Map2DEditor(this);
	m_tileset = new TilesetEditor(this);
	m_mapedit->SetPixelSize(m_zoom);
	m_tileset->SetPixelSize(4);

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
	m_mapedit->SetMode(Map2DEditor::Mode::SELECT);
	m_mapedit->SetDrawTile(0);
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
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
	if (m_tileset_select != nullptr)
	{
		m_tileset_select->Clear();
	}
	if (m_palette_select != nullptr)
	{
		m_palette_select->Clear();
	}
}

void Map2DEditorFrame::SetActivePalette(const std::string& name)
{
	if (m_gd != nullptr && m_gd->GetPalette(name) != nullptr)
	{
		m_palette = m_gd->GetPalette(name);
		m_mapedit->SetActivePalette(name);
		m_tileset->SetActivePalette(name);
		if (m_palette_select != nullptr)
		{
			m_palette_select->SetStringSelection(name);
		}
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
}

void Map2DEditorFrame::SetTileset(const std::string& name)
{
	if (m_gd != nullptr && m_gd->GetTileset(name) != nullptr)
	{
		m_tiles = m_gd->GetTileset(name);
		m_map->SetTileset(name);
		m_mapedit->SetTileset(name);
		m_tileset->Open(m_gd->GetTileset(name)->GetData());
		m_tile = 0;
		m_tileset->SelectTile(m_tile.GetIndex());
		if (m_tileset_select != nullptr)
		{
			m_tileset_select->SetStringSelection(name);
		}
		FireEvent(EVT_PROPERTIES_UPDATE);
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

bool Map2DEditorFrame::ExportBin(const std::string& filename) const
{
	auto compression = Tilemap2D::FromFileExtension(filename);
	ByteVector bytes;
	m_map->GetData()->GetBits(bytes, compression);
	WriteBytes(bytes, filename);
	return true;
}

bool Map2DEditorFrame::ExportPng(const std::string& filename) const
{
	int width = m_map->GetData()->GetWidth() * m_tiles->GetData()->GetTileWidth();
	int height = m_map->GetData()->GetHeight() * m_tiles->GetData()->GetTileHeight();
	ImageBufferWx buf(width, height);
	buf.InsertMap(0, 0, 0, *m_map->GetData(), *m_tiles->GetData());
	return buf.WritePNG(filename, {m_palette->GetData()}, true);
}

bool Map2DEditorFrame::ExportCsv(const std::string& filename) const
{
	std::ofstream fs(filename, std::ios::out | std::ios::trunc);

	fs << static_cast<int>(m_map->GetData()->GetCompression()) << "," << m_map->GetData()->GetBase() << "," << m_map->GetData()->GetLeft()
	   << "," << m_map->GetData()->GetTop() << std::endl;

	for (std::size_t y = 0; y < m_map->GetData()->GetHeight(); ++y)
	{
		for (std::size_t x = 0; x < m_map->GetData()->GetWidth(); ++x)
		{
			fs << StrPrintf("0x%04X", m_map->GetData()->GetTile(x, y).GetTileValue());
			if ((x + 1) == m_map->GetData()->GetWidth())
			{
				fs << std::endl;
			}
			else
			{
				fs << ",";
			}
		}
	}

	return true;
}

bool Map2DEditorFrame::ImportBin(const std::string& filename, uint16_t base, int width, int height) const
{
	auto compression = Tilemap2D::FromFileExtension(filename);
	ByteVector bytes = ReadBytes(filename);
	m_map->SetCompression(compression);
	m_map->SetBase(base);
	m_map->GetData()->Open(bytes, width, height, compression, base);
	return true;
}

bool Map2DEditorFrame::ImportCsv(const std::string& filename) const
{
	std::ifstream fs(filename, std::ios::in);

	uint32_t c, b, l, t;
	std::vector<std::vector<uint16_t>> tiles;
	std::string row;
	std::string cell;
	std::getline(fs, row);
	std::istringstream ss(row);
	std::getline(ss, cell, ',');
	StrToInt(cell, c);
	std::getline(ss, cell, ',');
	StrToInt(cell, b);
	std::getline(ss, cell, ',');
	StrToInt(cell, l);
	std::getline(ss, cell, ',');
	StrToInt(cell, t);

	while (std::getline(fs, row))
	{
		tiles.push_back(std::vector<uint16_t>());
		std::istringstream rss(row);
		while (std::getline(rss, cell, ','))
		{
			tiles.back().push_back(std::stoi(cell, nullptr, 16));
		}
	}

	if (tiles.size() == 0 || tiles.front().size() == 0)
	{
		return false;
	}
	int w = tiles.front().size();
	int h = tiles.size();
	Tilemap2D::Compression compression = static_cast<Tilemap2D::Compression>(c);
	m_map->GetData()->Resize(w, h);
	m_map->SetCompression(compression);
	m_map->GetData()->SetCompression(compression);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			m_map->GetData()->SetTile(Tile(tiles[y][x]), x, y);
		}
	}
	m_map->GetData()->SetLeft(l);
	m_map->GetData()->SetTop(t);
	m_map->SetBase(b);
	m_map->GetData()->SetBase(b);
	return true;
}

void Map2DEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	m_zoom = m_zoomslider->GetValue();
	m_mapedit->SetPixelSize(m_zoomslider->GetValue());
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void Map2DEditorFrame::OnTilesetZoomChange(wxCommandEvent& evt)
{
	m_tileset->SetPixelSize(m_tileset_zoomslider->GetValue());
	evt.Skip();
}

void Map2DEditorFrame::OnTileChanged(wxCommandEvent& evt)
{
	m_tile = m_mapedit->GetSelectedTile();
	m_mapedit->SetDrawTile(m_tile);
	m_tileset->SelectTile(m_tile.GetIndex());
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

void Map2DEditorFrame::OnTilesetSelect(wxCommandEvent& /*evt*/)
{
	if (m_gd == nullptr)
	{
		return;
	}
	SetTileset(m_tileset_select->GetValue().ToStdString());
	m_tileset_select->SetStringSelection(m_map->GetTileset());
}

void Map2DEditorFrame::OnPaletteSelect(wxCommandEvent& /*evt*/)
{
	if (m_gd == nullptr)
	{
		return;
	}
	SetActivePalette(m_palette_select->GetValue().ToStdString());
	m_palette_select->SetStringSelection(m_palette->GetName());
}

void Map2DEditorFrame::InitCombos() const
{
	std::vector<wxString> tileset_list;
	const auto& tilesets = m_gd->GetAllTilesets();
	tileset_list.reserve(tilesets.size());
	m_tileset_list.Clear();
	for (const auto& ts : tilesets)
	{
		tileset_list.push_back(ts.first);
		m_tileset_list.Add(ts.first);
	}
	if (m_tileset_select != nullptr)
	{
		m_tileset_select->Clear();
		m_tileset_select->Append(tileset_list);
	}
	std::vector<wxString> palette_list;
	const auto& palettes = m_gd->GetAllPalettes();
	palette_list.reserve(palettes.size());
	m_palette_list.Clear();
	for (const auto& p : palettes)
	{
		palette_list.push_back(p.first);
		m_palette_list.Add(p.first);
	}
	if (m_palette_select != nullptr)
	{
		m_palette_select->Clear();
		m_palette_select->Append(palette_list);
	}

	if (m_tiles != nullptr && m_tileset_select != nullptr)
	{
		m_tileset_select->SetStringSelection(m_tiles->GetName());
	}
	if (m_palette != nullptr && m_palette_select != nullptr)
	{
		m_palette_select->SetStringSelection(m_palette->GetName());
	}
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

void Map2DEditorFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& /*evt*/) const
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
	status.SetStatusText(wxString::Format("Zoom: %d%%", m_zoom * 100), 2);
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
	if (ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		props.Append(new wxPropertyCategory("Main", "M"));
		props.Append(new wxStringProperty("Name", "N", ""))->Enable(false);
		props.Append(new wxStringProperty("Start Address", "SA", "0x000000"))->Enable(false);
		props.Append(new wxStringProperty("End Address", "EA", "0x000000"))->Enable(false);
		props.Append(new wxFileProperty("Filename", "FN", "untitled.bin"))->Enable(false);
		props.Append(new wxStringProperty("Original Size", "OS", "0 bytes"))->Enable(false);
		props.Append(new wxStringProperty("Uncompressed Size", "US", "0 bytes"))->Enable(false);
		props.Append(new wxPropertyCategory("Map", "Ma"));
		props.Append(new wxEnumProperty("Tileset", "T", m_tileset_list));
		props.Append(new wxEnumProperty("Palette", "P", m_palette_list));
		props.Append(new wxStringProperty("Compression", "C", ""))->Enable(false);
		props.Append(new wxIntProperty("Tile Count", "#", 0))->Enable(false);
		props.Append(new wxIntProperty("Tile Width", "TW", 0))->Enable(false);
		props.Append(new wxIntProperty("Tile Height", "TH", 0))->Enable(false);
		props.Append(new wxIntProperty("Map Width", "W", 0))->Enable(false);
		props.Append(new wxIntProperty("Map Height", "H", 0))->Enable(false);
		props.Append(new wxIntProperty("Map Left", "L", 0));
		props.Append(new wxIntProperty("Map Top", "To", 0));
		props.Append(new wxStringProperty("Base Tile", "B", "0x0000"));
		RefreshProperties(props);
	}
	EditorFrame::InitProperties(props);
}

void Map2DEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		RefreshProperties(props);
	}
}

void Map2DEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr)
	{
		return;
	}

	const wxString& name = property->GetName();
	if (name == "P")
	{
		// Palette change
		SetActivePalette(property->GetValueAsString().ToStdString());
	}
	else if (name == "T")
	{
		// Tileset change
		SetTileset(property->GetValueAsString().ToStdString());
	}
	else if (name == "B")
	{
		uint32_t base;
		if (StrToInt(property->GetValueAsString().ToStdString(), base))
		{
			m_map->SetBase(base);
			RedrawTiles();
		}
	}
	else if (name == "To")
	{
		uint32_t top;
		if (StrToInt(property->GetValueAsString().ToStdString(), top))
		{
			m_map->GetData()->SetTop(top);
		}
	}
	else if (name == "L")
	{
		uint32_t left;
		if (StrToInt(property->GetValueAsString().ToStdString(), left))
		{
			m_map->GetData()->SetLeft(left);
		}
	}
	FireEvent(EVT_PROPERTIES_UPDATE);
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
	auto& viewMenu = AddMenu(menu, 1, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_TILE_NOS, "Tile Numbers", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	auto& toolsMenu = AddMenu(menu, 2, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_TILES, "Tiles", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_TILEMAP_TOOLBAR, "Tilemap Toolbar", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, ID_TOOLS_TILESET_TOOLBAR, "Tileset Toolbar", wxITEM_CHECK);

	wxAuiToolBar* tilemap_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);

	tilemap_tb->AddTool(ID_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	tilemap_tb->AddTool(ID_TOGGLE_TILE_NUMBERS, "Toggle Tile Numbers", ilist.GetImage("tile_nums"), "Toggle Tile Numbers", wxITEM_CHECK);
	tilemap_tb->AddTool(ID_TOGGLE_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Alpha", wxITEM_CHECK);
	tilemap_tb->AddSeparator();
	tilemap_tb->AddTool(ID_INSERT_ROW_BEFORE, "Insert Row Above", ilist.GetImage("insert_row_before"), "Insert Row Above");
	tilemap_tb->AddTool(ID_INSERT_ROW_AFTER, "Insert Row Below", ilist.GetImage("insert_row_after"), "Insert Row Below");
	tilemap_tb->AddTool(ID_DELETE_ROW, "Delete Row", ilist.GetImage("delete_row"), "Delete Row");
	tilemap_tb->AddTool(ID_INSERT_COLUMN_BEFORE, "Insert Column To Left", ilist.GetImage("insert_column_before"), "Insert Column To Left");
	tilemap_tb->AddTool(ID_INSERT_COLUMN_AFTER, "Insert Column To Right", ilist.GetImage("insert_column_after"), "Insert Column To Right");
	tilemap_tb->AddTool(ID_DELETE_COLUMN, "Delete Column", ilist.GetImage("delete_column"), "Delete Column");
	tilemap_tb->AddSeparator();
	tilemap_tb->AddTool(ID_HFLIP_TILE, "Horizontally Flip Tile", ilist.GetImage("hflip"), "Horizontally Flip Tile");
	tilemap_tb->AddTool(ID_VFLIP_TILE, "Vertically Flip Tile", ilist.GetImage("vflip"), "Vertically Flip Tile");
	tilemap_tb->AddTool(ID_TOGGLE_TILE_PRIORITY, "Toggle Tile Priority", ilist.GetImage("priority"), "Toggle Tile Priority");
	tilemap_tb->AddSeparator();
	tilemap_tb->AddTool(ID_SELECT, "Select", ilist.GetImage("mouse"), "Select", wxITEM_RADIO);
	tilemap_tb->AddTool(ID_PENCIL, "Pencil", ilist.GetImage("pencil"), "Pencil", wxITEM_RADIO);
	tilemap_tb->AddSeparator();
	tilemap_tb->AddLabel(wxID_ANY, "Zoom:");
	m_zoomslider = new wxSlider(tilemap_tb, ID_ZOOM_SLIDER, m_zoom, 1, 16, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	tilemap_tb->AddControl(m_zoomslider, "Zoom");
	AddToolbar(m_mgr, *tilemap_tb, "Tilemap", "Tilemap Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));
	wxAuiToolBar* tileset_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	m_tileset_select = new wxComboBox(tileset_tb, ID_TILESET_SELECT, "", wxDefaultPosition, wxDefaultSize, {}, wxCB_DROPDOWN | wxCB_READONLY);
	m_palette_select = new wxComboBox(tileset_tb, ID_PALETTE_SELECT, "", wxDefaultPosition, wxDefaultSize, {}, wxCB_DROPDOWN | wxCB_READONLY);
	m_tileset_zoomslider = new wxSlider(tileset_tb, ID_TILESET_ZOOM, 4, 1, 16, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	tileset_tb->SetToolBitmapSize(wxSize(16, 16));
	tileset_tb->AddLabel(wxID_ANY, "Tileset:");
	tileset_tb->AddControl(m_tileset_select, "Tileset");
	tileset_tb->AddLabel(wxID_ANY, "Palette:");
	tileset_tb->AddControl(m_palette_select, "Palette");
	tileset_tb->AddSeparator();
	tileset_tb->AddTool(ID_TILESET_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	tileset_tb->AddTool(ID_TILESET_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Gridlines", wxITEM_CHECK);
	tileset_tb->AddTool(ID_TILESET_TILE_NUMBERS, "Toggle Tile Numbers", ilist.GetImage("tile_nums"), "Toggle Gridlines", wxITEM_CHECK);
	tileset_tb->AddLabel(wxID_ANY, "Zoom:");
	tileset_tb->AddControl(m_tileset_zoomslider, "Zoom");
	AddToolbar(m_mgr, *tileset_tb, "Tileset", "Tileset Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(2));
	
	InitCombos();
	UpdateUI();

	m_mgr.Update();
}

void Map2DEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_BIN:
		OnExportBin();
		break;
	case ID_FILE_EXPORT_CSV:
		OnExportCsv();
		break;
	case ID_FILE_EXPORT_PNG:
		OnExportPng();
		break;
	case ID_FILE_IMPORT_BIN:
		OnImportBin();
		break;
	case ID_FILE_IMPORT_CSV:
		OnImportCsv();
		break;
	case ID_VIEW_TOGGLE_GRIDLINES:
	case ID_TOGGLE_GRIDLINES:
		m_mapedit->SetBordersEnabled(!m_mapedit->GetBordersEnabled());
		break;
	case ID_VIEW_TOGGLE_TILE_NOS:
	case ID_TOGGLE_TILE_NUMBERS:
		m_mapedit->SetTileNumbersEnabled(!m_mapedit->GetTileNumbersEnabled());
		break;
	case ID_VIEW_TOGGLE_ALPHA:
	case ID_TOGGLE_ALPHA:
		m_mapedit->SetAlphaEnabled(!m_mapedit->GetAlphaEnabled());
		break;
	case ID_TOOLS_TILES:
		SetPaneVisibility(m_tileset, !IsPaneVisible(m_tileset));
		break;
	case ID_TOOLS_TILEMAP_TOOLBAR:
		SetToolbarVisibility("Tilemap", !IsToolbarVisible("Tilemap"));
		break;
	case ID_TOOLS_TILESET_TOOLBAR:
		SetToolbarVisibility("Tileset", !IsToolbarVisible("Tileset"));
		break;
	case ID_INSERT_ROW_BEFORE:
		if (m_mapedit->IsSelectionValid())
		{
			m_mapedit->InsertRow(m_mapedit->GetSelection().y);
			auto sel = m_mapedit->GetSelection();
			sel.y++;
			m_mapedit->SetSelection(sel);
		}
		break;
	case ID_INSERT_ROW_AFTER:
		if (m_mapedit->IsSelectionValid())
		{
			m_mapedit->InsertRow(m_mapedit->GetSelection().y + 1);
		}
		break;
	case ID_DELETE_ROW:
		if (m_mapedit->IsSelectionValid())
		{
			m_mapedit->DeleteRow(m_mapedit->GetSelection().y);
		}
		break;
	case ID_INSERT_COLUMN_BEFORE:
		if (m_mapedit->IsSelectionValid())
		{
			m_mapedit->InsertColumn(m_mapedit->GetSelection().x);
			auto sel = m_mapedit->GetSelection();
			sel.x++;
			m_mapedit->SetSelection(sel);
		}
		break;
	case ID_INSERT_COLUMN_AFTER:
		if (m_mapedit->IsSelectionValid())
		{
			m_mapedit->InsertColumn(m_mapedit->GetSelection().x + 1);
		}
		break;
	case ID_DELETE_COLUMN:
		if (m_mapedit->IsSelectionValid())
		{
			m_mapedit->DeleteColumn(m_mapedit->GetSelection().x);
		}
		break;
	case ID_HFLIP_TILE:
		if (m_mapedit->IsSelectionValid())
		{
			auto tile = m_mapedit->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_HFLIP);
			m_mapedit->SetSelectedTile(tile);
		}
		break;
	case ID_VFLIP_TILE:
		if (m_mapedit->IsSelectionValid())
		{
			auto tile = m_mapedit->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_VFLIP);
			m_mapedit->SetSelectedTile(tile);
		}
		break;
	case ID_TOGGLE_TILE_PRIORITY:
		if (m_mapedit->IsSelectionValid())
		{
			auto tile = m_mapedit->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
			m_mapedit->SetSelectedTile(tile);
		}
		break;
	case ID_SELECT:
		m_mapedit->SetMode(Map2DEditor::Mode::SELECT);
		break;
	case ID_PENCIL:
		m_mapedit->SetMode(Map2DEditor::Mode::PENCIL);
		break;
	case ID_TILESET_ALPHA:
		m_tileset->SetAlphaEnabled(!m_tileset->GetAlphaEnabled());
		break;
	case ID_TILESET_TILE_NUMBERS:
		m_tileset->SetTileNumbersEnabled(!m_tileset->GetTileNumbersEnabled());
		break;
	case ID_TILESET_TOGGLE_GRIDLINES:
		m_tileset->SetBordersEnabled(!m_tileset->GetBordersEnabled());
		break;
	default:
		break;
	}
	UpdateUI();
	FireEvent(EVT_STATUSBAR_UPDATE);
	FireEvent(EVT_PROPERTIES_UPDATE);
	evt.Skip();
}

void Map2DEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	// The toolbar destructor deletes these, but doesn't clear the pointer
	m_palette_select = nullptr;
	m_tileset_select = nullptr;
	m_zoomslider = nullptr;
	EditorFrame::ClearMenu(menu);
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
		switch (m_mapedit->GetMode())
		{
		case Map2DEditor::Mode::SELECT:
			CheckToolbarItem("Tilemap", ID_SELECT, true);
			break;
		case Map2DEditor::Mode::PENCIL:
			CheckToolbarItem("Tilemap", ID_PENCIL, true);
		}
	}
	if (m_tileset != nullptr)
	{
		CheckToolbarItem("Tileset", ID_TILESET_ALPHA, !m_tileset->GetAlphaEnabled());
		CheckToolbarItem("Tileset", ID_TILESET_TOGGLE_GRIDLINES, m_tileset->GetBordersEnabled());
		CheckToolbarItem("Tileset", ID_TILESET_TILE_NUMBERS, m_tileset->GetTileNumbersEnabled());
	}

	if (m_map != nullptr && m_tileset_select != nullptr)
	{
		m_tileset_select->SetStringSelection(m_tiles->GetName());
	}
	if (m_tiles != nullptr && m_palette_select != nullptr)
	{
		m_palette_select->SetStringSelection(m_palette->GetName());
	}
}

void Map2DEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (!m_gd)
	{
		return;
	}
	if (m_tileset_list.GetCount() == 0 || m_palette_list.GetCount() == 0)
	{
		InitCombos();
	}
	if (m_map)
	{
		props.GetGrid()->SetPropertyValue("N", wxString(m_map->GetName()));
		props.GetGrid()->SetPropertyValue("SA", wxString(StrPrintf("0x%06X", m_map->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("EA", wxString(StrPrintf("0x%06X", m_map->GetOrigEndAddress())));
		props.GetGrid()->SetPropertyValue("OS", wxString(StrPrintf("%d bytes", m_map->GetOrigDataLength())));
		props.GetGrid()->SetPropertyValue("FN", wxString(m_map->GetFilename().string()));
		props.GetGrid()->SetPropertyValue("US", wxString(StrPrintf("%d bytes", m_map->GetOrigData()->GetWidth() * m_map->GetOrigData()->GetHeight() * sizeof(uint16_t))));
		props.GetGrid()->SetPropertyValue("#", wxString(StrPrintf("%d", m_map->GetOrigData()->GetWidth() * m_map->GetOrigData()->GetHeight())));
		props.GetGrid()->SetPropertyValue("W", wxString(StrPrintf("%d", m_map->GetData()->GetWidth())));
		props.GetGrid()->SetPropertyValue("H", wxString(StrPrintf("%d", m_map->GetData()->GetHeight())));
		props.GetGrid()->SetPropertyValue("L", wxString(StrPrintf("%d", m_map->GetData()->GetLeft())));
		props.GetGrid()->SetPropertyValue("To", wxString(StrPrintf("%d", m_map->GetData()->GetTop())));
		props.GetGrid()->SetPropertyValue("B", wxString(StrPrintf("0x%04X", m_map->GetData()->GetBase())));

		switch (m_map->GetData()->GetCompression())
		{
		case Tilemap2D::Compression::NONE:
			props.GetGrid()->SetPropertyValue("C", wxString("None"));
			break;
		case Tilemap2D::Compression::LZ77:
			props.GetGrid()->SetPropertyValue("C", wxString("LZ77"));
			break;
		case Tilemap2D::Compression::RLE:
			props.GetGrid()->SetPropertyValue("C", wxString("RLE"));
			break;
		}
	}
	if (m_palette)
	{
		props.GetGrid()->GetProperty("P")->SetChoices(m_palette_list);
		props.GetGrid()->SetPropertyValue("P", wxString(m_palette->GetName()));
	}
	if (m_tiles)
	{
		props.GetGrid()->GetProperty("T")->SetChoices(m_tileset_list);
		props.GetGrid()->SetPropertyValue("T", wxString(m_tiles->GetName()));
		props.GetGrid()->SetPropertyValue("TW", wxString::Format("%lu", m_tiles->GetData()->GetTileWidth()));
		props.GetGrid()->SetPropertyValue("TH", wxString::Format("%lu", m_tiles->GetData()->GetTileHeight()));
	}
}

void Map2DEditorFrame::OnExportBin()
{
	const wxString default_file = m_map->GetName() + m_map->GetData()->GetFileExtension();
	wxFileDialog fd(this, _("Export Map As Binary"), "", default_file, "RLE Compressed Tilemap (*.rle)|*.rle|LZ77 Compressed Tilemap (*.lz77)|*.lz77|Uncompressed Tilemap (*.bin)|*.bin|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportBin(fd.GetPath().ToStdString());
	}
}

void Map2DEditorFrame::OnExportPng()
{
	const wxString default_file = m_map->GetName() + ".png";
	wxFileDialog fd(this, _("Export Map As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPng(fd.GetPath().ToStdString());
	}
}

void Map2DEditorFrame::OnExportCsv()
{
	const wxString default_file = m_map->GetName() + ".csv";
	wxFileDialog fd(this, _("Export Map As CSV"), "", default_file, "CSV File (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportCsv(fd.GetPath().ToStdString());
	}
}

void Map2DEditorFrame::OnImportBin()
{
	wxFileDialog fd(this, _("Import Map From Binary"), "", "", "Map Files (*.bin, *.rle, *.lz77)|*.bin;*.rle;*.lz77"
		"|RLE Compressed Tilemap (*.rle)|*.rle|LZ77 Compressed Tilemap (*.lz77)|*.lz77|Uncompressed Tilemap (*.bin)"
		"|*.bin|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		uint32_t base, width, height;
		std::string input = wxGetTextFromUser("Enter Tilemap Base", "Import Tilemap", "0x0100", this).ToStdString();
		StrToInt(input, base);
		if (Tilemap2D::FromFileExtension(path) == Tilemap2D::Compression::NONE)
		{
			input = wxGetTextFromUser("Enter Tilemap Width", "Import Tilemap", "40", this).ToStdString();
			StrToInt(input, width);
			input = wxGetTextFromUser("Enter Tilemap Height", "Import Tilemap", "3", this).ToStdString();
			StrToInt(input, height);
			ImportBin(path, base & 0xFFFF, width, height);
		}
		else
		{
			ImportBin(path, base & 0xFFFF);
		}
	}
	m_mapedit->RedrawAll();
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void Map2DEditorFrame::OnImportCsv()
{
	wxFileDialog fd(this, _("Import Map From CSV"), "", "", "CSV File (*.csv)|*.csv|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportCsv(path);
	}
	m_mapedit->RedrawAll();
	FireEvent(EVT_PROPERTIES_UPDATE);
}
