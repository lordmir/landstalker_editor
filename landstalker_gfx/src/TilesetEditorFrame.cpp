#include "TilesetEditorFrame.h"

#include <fstream>
#include <string>
#include <sstream>
#include "Utils.h"

wxBEGIN_EVENT_TABLE(TilesetEditorFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_CHANGE, TilesetEditorFrame::OnPaletteChanged)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, TilesetEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, TilesetEditorFrame::OnTileEditRequested)
EVT_COMMAND(wxID_ANY, EVT_TILE_CHANGE, TilesetEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_TILE_PIXEL_HOVER, TilesetEditorFrame::OnTilePixelHover)
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

template <class T>
static std::string VecToCommaList(const std::vector<T> list)
{
	std::ostringstream ss;
	std::copy(list.begin(), list.end(), std::ostream_iterator<int>(ss, ","));
	return ss.str();
}

template <class T>
static std::vector<T> CommaListToVec(const std::string& input)
{
	std::vector<T> ret;
	std::istringstream ss(input);
	try
	{
		while (ss.good())
		{
			std::string substr;
			std::getline(ss, substr, ',');
			ret.push_back(std::stoi(substr));
		}
	}
	catch (...)
	{
		ret.clear();
	}
	return ret;
}

TilesetEditorFrame::TilesetEditorFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
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

void TilesetEditorFrame::InitStatusBar(wxStatusBar& status) const
{
	status.SetFieldsCount(3);
	status.SetStatusText("", 0);
	status.SetStatusText("", 1);
	status.SetStatusText("", 2);
}

void TilesetEditorFrame::OnZoomChange(wxCommandEvent& evt)
{
	evt.Skip();
}

void TilesetEditorFrame::OnTileSelectionChanged(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
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
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void TilesetEditorFrame::OnTilePixelHover(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
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
	FireEvent(EVT_STATUSBAR_UPDATE);
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

void TilesetEditorFrame::UpdateStatusBar(wxStatusBar& status) const
{
	std::ostringstream ss;
	ss << "Selected Tile " << m_tile.GetIndex();
	if (m_tileEditor->IsHoverValid())
	{
		const auto selection = m_tileEditor->GetHoveredPixel();
		ss << ": (" << selection.x << ", " << selection.y << ")";
	}
	status.SetStatusText(ss.str(), 0);
	ss.str(std::string());
	if (m_tilesetEditor->IsHoverValid())
	{
		const auto selection = m_tilesetEditor->GetHoveredTile();
		ss << "Tile at mouse: " << selection.GetIndex();
	}
	status.SetStatusText(ss.str(), 1);
}

void TilesetEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	for (const auto& b : Tileset::BLOCKTYPE_STRINGS)
	{
		m_blocktype_list.Add(b);
	}
	props.GetGrid()->Clear();
	props.Append(new wxPropertyCategory("Main", "M"));
	props.Append(new wxStringProperty("Start Address", "SA", "0x000000"))->Enable(false);
	props.Append(new wxStringProperty("End Address", "EA", "0x000000"))->Enable(false);
	props.Append(new wxFileProperty("Filename", "FN", "untitled.bin"))->Enable(false);
	props.Append(new wxStringProperty("Compressed Size", "CS", "0 bytes"))->Enable(false);
	props.Append(new wxStringProperty("Uncompressed Size", "US", "0 bytes"))->Enable(false);
	props.Append(new wxPropertyCategory("Tileset", "T"));
	props.Append(new wxEnumProperty("Palette", "P", m_palette_list));
	props.Append(new wxStringProperty("Colour Indicies", "I", ""));
	props.Append(new wxBoolProperty("LZ77 Compressed", "C", false))->Enable(false);
	props.Append(new wxIntProperty("Tile Count", "#", 0))->Enable(false);
	props.Append(new wxIntProperty("Tile Width", "W", 0))->Enable(false);
	props.Append(new wxIntProperty("Tile Height", "H", 0))->Enable(false);
	props.Append(new wxIntProperty("Tile Bitdepth", "D", 0))->Enable(false);
	props.Append(new wxEnumProperty("Tile Block Layout", "B", m_blocktype_list))->Enable(false);
	UpdateProperties(props);
}

void TilesetEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	if (m_tileset)
	{
		props.GetGrid()->SetPropertyValue("US", wxString::Format("%lu bytes", m_tileset->GetTilesetUncompressedSizeBytes()));
		props.GetGrid()->SetPropertyValue("#", wxString::Format("%lu", m_tileset->GetTileCount()));
		props.GetGrid()->SetPropertyValue("W", wxString::Format("%lu", m_tileset->GetTileWidth()));
		props.GetGrid()->SetPropertyValue("H", wxString::Format("%lu", m_tileset->GetTileHeight()));
		props.GetGrid()->SetPropertyValue("D", wxString::Format("%lu", m_tileset->GetTileBitDepth()));
		props.GetGrid()->SetPropertyValue("B", wxString::Format("%lu", m_tileset->GetTileBlockType()));
		props.GetGrid()->SetPropertyValue("C", wxString::Format("%lu", m_tileset->GetCompressed()));
		props.GetGrid()->SetPropertyValue("I", wxString(VecToCommaList(m_tileset->GetColourIndicies())));
	}
	props.GetGrid()->SetPropertyValue("P", wxString(m_selected_palette));
}

void TilesetEditorFrame::OnPropertyChange(wxPropertyGridEvent& evt)
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
	else if (name == "I")
	{
		// Indicies change
		m_tileset->SetColourIndicies(CommaListToVec<uint8_t>(property->GetValueAsString().ToStdString()));
		m_paletteEditor->Refresh();
		m_tilesetEditor->RedrawTiles();
		m_tileEditor->Refresh();
	}
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void TilesetEditorFrame::SetPalettes(std::shared_ptr<std::map<std::string, Palette>> palettes)
{
	m_palettes = palettes;
	m_tilesetEditor->SetPalettes(palettes);
	m_paletteEditor->SetPalettes(palettes);
	m_tileEditor->SetPalettes(palettes);
	m_palette_list.Clear();
	for (const auto& pal : *m_palettes)
	{
		m_palette_list.Add(pal.first);
	}
}

void TilesetEditorFrame::SetActivePalette(const std::string& name)
{
	m_selected_palette = name;
	m_tilesetEditor->SetActivePalette(name);
	m_paletteEditor->SelectPalette(name);
	m_tileEditor->SetActivePalette(name);
	FireEvent(EVT_PROPERTIES_UPDATE);
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
	FireEvent(EVT_PROPERTIES_UPDATE);
	return retval;
}
