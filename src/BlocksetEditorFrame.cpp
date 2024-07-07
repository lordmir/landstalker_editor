#include "BlocksetEditorFrame.h"

enum MENU_IDS
{
	ID_FILE_EXPORT_CBS = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_CBS,
	ID_FILE_IMPORT_CSV,
	ID_VIEW,
	ID_VIEW_TOGGLE_GRIDLINES,
	ID_VIEW_TOGGLE_TILE_NOS,
	ID_VIEW_TOGGLE_ALPHA,
	ID_TOOLS,
	ID_TOOLS_TILES,
	ID_TOOLS_TOOLBAR,
	ID_TOOLS_BLOCK_SELECT,
	ID_TOOLS_TILE_SELECT,
	ID_TOOLS_TILE_DRAW,
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_TILE_NUMBERS,
	ID_TOGGLE_ALPHA,
	ID_INSERT_BLOCK_BEFORE,
	ID_INSERT_BLOCK_AFTER,
	ID_DELETE_BLOCK,
	ID_HFLIP_TILE,
	ID_VFLIP_TILE,
	ID_TOGGLE_TILE_PRIORITY,
	ID_BLOCK_SELECT,
	ID_TILE_SELECT,
	ID_PENCIL,
	ID_ZOOM,
	ID_PALETTE_SELECT
};

wxBEGIN_EVENT_TABLE(BlocksetEditorFrame, wxWindow)
EVT_SLIDER(wxID_ANY, BlocksetEditorFrame::OnZoomChange)
EVT_TOOL(wxID_ANY, BlocksetEditorFrame::OnButtonClicked)
EVT_COMBOBOX(ID_PALETTE_SELECT, BlocksetEditorFrame::OnPaletteSelect)
EVT_COMMAND(wxID_ANY, EVT_BLOCK_SELECT, BlocksetEditorFrame::OnBlockSelect)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, BlocksetEditorFrame::OnTileSelect)
EVT_COMMAND(wxID_ANY, EVT_TILE_SELECT, BlocksetEditorFrame::OnTileSelect)
wxEND_EVENT_TABLE()

BlocksetEditorFrame::BlocksetEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst)
{
	m_mgr.SetManagedWindow(this);

	m_editor = new BlocksetEditorCtrl(this);
	m_tileset = new TilesetEditor(this);

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.4, 0.4);
	m_mgr.AddPane(m_tileset, wxAuiPaneInfo().Right().Layer(1).MinSize(100, 100).BestSize(450, 450).FloatingSize(450, 450).Caption("Tiles"));
	m_mgr.AddPane(m_editor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

BlocksetEditorFrame::~BlocksetEditorFrame()
{
}

void BlocksetEditorFrame::Open(const std::string& blockset_name)
{
	m_blocks = m_gd->GetRoomData()->GetBlockset(blockset_name);
	m_tiles = m_gd->GetRoomData()->GetTileset(m_blocks->GetTileset());
	m_editor->Open(blockset_name);
	m_tileset->Open(m_tiles->GetData());
	m_tileset->SetActivePalette(m_tiles->GetDefaultPalette());
}

void BlocksetEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_editor->SetGameData(gd);
	m_tileset->SetGameData(gd);
	m_gd = gd;
}

void BlocksetEditorFrame::ClearGameData()
{
	m_gd = nullptr;
}

void BlocksetEditorFrame::SetActivePalette(const std::string& name)
{
	m_editor->SetActivePalette(name);
	m_tileset->SetActivePalette(name);
}

