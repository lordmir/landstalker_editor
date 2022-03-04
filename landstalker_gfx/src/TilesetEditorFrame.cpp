#include "TilesetEditorFrame.h"

#include <fstream>
#include <string>
#include <sstream>
#include "Utils.h"
#include "wx/artprov.h"

wxBEGIN_EVENT_TABLE(TilesetEditorFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_CHANGE, TilesetEditorFrame::OnPaletteChanged)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, TilesetEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, TilesetEditorFrame::OnTileEditRequested)
EVT_COMMAND(wxID_ANY, EVT_TILE_CHANGE, TilesetEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_TILE_PIXEL_HOVER, TilesetEditorFrame::OnTilePixelHover)
EVT_COMMAND(wxID_ANY, EVT_TILESET_HOVER, TilesetEditorFrame::OnTileSelectionChanged)
EVT_COMMAND(wxID_ANY, EVT_TILESET_CHANGE, TilesetEditorFrame::OnTilesetChange)
wxEND_EVENT_TABLE()

enum TOOL_IDS
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
	ID_EDIT_TILE,
	ID_DRAW_TOGGLE_GRIDLINES,
	ID_PENCIL
};

enum MENU_IDS
{
	ID_FILE_NEW = 20000,
	ID_FILE_EXPORT_BIN,
	ID_FILE_EXPORT_ALL,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_BIN,
	ID_FILE_IMPORT_PNG,
	ID_VIEW,
	ID_VIEW_TOGGLE_GRIDLINES,
	ID_VIEW_TOGGLE_TILE_NOS,
	ID_VIEW_TOGGLE_ALPHA,
	ID_TOOLS,
	ID_TOOLS_PALETTE,
	ID_TOOLS_EDITOR,
	ID_TOOLS_TILESET_TOOLBAR,
	ID_TOOLS_DRAW_TOOLBAR
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

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
	m_mgr.AddPane(m_tileEditor, wxAuiPaneInfo().Left().Layer(1).MinSize(100, 100).BestSize(450, 450).FloatingSize(450,450).Caption("Editor"));
	m_mgr.AddPane(m_paletteEditor, wxAuiPaneInfo().Bottom().Layer(1).MinSize(180, 40).BestSize(700, 100).FloatingSize(700,100).Caption("Palette"));
	m_mgr.AddPane(m_tilesetEditor, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
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

void TilesetEditorFrame::UpdateUI() const
{
	CheckMenuItem(ID_TOOLS_PALETTE, IsPaneVisible(m_paletteEditor));
	CheckMenuItem(ID_TOOLS_EDITOR, IsPaneVisible(m_tileEditor));
	CheckMenuItem(ID_TOOLS_TILESET_TOOLBAR, IsToolbarVisible("Tileset"));
	CheckMenuItem(ID_TOOLS_DRAW_TOOLBAR, IsToolbarVisible("Draw"));
	if (m_tilesetEditor != nullptr)
	{
		CheckMenuItem(ID_VIEW_TOGGLE_ALPHA, !m_tilesetEditor->GetAlphaEnabled());
		CheckToolbarItem("Tileset", ID_TOGGLE_ALPHA, !m_tilesetEditor->GetAlphaEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_GRIDLINES, m_tilesetEditor->GetBordersEnabled());
		CheckToolbarItem("Tileset", ID_TOGGLE_GRIDLINES, m_tilesetEditor->GetBordersEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_TILE_NOS, m_tilesetEditor->GetTileNumbersEnabled());
		CheckToolbarItem("Tileset", ID_TOGGLE_TILE_NOS, m_tilesetEditor->GetTileNumbersEnabled());
	}
	if (m_tileEditor != nullptr)
	{
		CheckToolbarItem("Draw", ID_DRAW_TOGGLE_GRIDLINES, m_tileEditor->GetBordersEnabled());
	}
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
	if(ArePropsInitialised() == false)
	{
		EditorFrame::InitProperties(props);
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
}

void TilesetEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	if (ArePropsInitialised() == false)
	{
		InitProperties(props);
	}
	else
	{
		if (m_tileset)
		{
			props.GetGrid()->SetPropertyValue("US", wxString::Format("%lu bytes", m_tileset->GetTilesetUncompressedSizeBytes()));
			props.GetGrid()->SetPropertyValue("#", wxString::Format("%lu", m_tileset->GetTileCount()));
			props.GetGrid()->SetPropertyValue("W", wxString::Format("%lu", m_tileset->GetTileWidth()));
			props.GetGrid()->SetPropertyValue("H", wxString::Format("%lu", m_tileset->GetTileHeight()));
			props.GetGrid()->SetPropertyValue("D", wxString::Format("%lu", m_tileset->GetTileBitDepth()));
			props.GetGrid()->SetPropertyValue("B", wxString(Tileset::BLOCKTYPE_STRINGS[m_tileset->GetTileBlockType()]));
			props.GetGrid()->SetPropertyValue("C", m_tileset->GetCompressed());
			props.GetGrid()->SetPropertyValue("I", wxString(VecToCommaList(m_tileset->GetColourIndicies())));
		}
		props.GetGrid()->SetPropertyValue("P", wxString(m_selected_palette));
	}
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

void TilesetEditorFrame::InitMenu(wxMenuBar& menu, wxAuiManager& mgr, ImageList& ilist) const
{
	auto* parent = mgr.GetManagedWindow();

	ClearMenu(menu, mgr);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 2, ID_FILE_NEW, "New Tileset");
	AddMenuItem(fileMenu, 3, ID_FILE_EXPORT_BIN, "Export Tileset...");
	AddMenuItem(fileMenu, 4, ID_FILE_EXPORT_ALL, "Export All Tilesets...");
	AddMenuItem(fileMenu, 5, ID_FILE_EXPORT_PNG, "Export Tileset as PNG...");
	AddMenuItem(fileMenu, 6, ID_FILE_IMPORT_BIN, "Import Tileset...");
	auto& viewMenu = AddMenu(menu, 1, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_TILE_NOS, "Tile Numbers", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	auto& toolsMenu = AddMenu(menu, 2, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_PALETTE, "Palette", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_EDITOR, "Tile Editor", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, ID_TOOLS_TILESET_TOOLBAR, "Tileset Toolbar", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 3, ID_TOOLS_DRAW_TOOLBAR, "Draw Toolbar", wxITEM_CHECK);

	wxAuiToolBar* tileset_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	tileset_tb->SetToolBitmapSize(wxSize(16, 16));
	tileset_tb->AddTool(ID_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	tileset_tb->AddTool(ID_TOGGLE_TILE_NOS, "Toggle Tile Numbers", ilist.GetImage("tile_nums"), "Toggle Tile Numbers", wxITEM_CHECK);
	tileset_tb->AddTool(ID_TOGGLE_ALPHA, "Toggle Alpha", ilist.GetImage("alpha"), "Toggle Alpha", wxITEM_CHECK);
	tileset_tb->AddSeparator();
	tileset_tb->AddTool(ID_ADD_TILE_BEFORE_SEL, "Add Tile Before Selected", ilist.GetImage("insert_before"), "Add Tile Before Selected");
	tileset_tb->AddTool(ID_ADD_TILE_AFTER_SEL, "Add Tile After Selected", ilist.GetImage("insert_after"), "Add Tile After Selected");
	tileset_tb->AddTool(ID_EXTEND_TILESET, "Extend Tileset", ilist.GetImage("append_tile"), "Extend Tileset");
	tileset_tb->AddTool(ID_DELETE_TILE, "Delete Tile", ilist.GetImage("delete_tile"), "Delete Tile");
	tileset_tb->AddSeparator();
	tileset_tb->AddTool(ID_SWAP_TILES, "Swap Tiles", ilist.GetImage("swap"), "Swap Tiles");
	tileset_tb->AddTool(ID_CUT_TILE, "Cut Tile", ilist.GetImage("cut"), "Cut Tile");
	tileset_tb->AddTool(ID_COPY_TILE, "Copy Tile", ilist.GetImage("copy"), "Copy Tile");
	tileset_tb->AddTool(ID_PASTE_TILE, "Paste Tile", ilist.GetImage("paste"), "Paste Tile");
	AddToolbar(mgr, *tileset_tb, "Tileset", "Tileset Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));

	wxAuiToolBar* draw_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	draw_tb->SetToolBitmapSize(wxSize(16, 16));
	draw_tb->AddTool(ID_DRAW_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	draw_tb->AddSeparator();
	draw_tb->AddTool(ID_PENCIL, "Pencil", ilist.GetImage("pencil"), "Pencil", wxITEM_RADIO);
	AddToolbar(mgr, *draw_tb, "Draw", "Drawing Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1));

	UpdateUI();

	mgr.Update();
}

void TilesetEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	const auto id = evt.GetId();
	if ((id >= 20000) && (id < 31000))
	{
		switch (id)
		{
		case ID_VIEW_TOGGLE_GRIDLINES:
		case ID_TOGGLE_GRIDLINES:
			if (m_tilesetEditor != nullptr)
			{
				m_tilesetEditor->SetBordersEnabled(!m_tilesetEditor->GetBordersEnabled());
			}
			break;
		case ID_VIEW_TOGGLE_TILE_NOS:
		case ID_TOGGLE_TILE_NOS:
			if (m_tilesetEditor != nullptr)
			{
				m_tilesetEditor->SetTileNumbersEnabled(!m_tilesetEditor->GetTileNumbersEnabled());
			}
			break;
		case ID_VIEW_TOGGLE_ALPHA:
		case ID_TOGGLE_ALPHA:
			if (m_tilesetEditor != nullptr)
			{
				m_tilesetEditor->SetAlphaEnabled(!m_tilesetEditor->GetAlphaEnabled());
			}
			break;
		case ID_DRAW_TOGGLE_GRIDLINES:
			if (m_tileEditor != nullptr)
			{
				m_tileEditor->SetBordersEnabled(!m_tileEditor->GetBordersEnabled());
			}
			break;
		case ID_TOOLS_PALETTE:
			SetPaneVisibility(m_paletteEditor, !IsPaneVisible(m_paletteEditor));
			break;
		case ID_TOOLS_EDITOR:
			SetPaneVisibility(m_tileEditor, !IsPaneVisible(m_tileEditor));
			break;
		case ID_TOOLS_TILESET_TOOLBAR:
			SetToolbarVisibility("Tileset", !IsToolbarVisible("Tileset"));
			break;
		case ID_TOOLS_DRAW_TOOLBAR:
			SetToolbarVisibility("Draw", !IsToolbarVisible("Draw"));
			break;
		default:
			wxMessageBox(wxString::Format("Unrecognised Event %d", evt.GetId()));
		}
		UpdateUI();
	}
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
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	return retval;
}
