#include <user_interface/blockset/include/BlocksetEditorFrame.h>

enum MENU_IDS
{
	ID_FILE_EXPORT_CBS = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_CBS,
	ID_FILE_IMPORT_CSV,
	ID_EDIT,
	ID_EDIT_CUT,
	ID_EDIT_COPY,
	ID_EDIT_PASTE,
	ID_EDIT_SWAP,
	ID_EDIT_CLEAR,
	ID_VIEW,
	ID_VIEW_TOGGLE_GRIDLINES,
	ID_VIEW_TOGGLE_TILE_NOS,
	ID_VIEW_TOGGLE_ALPHA,
	ID_TOOLS,
	ID_TOOLS_TILES,
	ID_TOOLS_TOOLBAR,
	ID_TOOLS_SEP,
	ID_TOOLS_BLOCK_SELECT,
	ID_TOOLS_TILE_SELECT,
	ID_TOOLS_TILE_DRAW,
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_TILE_NUMBERS,
	ID_TOGGLE_ALPHA,
	ID_CUT,
	ID_COPY,
	ID_PASTE,
	ID_SWAP,
	ID_CLEAR,
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
	ID_PALETTE_SELECT,
	ID_TILE_UP,
	ID_TILE_DOWN,
	ID_TILE_LEFT,
	ID_TILE_RIGHT,
	ID_BLOCK_UP,
	ID_BLOCK_DOWN,
	ID_BLOCK_LEFT,
	ID_BLOCK_RIGHT,
	ID_CLEAR_SELECTION,
	ID_SELECT_TILE,
	ID_DRAW_TILE
};

wxBEGIN_EVENT_TABLE(BlocksetEditorFrame, wxWindow)
EVT_KEY_DOWN(BlocksetEditorFrame::OnKeyPress)
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
	FireEvent(EVT_PROPERTIES_UPDATE);

	this->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(BlocksetEditorFrame::OnKeyPress), nullptr, this);
	m_editor->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(BlocksetEditorFrame::OnKeyPress), nullptr, this);
	m_tileset->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(BlocksetEditorFrame::OnKeyPress), nullptr, this);
}

BlocksetEditorFrame::~BlocksetEditorFrame()
{
	this->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(BlocksetEditorFrame::OnKeyPress), nullptr, this);
	m_editor->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(BlocksetEditorFrame::OnKeyPress), nullptr, this);
	m_tileset->Disconnect(wxEVT_KEY_DOWN, wxKeyEventHandler(BlocksetEditorFrame::OnKeyPress), nullptr, this);
}

void BlocksetEditorFrame::Open(const std::string& blockset_name)
{
	m_blocks = m_gd->GetRoomData()->GetBlockset(blockset_name);
	m_tiles = m_gd->GetRoomData()->GetTileset(m_blocks->GetTileset());
	m_editor->Open(blockset_name);
	m_tileset->Open(m_tiles->GetData());
	m_tileset->SetActivePalette(m_tiles->GetDefaultPalette());
	m_palette = m_gd->GetRoomData()->GetRoomPalette(m_tileset->GetActivePalette());
}

void BlocksetEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_editor->SetGameData(gd);
	m_tileset->SetGameData(gd);
	m_gd = gd;
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void BlocksetEditorFrame::ClearGameData()
{
	m_gd = nullptr;
}