void BlocksetEditorFrame::SetDrawTile(const Tile& tile)
{
	m_editor->SetDrawTile(tile);
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void BlocksetEditorFrame::Redraw()
{
	m_editor->RedrawTiles();
}

void BlocksetEditorFrame::RedrawTiles(int index) const
{
	m_editor->RedrawTiles(index);
}

void BlocksetEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_CBS, "Export Blockset as Binary...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_CSV, "Export Blockset as CSV...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_PNG, "Export Blockset as PNG...");
	AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_CBS, "Import Blockset from Binary...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_CSV, "Import Blockset from CSV...");
	auto& viewMenu = AddMenu(menu, 1, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_TILE_NOS, "Tile Numbers", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	auto& toolsMenu = AddMenu(menu, 2, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_TILES, "Tiles Pane", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_TOOLBAR, "Blockset Toolbar", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, wxID_ANY, "", wxITEM_SEPARATOR);
	AddMenuItem(toolsMenu, 3, ID_TOOLS_BLOCK_SELECT, "Block Select Mode", wxITEM_RADIO);
	AddMenuItem(toolsMenu, 4, ID_TOOLS_TILE_SELECT, "Tile Select Mode", wxITEM_RADIO);
	AddMenuItem(toolsMenu, 5, ID_TOOLS_TILE_DRAW, "Tile Draw Mode", wxITEM_RADIO);

	wxAuiToolBar* toolbar = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);

	toolbar->AddTool(ID_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_TILE_NUMBERS, "Toggle Tile Numbers", ilist.GetImage("tile_nums"), "Toggle Tile Numbers", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Alpha", wxITEM_CHECK);
	toolbar->AddSeparator();
	toolbar->AddTool(ID_INSERT_BLOCK_BEFORE, "Insert Block Before", ilist.GetImage("insert_before"), "Insert Block Before");
	toolbar->AddTool(ID_INSERT_BLOCK_AFTER, "Insert Block After", ilist.GetImage("insert_after"), "Insert Block After");
	toolbar->AddTool(ID_DELETE_BLOCK, "Delete Block", ilist.GetImage("delete_tile"), "Delete Block");
	toolbar->AddSeparator();
	toolbar->AddTool(ID_HFLIP_TILE, "Horizontally Flip Tile", ilist.GetImage("hflip"), "Horizontally Flip Tile");
	toolbar->AddTool(ID_VFLIP_TILE, "Vertically Flip Tile", ilist.GetImage("vflip"), "Vertically Flip Tile");
	toolbar->AddTool(ID_TOGGLE_TILE_PRIORITY, "Toggle Tile Priority", ilist.GetImage("priority"), "Toggle Tile Priority");
	toolbar->AddSeparator();
	toolbar->AddTool(ID_BLOCK_SELECT, "Block Select", ilist.GetImage("sel_block"), "Block Select", wxITEM_RADIO);
	toolbar->AddTool(ID_TILE_SELECT, "Tile Select", ilist.GetImage("sel_tile"), "Tile Select", wxITEM_RADIO);
	toolbar->AddTool(ID_PENCIL, "Draw Tiles", ilist.GetImage("pencil"), "Draw Tiles", wxITEM_RADIO);
	toolbar->AddSeparator();
	toolbar->AddLabel(wxID_ANY, "Zoom:");
	m_zoomslider = new wxSlider(toolbar, ID_ZOOM, m_zoom, 1, 8, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	toolbar->AddControl(m_zoomslider, "Zoom");
	m_palette_select = new wxComboBox(toolbar, ID_PALETTE_SELECT, "", wxDefaultPosition, wxDefaultSize, {}, wxCB_DROPDOWN | wxCB_READONLY);
	toolbar->SetToolBitmapSize(wxSize(16, 16));
	toolbar->AddLabel(wxID_ANY, "Palette:");
	toolbar->AddControl(m_palette_select, "Palette");

	AddToolbar(m_mgr, *toolbar, "Blockset", "Blockset Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));

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

	if (m_palette != nullptr && m_palette != nullptr)
	{
		m_palette_select->SetStringSelection(m_palette->GetName());
	}
	UpdateUI();

	m_mgr.Update();
}

void BlocksetEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_FILE_EXPORT_CBS:
		OnExportBin();
		break;
	case ID_FILE_EXPORT_CSV:
		OnExportCsv();
		break;
	case ID_FILE_EXPORT_PNG:
		OnExportPng();
		break;
	case ID_FILE_IMPORT_CBS:
		OnImportBin();
		break;
	case ID_FILE_IMPORT_CSV:
		OnImportCsv();
		break;
	default:
		break;
	}
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	evt.Skip();
}

void BlocksetEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	EditorFrame::ClearMenu(menu);
	// The toolbar destructor deletes these, but doesn't clear the pointer
	m_palette_select = nullptr;
}

void BlocksetEditorFrame::ExportBin(const std::string& filename) const
{
	ByteVector bytes(65536);
	auto sz = BlocksetCmp::Encode(*m_blocks->GetData(), bytes.data(), 65536);
	bytes.resize(sz);
	WriteBytes(bytes, filename);
}

void BlocksetEditorFrame::ExportPng(const std::string& filename) const
{
	int width = std::min<int>(16, m_blocks->GetData()->size());
	int height = std::max<int>(1, (m_blocks->GetData()->size() + 15) / 16);
	int blockwidth = MapBlock::GetBlockWidth() * m_tiles->GetData()->GetTileWidth();
	int blockheight = MapBlock::GetBlockHeight() * m_tiles->GetData()->GetTileHeight();
	int pixelwidth = blockwidth * width;
	int pixelheight = blockheight * height;
	ImageBuffer buf(pixelwidth, pixelheight);
	std::size_t i = 0;
	for (int y = 0; y < pixelheight; y += blockheight)
	{
		for (int x = 0; x < pixelwidth; x += blockwidth, ++i)
		{
			if (i < m_blocks->GetData()->size())
			{
				buf.InsertBlock(x, y, 0, m_blocks->GetData()->at(i), *m_tiles->GetData());
			}
		}
	}
	auto palette = m_gd->GetPalette(m_editor->GetActivePalette());
	buf.WritePNG(filename, { palette->GetData() }, true);
}

void BlocksetEditorFrame::ExportCsv(const std::string& filename) const
{
	std::ofstream fs(filename, std::ios::out | std::ios::trunc);

	for (std::size_t y = 0; y < m_blocks->GetData()->size(); ++y)
	{
		for (std::size_t x = 0; x < 4; ++x)
		{
			fs << StrPrintf("0x%04X", m_blocks->GetData()->at(y).GetTile(x).GetTileValue());
			if (x == 3)
			{
				fs << std::endl;
			}
			else
			{
				fs << ",";
			}
		}
	}
}

void BlocksetEditorFrame::ImportBin(const std::string& filename)
{
	ByteVector bytes = ReadBytes(filename);
	m_blocks->GetData()->clear();
	BlocksetCmp::Decode(bytes.data(), bytes.size(), *m_blocks->GetData());
	m_editor->RedrawTiles();
}

void BlocksetEditorFrame::ImportCsv(const std::string& filename)
{
	std::ifstream fs(filename, std::ios::in);

	std::vector<MapBlock> blocks;
	std::vector<Tile> tiles(4);
	std::string row;
	std::string cell;
	uint32_t val;
	while (std::getline(fs, row))
	{
		std::istringstream ss(row);
		for (int x = 0; x < 4; ++x)
		{
			std::getline(ss, cell, ',');
			StrToInt(cell, val);
			tiles[x] = val;
		}
		blocks.push_back(MapBlock(tiles.cbegin(), tiles.cend()));
	}
	*m_blocks->GetData() = blocks;
	m_editor->RedrawTiles();
	UpdateUI();
}

void BlocksetEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	m_zoom = m_zoomslider->GetValue();
	m_editor->SetPixelSize(m_zoomslider->GetValue());
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void BlocksetEditorFrame::OnButtonClicked(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_TOGGLE_GRIDLINES:
		m_editor->SetBordersEnabled(!m_editor->GetBordersEnabled());
		m_editor->ForceRedraw();
		break;
	case ID_TOGGLE_TILE_NUMBERS:
		m_editor->SetTileNumbersEnabled(!m_editor->GetTileNumbersEnabled());
		m_editor->ForceRedraw();
		break;
	case ID_TOGGLE_ALPHA:
		m_editor->SetAlphaEnabled(!m_editor->GetAlphaEnabled());
		m_editor->ForceRedraw();
		break;
	case ID_INSERT_BLOCK_BEFORE:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->InsertBlock(m_editor->GetBlockSelection());
			m_editor->SetBlockSelection(m_editor->GetBlockSelection() + 1);
		}
		break;
	case ID_INSERT_BLOCK_AFTER:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->InsertBlock(m_editor->GetBlockSelection() + 1);
		}
		break;
	case ID_DELETE_BLOCK:
		m_editor->DeleteBlock(m_editor->GetBlockSelection());
		break;
	case ID_HFLIP_TILE:
		m_editor->ToggleSelectedHFlip();
		break;
	case ID_VFLIP_TILE:
		m_editor->ToggleSelectedVFlip();
		break;
	case ID_TOGGLE_TILE_PRIORITY:
		m_editor->ToggleSelectedPriority();
		break;
	case ID_BLOCK_SELECT:
		m_editor->SetMode(BlocksetEditorCtrl::Mode::BLOCK_SELECT);
		break;
	case ID_TILE_SELECT:
		m_editor->SetMode(BlocksetEditorCtrl::Mode::TILE_SELECT);
		break;
	case ID_PENCIL:
		m_editor->SetMode(BlocksetEditorCtrl::Mode::PENCIL);
		break;
	default:
		break;
	}
	UpdateUI();
	evt.Skip();
}

void BlocksetEditorFrame::OnPaletteSelect(wxCommandEvent& evt)
{
	if (m_gd == nullptr)
	{
		return;
	}
	SetActivePalette(m_palette_select->GetValue().ToStdString());
	m_palette_select->SetStringSelection(m_editor->GetActivePalette());
	evt.Skip();
}

void BlocksetEditorFrame::OnBlockSelect(wxCommandEvent& evt)
{
	UpdateUI();
	evt.Skip();
}

void BlocksetEditorFrame::OnTileSelect(wxCommandEvent& evt)
{
	int tile_id = std::stoi(evt.GetString().ToStdString());
	if (tile_id >= 0)
	{
		m_editor->SetMode(BlocksetEditorCtrl::Mode::PENCIL);
		m_editor->SetDrawTile(tile_id);
		m_tileset->SelectTile(tile_id);
	}
	UpdateUI();
	evt.Skip();
}

