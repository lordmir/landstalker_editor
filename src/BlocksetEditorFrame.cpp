#include "BlocksetEditorFrame.h"

wxBEGIN_EVENT_TABLE(BlocksetEditorFrame, wxWindow)
EVT_SLIDER(wxID_ANY, BlocksetEditorFrame::OnZoomChange)
EVT_TOOL(wxID_ANY, BlocksetEditorFrame::OnButtonClicked)
wxEND_EVENT_TABLE()

enum MENU_IDS
{
	ID_FILE_EXPORT_CBS = 20000,
	ID_FILE_EXPORT_CSV,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_CBS,
	ID_FILE_IMPORT_CSV,
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
	ID_PENCIL
};

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
	UpdateStatus();
	FireEvent(EVT_PROPERTIES_UPDATE);
	evt.Skip();
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
	auto sz = BlocksetCmp::Decode(bytes.data(), bytes.size(), *m_blocks->GetData());
	m_editor->RedrawTiles();
	UpdateStatus();
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
	UpdateStatus();
}

void BlocksetEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	UpdateStatus();
	evt.Skip();
}

void BlocksetEditorFrame::OnButtonClicked(wxCommandEvent& evt)
{
	switch (evt.GetId())
	{
	case ID_TOGGLE_GRIDLINES:
		m_editor->SetBordersEnabled(!m_editor->GetBordersEnabled());
		break;
	case ID_TOGGLE_TILE_NUMBERS:
		m_editor->SetTileNumbersEnabled(!m_editor->GetTileNumbersEnabled());
		break;
	case ID_TOGGLE_ALPHA:
		m_editor->SetAlphaEnabled(!m_editor->GetAlphaEnabled());
		break;
	case ID_INSERT_BLOCK_BEFORE:
		if (m_editor->IsBlockSelectionValid())
		{
		}
		break;
	case ID_INSERT_BLOCK_AFTER:
		if (m_editor->IsBlockSelectionValid())
		{
		}
		break;
	case ID_DELETE_BLOCK:
		if (m_editor->IsBlockSelectionValid())
		{
		}
		break;
	case ID_HFLIP_TILE:
		if (m_editor->IsTileSelectionValid())
		{
			auto tile = m_editor->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_HFLIP);
			m_editor->SetSelectedTile(tile);
		}
		break;
	case ID_VFLIP_TILE:
		if (m_editor->IsTileSelectionValid())
		{
			auto tile = m_editor->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_VFLIP);
			m_editor->SetSelectedTile(tile);
		}
		break;
	case ID_TOGGLE_TILE_PRIORITY:
		if (m_editor->IsTileSelectionValid())
		{
			auto tile = m_editor->GetSelectedTile();
			tile.Attributes().toggleAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
			m_editor->SetSelectedTile(tile);
		}
		break;
	case ID_BLOCK_SELECT:
		m_toolbar->ToggleTool(ID_BLOCK_SELECT, true);
		m_editor->SetMode(BlocksetEditorCtrl::Mode::BLOCK_SELECT);
		break;
	case ID_TILE_SELECT:
		m_toolbar->ToggleTool(ID_TILE_SELECT, true);
		m_editor->SetMode(BlocksetEditorCtrl::Mode::TILE_SELECT);
		break;
	case ID_PENCIL:
		m_toolbar->ToggleTool(ID_PENCIL, true);
		m_editor->SetMode(BlocksetEditorCtrl::Mode::PENCIL);
		break;
	default:
		break;
	}
	UpdateStatus();
	evt.Skip();
}

void BlocksetEditorFrame::UpdateStatus()
{
	if (m_zoomslider)
	{
		std::ostringstream ss;
		m_statusbar->SetStatusText(wxString::Format("Zoom: %d%s", m_zoomslider->GetValue(), ss.str()), 1);
		m_editor->SetPixelSize(m_zoomslider->GetValue());
	}
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