void BlocksetEditorFrame::SetActivePalette(const std::string& name)
{
	m_editor->SetActivePalette(name);
	m_tileset->SetActivePalette(name);
	m_palette = m_gd->GetRoomData()->GetRoomPalette(name);
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
	auto& editMenu = AddMenu(menu, 1, ID_EDIT, "Edit");
	AddMenuItem(editMenu, 0, ID_EDIT_CUT, "Cut");
	AddMenuItem(editMenu, 1, ID_EDIT_COPY, "Copy");
	AddMenuItem(editMenu, 2, ID_EDIT_PASTE, "Paste");
	AddMenuItem(editMenu, 3, ID_EDIT_SWAP, "Swap");
	AddMenuItem(editMenu, 4, ID_EDIT_CLEAR, "Clear");
	auto& viewMenu = AddMenu(menu, 2, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_TILE_NOS, "Tile Numbers", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	auto& toolsMenu = AddMenu(menu, 3, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_TILES, "Tiles Pane", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_TOOLBAR, "Blockset Toolbar", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, ID_TOOLS_SEP, "", wxITEM_SEPARATOR);
	AddMenuItem(toolsMenu, 3, ID_TOOLS_BLOCK_SELECT, "Block Select Mode", wxITEM_RADIO);
	AddMenuItem(toolsMenu, 4, ID_TOOLS_TILE_SELECT, "Tile Select Mode", wxITEM_RADIO);
	AddMenuItem(toolsMenu, 5, ID_TOOLS_TILE_DRAW, "Tile Draw Mode", wxITEM_RADIO);

	wxAuiToolBar* toolbar = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);

	toolbar->AddTool(ID_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_TILE_NUMBERS, "Toggle Tile Numbers", ilist.GetImage("tile_nums"), "Toggle Tile Numbers", wxITEM_CHECK);
	toolbar->AddTool(ID_TOGGLE_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Alpha", wxITEM_CHECK);
	toolbar->AddSeparator();
	toolbar->AddTool(ID_CUT, "Cut", ilist.GetImage("cut"), "Cut");
	toolbar->AddTool(ID_COPY, "Copy", ilist.GetImage("copy"), "Copy");
	toolbar->AddTool(ID_PASTE, "Paste", ilist.GetImage("paste"), "Paste");
	toolbar->AddTool(ID_SWAP, "Swap", ilist.GetImage("swap"), "Swap");
	toolbar->AddTool(ID_CLEAR, "Clear", ilist.GetImage("delete"), "Clear");
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
	InitPaletteList();
	if (m_palette_select != nullptr)
	{
		m_palette_select->Clear();
		m_palette_select->Append(m_palette_vec);
	}

	if (m_palette_select != nullptr && m_palette != nullptr)
	{
		m_palette_select->SetStringSelection(m_palette->GetName());
	}
	UpdateUI();
	m_mgr.Update();
}

void BlocksetEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	ProcessEvent(evt.GetId());
	evt.Skip();
}

void BlocksetEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	// The toolbar destructor deletes these, but doesn't clear the pointer
	m_palette_select = nullptr;
	m_zoomslider = nullptr;
	EditorFrame::ClearMenu(menu);
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

void BlocksetEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if (ArePropsInitialised() == false)
	{
		props.GetGrid()->Clear();
		props.Append(new wxPropertyCategory("Main", "M"));
		props.Append(new wxStringProperty("Name", "Name", ""))->Enable(false);
		props.Append(new wxStringProperty("Tileset", "Tileset", ""))->Enable(false);
		props.Append(new wxStringProperty("Primary Blockset", "Primary Blockset", ""))->Enable(false);// m_gd->GetRoomData()->GetTileset(m_blocks->GetTileset())))->Enable(false);
		props.Append(new wxEnumProperty("Palette", "Palette", m_palette_list));
		props.Append(new wxIntProperty("Block Count", "Block Count", 0))->Enable(false);
		props.Append(new wxStringProperty("Start Address", "Start Address", "0x000000"))->Enable(false);
		props.Append(new wxStringProperty("End Address", "End Address", "0x000000"))->Enable(false);
		props.Append(new wxFileProperty("Filename", "Filename", "untitled.cbs"))->Enable(false);
		props.Append(new wxStringProperty("Original Size", "Original Size", "0 bytes"))->Enable(false);
		props.Append(new wxStringProperty("Uncompressed Size", "Uncompressed Size", "0 bytes"))->Enable(false);
		RefreshProperties(props);
	}
	EditorFrame::InitProperties(props);
}

void BlocksetEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_gd && m_blocks && m_palette)
	{
		InitPaletteList();
		props.GetGrid()->SetPropertyValue("Name", _(m_blocks->GetName()));
		props.GetGrid()->SetPropertyValue("Tileset", _(m_gd->GetRoomData()->GetTileset(m_blocks->GetTileset())->GetName()));
		props.GetGrid()->SetPropertyValue("Primary Blockset", _(m_gd->GetRoomData()->GetBlockset(m_blocks->GetTileset(), m_blocks->GetPrimary(), 0)->GetName()));
		props.GetGrid()->GetProperty("Palette")->SetChoices(m_palette_list);
		props.GetGrid()->GetProperty("Palette")->SetValue(m_palette->GetName());
		props.GetGrid()->SetPropertyValue("Block Count", static_cast<int>(m_blocks->GetData()->size()));
		props.GetGrid()->SetPropertyValue("Start Address", _(Hex(m_blocks->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("End Address", _(Hex(m_blocks->GetEndAddress())));
		props.GetGrid()->SetPropertyValue("Filename", _(m_blocks->GetFilename().string()));
		props.GetGrid()->SetPropertyValue("Original Size", wxString::Format("%lu bytes", m_blocks->GetOrigBytes()->size()));
		props.GetGrid()->SetPropertyValue("Uncompressed Size", wxString::Format("%lu bytes", m_blocks->GetData()->size() * 8));
	}
}

void BlocksetEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		RefreshProperties(props);
	}
}

void BlocksetEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
{
	wxPGProperty* property = evt.GetProperty();
	if (property == nullptr)
	{
		return;
	}
	if (m_gd == nullptr)
	{
		return;
	}

	const wxString& name = property->GetName();
	if (name == "Palette")
	{
		SetActivePalette(property->GetValueAsString().ToStdString());
		m_palette_select->SetStringSelection(m_editor->GetActivePalette());
	}

	FireEvent(EVT_PROPERTIES_UPDATE);
}

void BlocksetEditorFrame::InitPaletteList() const
{
	if (m_gd && m_palette_list.GetCount() == 0)
	{
		const auto& palettes = m_gd->GetAllPalettes();
		m_palette_vec.reserve(palettes.size());
		m_palette_list.Clear();
		for (const auto& p : palettes)
		{
			m_palette_vec.push_back(p.first);
			m_palette_list.Add(p.first);
		}
	}
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
	ProcessEvent(evt.GetId());
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

	FireEvent(EVT_PROPERTIES_UPDATE);
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

void BlocksetEditorFrame::OnKeyPress(wxKeyEvent& evt)
{
	switch (evt.GetKeyCode())
	{
	case WXK_LEFT:
	case 'a':
	case 'A':
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT)
		{
			ProcessEvent(ID_BLOCK_LEFT);
		}
		else
		{
			ProcessEvent(ID_TILE_LEFT);
		}
		break;
	case WXK_RIGHT:
	case 'd':
	case 'D':
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT)
		{
			ProcessEvent(ID_BLOCK_RIGHT);
		}
		else
		{
			ProcessEvent(ID_TILE_RIGHT);
		}
		break;
	case WXK_UP:
	case 'w':
	case 'W':
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT)
		{
			ProcessEvent(ID_BLOCK_UP);
		}
		else
		{
			ProcessEvent(ID_TILE_UP);
		}
		break;
	case WXK_DOWN:
	case 's':
	case 'S':
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT)
		{
			ProcessEvent(ID_BLOCK_DOWN);
		}
		else
		{
			ProcessEvent(ID_TILE_DOWN);
		}
		break;
	case WXK_ESCAPE:
		ProcessEvent(ID_CLEAR_SELECTION);
		break;
	case WXK_DELETE:
		if (evt.GetModifiers() == wxMOD_SHIFT)
		{
			if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT && m_editor->IsBlockSelectionValid())
			{
				ProcessEvent(ID_DELETE_BLOCK);
			}
		}
		else
		{
			ProcessEvent(ID_CLEAR);
		}
		break;
	case WXK_INSERT:
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT)
		{
			if (evt.GetModifiers() == 0 && m_editor->IsBlockSelectionValid())
			{
				ProcessEvent(ID_INSERT_BLOCK_BEFORE);
			}
			else if (evt.GetModifiers() == wxMOD_SHIFT && m_editor->IsBlockSelectionValid())
			{
				ProcessEvent(ID_INSERT_BLOCK_AFTER);
			}
		}
		break;
	case 'p':
	case 'P':
		if (evt.GetModifiers() == wxMOD_CONTROL)
		{
			ProcessEvent(ID_SWAP);
		}
		else
		{
			ProcessEvent(ID_TOGGLE_TILE_PRIORITY);
		}
		break;
	case 'v':
	case 'V':
		if (evt.GetModifiers() == wxMOD_CONTROL)
		{
			ProcessEvent(ID_PASTE);
		}
		else
		{
			ProcessEvent(ID_VFLIP_TILE);
		}
		break;
	case 'h':
	case 'H':
		ProcessEvent(ID_HFLIP_TILE);
		break;
	case WXK_SPACE:
		ProcessEvent(ID_DRAW_TILE);
		break;
	case 'c':
	case 'C':
		if (evt.GetModifiers() == wxMOD_CONTROL)
		{
			ProcessEvent(ID_COPY);
		}
		else
		{
			ProcessEvent(ID_SELECT_TILE);
		}
		break;
	case 'x':
	case 'X':
		if (evt.GetModifiers() == wxMOD_CONTROL)
		{
			ProcessEvent(ID_CUT);
		}
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case WXK_NUMPAD0:
	case WXK_NUMPAD1:
	case WXK_NUMPAD2:
	case WXK_NUMPAD3:
	case WXK_NUMPAD4:
	case WXK_NUMPAD5:
	case WXK_NUMPAD6:
	case WXK_NUMPAD7:
	case WXK_NUMPAD8:
	case WXK_NUMPAD9:
	{
		int digit = evt.GetKeyCode() >= '0' && evt.GetKeyCode() <= '9' ? evt.GetKeyCode() - '0' : evt.GetKeyCode() - WXK_NUMPAD0;
		int tile = m_tileset->IsSelectionValid() ? m_tileset->GetSelectedTile().GetIndex() * 10 + digit : digit;
		while (tile >= m_tileset->GetTilemapSize() && tile > 0)
		{
			tile %= static_cast<int>(pow(10.0, floor(log10(tile))));
		}
		m_editor->SetDrawTile(tile);
		m_tileset->SelectTile(tile);
		break;
	}
	case 'b':
	case 'B':
		ProcessEvent(ID_BLOCK_SELECT);
		break;
	case 't':
	case 'T':
		ProcessEvent(ID_TILE_SELECT);
		break;
	case 'r':
	case 'R':
		ProcessEvent(ID_PENCIL);
		break;
	}
	evt.Skip();
}

