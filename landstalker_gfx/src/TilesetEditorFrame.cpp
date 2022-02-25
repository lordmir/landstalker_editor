#include "TilesetEditorFrame.h"

#include <fstream>
#include <string>
#include "Utils.h"

wxBEGIN_EVENT_TABLE(TilesetEditorFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_CHANGE, TilesetEditorFrame::OnPaletteChanged)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, TilesetEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, TilesetEditorFrame::OnTileEditRequested)
EVT_COMMAND(wxID_ANY, EVT_TILE_CHANGE, TilesetEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_TILESET_HOVER, TilesetEditorFrame::OnTileSelectionChanged)
EVT_COMMAND(wxID_ANY, EVT_TILESET_CHANGE, TilesetEditorFrame::OnTilesetChange)
wxEND_EVENT_TABLE()

enum MENU_IDS
{
	ID_TOGGLE_GRIDLINES = 30000,
	ID_TOGGLE_TILE_NOS,
	ID_TOGGLE_ALPHA,
	ID_ADD_TILE_AFTER_SEL,
	ID_ADD_TILE_BEFORE_SEL,
	ID_EXTEND_TILESET,
	ID_DELETE_TILE,
	ID_SWAP_TILES,
	ID_CUT_TILE,
	ID_COPY_TILE,
	ID_PASTE_TILE,
	ID_EDIT_TILE
};

TilesetEditorFrame::TilesetEditorFrame(wxWindow* parent)
	: wxWindow(parent, 22001),
	  m_title("")
{
	m_mgr.SetManagedWindow(this);
	m_tilesetEditor = new TilesetEditor(this);

	m_paletteEditor = new PaletteEditor(this);

	m_tileEditor = new TileEditor(this);
	m_tileEditor->SetPixelSize(64);

	// add the panes to the manager
	m_mgr.AddPane(m_paletteEditor, wxBOTTOM, wxT("Palettes"));
	m_mgr.GetPane(m_paletteEditor).MinSize(500, 150).FloatingSize(300, 50);
	m_mgr.AddPane(m_tileEditor, wxLEFT, wxT("Editor"));
	m_mgr.GetPane(m_tileEditor).MinSize(640, 640).FloatingSize(520, 520);
	m_mgr.AddPane(m_tilesetEditor, wxCENTER);

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
}

TilesetEditorFrame::~TilesetEditorFrame()
{
}

void TilesetEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	evt.Skip();
}

void TilesetEditorFrame::OnTileSelectionChanged(wxCommandEvent& evt)
{
	evt.Skip();
}

void TilesetEditorFrame::OnTileChanged(wxCommandEvent& evt)
{
	auto tile = std::stoi(evt.GetString().ToStdString());
	m_tilesetEditor->RedrawTiles(tile);
	evt.Skip();
}

void TilesetEditorFrame::OnTilesetChange(wxCommandEvent& evt)
{
	m_tilesetEditor->RedrawTiles();
	m_tileEditor->Redraw();
	evt.Skip();
}

void TilesetEditorFrame::OnTileEditRequested(wxCommandEvent& evt)
{
	int tileId = std::stoi(evt.GetString().ToStdString());
	if (tileId >= 0)
	{
		m_tile = tileId;
		m_tileEditor->SetTile(m_tile);
	}
	evt.Skip();
}

void TilesetEditorFrame::OnPaletteChanged(wxCommandEvent& evt)
{
	m_paletteEditor->Refresh();
	m_tilesetEditor->RedrawTiles();
	m_tileEditor->Refresh();
	evt.Skip();
}

void TilesetEditorFrame::OnPaletteColourSelect(wxCommandEvent& evt)
{
	m_tileEditor->SetPrimaryColour(m_paletteEditor->GetPrimaryColour());
	m_tileEditor->SetSecondaryColour(m_paletteEditor->GetSecondaryColour());
	evt.Skip();
}

std::string TilesetEditorFrame::GetStatusBarText() const
{
	return "Passion";
}

void TilesetEditorFrame::SetPalettes(std::shared_ptr<std::map<std::string, Palette>> palettes)
{
	m_palettes = palettes;
	m_tilesetEditor->SetPalettes(palettes);
	m_paletteEditor->SetPalettes(palettes);
	m_tileEditor->SetPalettes(palettes);
}

void TilesetEditorFrame::SetActivePalette(const std::string& name)
{
	m_selected_palette = name;
	m_tilesetEditor->SetActivePalette(name);
	m_paletteEditor->SelectPalette(name);
	m_tileEditor->SetActivePalette(name);
}

bool TilesetEditorFrame::Open(std::vector<uint8_t>& pixels, bool uses_compression, int tile_width, int tile_height, int tile_bitdepth)
{
	bool retval = false;
	retval = m_tilesetEditor->Open(pixels, uses_compression, tile_width, tile_height, tile_bitdepth);
	if (retval)
	{
		m_tileset = m_tilesetEditor->GetTileset();
		m_tile = 0;
		m_tileEditor->SetTileset(m_tileset);
		m_tileEditor->SetTile(m_tile);
	}
	return retval;
}