void BlocksetEditorFrame::UpdateUI() const
{
	CheckMenuItem(ID_TOOLS_TILES, IsPaneVisible(m_tileset));
	CheckMenuItem(ID_TOOLS_TOOLBAR, IsToolbarVisible("Blockset"));
	if (m_editor != nullptr)
	{
		CheckMenuItem(ID_VIEW_TOGGLE_ALPHA, !m_editor->GetAlphaEnabled());
		CheckToolbarItem("Blockset", ID_TOGGLE_ALPHA, !m_editor->GetAlphaEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_GRIDLINES, m_editor->GetBordersEnabled());
		CheckToolbarItem("Blockset", ID_TOGGLE_GRIDLINES, m_editor->GetBordersEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_TILE_NOS, m_editor->GetTileNumbersEnabled());
		CheckToolbarItem("Tilemap", ID_TOGGLE_TILE_NUMBERS, m_editor->GetTileNumbersEnabled());
		switch (m_editor->GetMode())
		{
		case BlocksetEditorCtrl::Mode::BLOCK_SELECT:
			CheckMenuItem(ID_TOOLS_BLOCK_SELECT, true);
			CheckToolbarItem("Blockset", ID_BLOCK_SELECT, true);
			EnableToolbarItem("Blockset", ID_DELETE_BLOCK, m_editor->IsBlockSelectionValid());
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_BEFORE, m_editor->IsBlockSelectionValid());
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_AFTER, m_editor->IsBlockSelectionValid());
			EnableToolbarItem("Blockset", ID_VFLIP_TILE, false);
			EnableToolbarItem("Blockset", ID_HFLIP_TILE, false);
			EnableToolbarItem("Blockset", ID_TOGGLE_TILE_PRIORITY, false);
			break;
		case BlocksetEditorCtrl::Mode::TILE_SELECT:
			CheckMenuItem(ID_TOOLS_TILE_SELECT, true);
			CheckToolbarItem("Blockset", ID_TILE_SELECT, true);
			EnableToolbarItem("Blockset", ID_DELETE_BLOCK, false);
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_BEFORE, false);
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_AFTER, false);
			EnableToolbarItem("Blockset", ID_VFLIP_TILE, m_editor->IsTileSelectionValid());
			EnableToolbarItem("Blockset", ID_HFLIP_TILE, m_editor->IsTileSelectionValid());
			EnableToolbarItem("Blockset", ID_TOGGLE_TILE_PRIORITY, m_editor->IsTileSelectionValid());
			break;
		case BlocksetEditorCtrl::Mode::PENCIL:
			CheckMenuItem(ID_TOOLS_TILE_DRAW, true);
			CheckToolbarItem("Blockset", ID_PENCIL, true);
			EnableToolbarItem("Blockset", ID_DELETE_BLOCK, false);
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_BEFORE, false);
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_AFTER, false);
			EnableToolbarItem("Blockset", ID_VFLIP_TILE, false);
			EnableToolbarItem("Blockset", ID_HFLIP_TILE, false);
			EnableToolbarItem("Blockset", ID_TOGGLE_TILE_PRIORITY, false);
			break;
		}
		if (m_editor != nullptr && m_palette_select != nullptr)
		{
			m_palette_select->SetStringSelection(m_editor->GetActivePalette());
		}
		if (m_zoomslider != nullptr)
		{
			m_zoomslider->SetValue(m_editor->GetPixelSize());
		}
	}
}

void BlocksetEditorFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 2);
}

void BlocksetEditorFrame::OnExportBin()
{
	const wxString default_file = m_blocks->GetName() + ".cbs";
	wxFileDialog fd(this, _("Export Blockset As Binary"), "", default_file, "Compressed Blockset file (*.cbs)|*.cbs|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportBin(fd.GetPath().ToStdString());
	}
}

void BlocksetEditorFrame::OnExportPng()
{
	const wxString default_file = m_blocks->GetName() + ".png";
	wxFileDialog fd(this, _("Export Map As PNG"), "", default_file, "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportPng(fd.GetPath().ToStdString());
	}
}

void BlocksetEditorFrame::OnExportCsv()
{
	const wxString default_file = m_blocks->GetName() + ".csv";
	wxFileDialog fd(this, _("Export Blockset As CSV"), "", default_file, "Comma-Separated Values file (*.csv)|*.csv|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		ExportCsv(fd.GetPath().ToStdString());
	}
}

void BlocksetEditorFrame::OnImportBin()
{
	wxFileDialog fd(this, _("Import Blockset From CBS"), "", "", "Compressed Blockset file (*.cbs)|*.cbs|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportBin(path);
	}
}

void BlocksetEditorFrame::OnImportCsv()
{
	wxFileDialog fd(this, _("Import Blockset From CSV"), "", "", "Comma-Separated Values file (*.csv)|*.csv|All Files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		ImportCsv(path);
	}
}

void BlocksetEditorFrame::FireUpdateStatusEvent(const std::string& data, int pane)
{
	wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
	evt.SetString(data);
	evt.SetInt(pane);
	evt.SetClientData(GetParent());
	wxPostEvent(GetParent(), evt);
}