void BlocksetEditorFrame::ProcessEvent(int id)
{
	int t = -1;
	int b = -1;
	if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::TILE_SELECT)
	{
		b = m_editor->GetBlockSelection();
		t = m_editor->GetTileSelection();
	}
	else if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::PENCIL)
	{
		b = m_editor->GetBlockHover();
		t = m_editor->GetTileHover();
	}
	switch (id)
	{
	case ID_TOOLS_TILES:
		SetPaneVisibility(m_tileset, !IsPaneVisible(m_tileset));
		break;
	case ID_TOOLS_TOOLBAR:
		SetToolbarVisibility("Blockset", !IsToolbarVisible("Blockset"));
		break;
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
	case ID_TOGGLE_GRIDLINES:
	case ID_VIEW_TOGGLE_GRIDLINES:
		m_editor->SetBordersEnabled(!m_editor->GetBordersEnabled());
		m_editor->ForceRedraw();
		break;
	case ID_TOGGLE_TILE_NUMBERS:
	case ID_VIEW_TOGGLE_TILE_NOS:
		m_editor->SetTileNumbersEnabled(!m_editor->GetTileNumbersEnabled());
		m_editor->ForceRedraw();
		break;
	case ID_TOGGLE_ALPHA:
	case ID_VIEW_TOGGLE_ALPHA:
		m_editor->SetAlphaEnabled(!m_editor->GetAlphaEnabled());
		m_editor->ForceRedraw();
		break;
	case ID_CUT:
	case ID_EDIT_CUT:
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT && m_editor->IsBlockSelectionValid())
		{
			m_blockclipboard = m_editor->GetSelectedBlock();
			m_editor->SetSelectedBlock(MapBlock());
		}
		else if (t != -1 && b != -1)
		{
			m_tileclipboard = m_editor->GetTile(b, t);
			m_editor->SetTile(b, t, Tile());
		}
		break;
	case ID_COPY:
	case ID_EDIT_COPY:
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT && m_editor->IsBlockSelectionValid())
		{
			m_blockclipboard = m_editor->GetSelectedBlock();
		}
		else if (t != -1 && b != -1)
		{
			m_tileclipboard = m_editor->GetTile(b, t);
		}
		break;
	case ID_PASTE:
	case ID_EDIT_PASTE:
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT && m_editor->IsBlockSelectionValid() && m_blockclipboard.has_value())
		{
			m_editor->SetSelectedBlock(*m_blockclipboard);
		}
		else if (t != -1 && b != -1 && m_tileclipboard.has_value())
		{
			m_editor->SetTile(b, t, *m_tileclipboard);
		}
		break;
	case ID_SWAP:
	case ID_EDIT_SWAP:
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT && m_editor->IsBlockSelectionValid())
		{
			if (m_blockswap == -1)
			{
				m_blockswap = m_editor->GetBlockSelection();
				m_tileswap = -1;
			}
			else
			{
				auto temp = m_editor->GetBlock(m_blockswap);
				m_editor->SetBlock(m_blockswap, m_editor->GetSelectedBlock());
				m_editor->SetSelectedBlock(temp);
				m_blockswap = -1;
				m_tileswap = -1;
			}
		}
		else if (t != -1 && b != -1)
		{
			if (m_tileswap == -1 || m_blockswap == -1)
			{
				m_blockswap = b;
				m_tileswap = t;
			}
			else
			{
				auto temp = m_editor->GetTile(m_blockswap, m_tileswap);
				m_editor->SetTile(m_blockswap, m_tileswap, m_editor->GetTile(b, t));
				m_editor->SetTile(b, t, temp);
				m_blockswap = -1;
				m_tileswap = -1;
			}
		}
		break;
	case ID_CLEAR:
	case ID_EDIT_CLEAR:
		if (m_editor->GetMode() == BlocksetEditorCtrl::Mode::BLOCK_SELECT && m_editor->IsBlockSelectionValid())
		{
			m_editor->SetSelectedBlock(MapBlock());
		}
		else if (t != -1 && b != -1)
		{
			m_editor->SetTile(b, t, Tile());
		}
		break;
	case ID_INSERT_BLOCK_BEFORE:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->InsertBlock(m_editor->GetBlockSelection());
			m_editor->SetBlockSelection(m_editor->GetBlockSelection() + 1);
			UpdateUI();
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
		break;
	case ID_INSERT_BLOCK_AFTER:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->InsertBlock(m_editor->GetBlockSelection() + 1);
			UpdateUI();
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
		break;
	case ID_DELETE_BLOCK:
		m_editor->DeleteBlock(m_editor->GetBlockSelection());
		UpdateUI();
		FireEvent(EVT_PROPERTIES_UPDATE);
		break;
	case ID_HFLIP_TILE:
	{
		if (m_editor->IsTileIndexValid(t) && m_editor->IsBlockIndexValid(b))
		{
			m_editor->ToggleHFlip(b, t);
		}
		break;
	}
	case ID_VFLIP_TILE:
	{
		if (m_editor->IsTileIndexValid(t) && m_editor->IsBlockIndexValid(b))
		{
			m_editor->ToggleVFlip(b, t);
		}
		break;
	}
	case ID_TOGGLE_TILE_PRIORITY:
	{
		if (m_editor->IsTileIndexValid(t) && m_editor->IsBlockIndexValid(b))
		{
			m_editor->TogglePriority(b, t);
		}
		break;
	}
	case ID_BLOCK_SELECT:
	case ID_TOOLS_BLOCK_SELECT:
		m_editor->SetMode(BlocksetEditorCtrl::Mode::BLOCK_SELECT);
		UpdateUI();
		break;
	case ID_TILE_SELECT:
	case ID_TOOLS_TILE_SELECT:
		m_editor->SetMode(BlocksetEditorCtrl::Mode::TILE_SELECT);
		UpdateUI();
		break;
	case ID_PENCIL:
	case ID_TOOLS_TILE_DRAW:
		m_editor->SetMode(BlocksetEditorCtrl::Mode::PENCIL);
		UpdateUI();
		break;
	case ID_BLOCK_UP:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->SetBlockSelection(std::clamp<int16_t>(m_editor->GetBlockSelection() - m_editor->GetControlBlockWidth(), 0, m_editor->GetBlockmapSize() - 1));
		}
		else
		{
			m_editor->SetBlockSelection(0);
		}
		break;
	case ID_BLOCK_DOWN:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->SetBlockSelection(std::clamp<int16_t>(m_editor->GetBlockSelection() + m_editor->GetControlBlockWidth(), 0, m_editor->GetBlockmapSize() - 1));
		}
		else
		{
			m_editor->SetBlockSelection(0);
		}
		break;
	case ID_BLOCK_LEFT:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->SetBlockSelection(std::clamp<int16_t>(m_editor->GetBlockSelection() - 1, 0, m_editor->GetBlockmapSize() - 1));
		}
		else
		{
			m_editor->SetBlockSelection(0);
		}
		break;
	case ID_BLOCK_RIGHT:
		if (m_editor->IsBlockSelectionValid())
		{
			m_editor->SetBlockSelection(std::clamp<int16_t>(m_editor->GetBlockSelection() + 1, 0, m_editor->GetBlockmapSize() - 1));
		}
		else
		{
			m_editor->SetBlockSelection(0);
		}
		break;
	case ID_TILE_UP:
		if (m_editor->IsTileHoverValid())
		{
			int block = m_editor->GetBlockHover();
			int tile = m_editor->GetTileHover();
			if (tile < 2)
			{
				tile += 2;
				block -= m_editor->GetControlBlockWidth();
			}
			else
			{
				tile -= 2;
			}
			if (block < 0)
			{
				block = 0;
				tile = tile % 2;
			}
			m_editor->SetTileHover(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
			m_editor->SetTileSelection(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
		}
		else
		{
			m_editor->SetTileHover(0, 0);
			m_editor->SetTileSelection(0, 0);
		}
		break;
	case ID_TILE_DOWN:
		if (m_editor->IsTileHoverValid())
		{
			int block = m_editor->GetBlockHover();
			int tile = m_editor->GetTileHover();
			if (tile > 1)
			{
				tile -= 2;
				block += m_editor->GetControlBlockWidth();
			}
			else
			{
				tile += 2;
			}
			if (block > static_cast<int>(m_editor->GetBlockmapSize()))
			{
				block = m_editor->GetBlockmapSize() - 1;
				tile = tile % 2 + 2;
			}
			m_editor->SetTileHover(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
			m_editor->SetTileSelection(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
		}
		else
		{
			m_editor->SetTileHover(0, 0);
			m_editor->SetTileSelection(0, 0);
		}
		break;
	case ID_TILE_LEFT:
		if (m_editor->IsTileHoverValid())
		{
			int block = m_editor->GetBlockHover();
			int tile = m_editor->GetTileHover();
			if (tile % 2)
			{
				--tile;
			}
			else
			{
				++tile;
				--block;
			}
			if (block < 0)
			{
				block = 0;
				tile = tile % 2;
			}
			m_editor->SetTileHover(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
			m_editor->SetTileSelection(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
		}
		else
		{
			m_editor->SetTileHover(0, 0);
			m_editor->SetTileSelection(0, 0);
		}
		break;
	case ID_TILE_RIGHT:
		if (m_editor->IsTileHoverValid())
		{
			int block = m_editor->GetBlockHover();
			int tile = m_editor->GetTileHover();
			if ((tile % 2) > 0)
			{
				tile -= 1;
				++block;
			}
			else
			{
				tile += 1;
			}
			if (block > static_cast<int>(m_editor->GetBlockmapSize()))
			{
				block = m_editor->GetBlockmapSize() - 1;
				tile = tile % 2 + 2;
			}
			m_editor->SetTileHover(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
			m_editor->SetTileSelection(std::clamp<int16_t>(block, 0, m_editor->GetBlockmapSize() - 1), tile);
		}
		else
		{
			m_editor->SetTileHover(0, 0);
			m_editor->SetTileSelection(0, 0);
		}
		break;
	case ID_SELECT_TILE:
		if (m_editor->IsTileHoverValid())
		{
			m_editor->SetMode(BlocksetEditorCtrl::Mode::PENCIL);
			m_editor->SetDrawTile(m_editor->GetHoveredTile().GetIndex());
			m_tileset->SelectTile(m_editor->GetHoveredTile().GetIndex());
			UpdateUI();
		}
		break;
	case ID_DRAW_TILE:
		if (m_editor->IsTileHoverValid())
		{
			m_editor->SetMode(BlocksetEditorCtrl::Mode::PENCIL);
			m_editor->SetHoveredTile(m_editor->GetDrawTile());
			UpdateUI();
		}
		break;
	case ID_CLEAR_SELECTION:
		m_editor->SetBlockSelection(-1);
		m_editor->SetTileHover(-1, -1);
		m_editor->SetTileSelection(-1, -1);
		UpdateUI();
		break;
	default:
		break;
	}
	FireEvent(EVT_PROPERTIES_UPDATE);
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
			EnableToolbarItem("Blockset", ID_CUT, m_editor->IsBlockSelectionValid());
			EnableToolbarItem("Blockset", ID_COPY, m_editor->IsBlockSelectionValid());
			EnableToolbarItem("Blockset", ID_PASTE, m_editor->IsBlockSelectionValid() && m_blockclipboard.has_value());
			EnableToolbarItem("Blockset", ID_SWAP, m_editor->IsBlockSelectionValid());
			EnableToolbarItem("Blockset", ID_CLEAR, m_editor->IsBlockSelectionValid());
			EnableMenuItem(ID_EDIT_CUT, m_editor->IsBlockSelectionValid());
			EnableMenuItem(ID_EDIT_COPY, m_editor->IsBlockSelectionValid());
			EnableMenuItem(ID_EDIT_PASTE, m_editor->IsBlockSelectionValid() && m_blockclipboard.has_value());
			EnableMenuItem(ID_EDIT_SWAP, m_editor->IsBlockSelectionValid());
			EnableMenuItem(ID_EDIT_CLEAR, m_editor->IsBlockSelectionValid());
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
			EnableToolbarItem("Blockset", ID_CUT, m_editor->IsTileSelectionValid());
			EnableToolbarItem("Blockset", ID_COPY, m_editor->IsTileSelectionValid());
			EnableToolbarItem("Blockset", ID_PASTE, m_editor->IsTileSelectionValid() && m_tileclipboard.has_value());
			EnableToolbarItem("Blockset", ID_SWAP, m_editor->IsTileSelectionValid());
			EnableToolbarItem("Blockset", ID_CLEAR, m_editor->IsTileSelectionValid());
			EnableMenuItem(ID_EDIT_CUT, m_editor->IsTileSelectionValid());
			EnableMenuItem(ID_EDIT_COPY, m_editor->IsTileSelectionValid());
			EnableMenuItem(ID_EDIT_PASTE, m_editor->IsTileSelectionValid() && m_tileclipboard.has_value());
			EnableMenuItem(ID_EDIT_SWAP, m_editor->IsTileSelectionValid());
			EnableMenuItem(ID_EDIT_CLEAR, m_editor->IsTileSelectionValid());
			break;
		case BlocksetEditorCtrl::Mode::PENCIL:
			CheckMenuItem(ID_TOOLS_TILE_DRAW, true);
			CheckToolbarItem("Blockset", ID_PENCIL, true);
			EnableToolbarItem("Blockset", ID_DELETE_BLOCK, false);
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_BEFORE, false);
			EnableToolbarItem("Blockset", ID_INSERT_BLOCK_AFTER, false);
			EnableToolbarItem("Blockset", ID_VFLIP_TILE, m_editor->IsTileHoverValid());
			EnableToolbarItem("Blockset", ID_HFLIP_TILE, m_editor->IsTileHoverValid());
			EnableToolbarItem("Blockset", ID_TOGGLE_TILE_PRIORITY, m_editor->IsTileHoverValid());
			EnableToolbarItem("Blockset", ID_CUT, m_editor->IsTileHoverValid());
			EnableToolbarItem("Blockset", ID_COPY, m_editor->IsTileHoverValid());
			EnableToolbarItem("Blockset", ID_PASTE, m_editor->IsTileHoverValid() && m_tileclipboard.has_value());
			EnableToolbarItem("Blockset", ID_SWAP, m_editor->IsTileHoverValid());
			EnableToolbarItem("Blockset", ID_CLEAR, m_editor->IsTileHoverValid());
			EnableMenuItem(ID_EDIT_CUT, m_editor->IsTileHoverValid());
			EnableMenuItem(ID_EDIT_COPY, m_editor->IsTileHoverValid());
			EnableMenuItem(ID_EDIT_PASTE, m_editor->IsTileHoverValid() && m_tileclipboard.has_value());
			EnableMenuItem(ID_EDIT_SWAP, m_editor->IsTileHoverValid());
			EnableMenuItem(ID_EDIT_CLEAR, m_editor->IsTileHoverValid());                                                   
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
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
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
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void BlocksetEditorFrame::FireUpdateStatusEvent(const std::string& data, int pane)
{
	wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
	evt.SetString(data);
	evt.SetInt(pane);
	evt.SetClientData(GetParent());
	wxPostEvent(GetParent(), evt);
}
