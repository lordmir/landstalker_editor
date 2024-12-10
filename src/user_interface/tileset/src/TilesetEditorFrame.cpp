#include <user_interface/tileset/include/TilesetEditorFrame.h>

#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <landstalker/misc/include/Utils.h>
#include <wx/artprov.h>

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
	ID_ZOOM_SLIDER,
	ID_DRAW_TOGGLE_GRIDLINES,
	ID_PENCIL,
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

wxBEGIN_EVENT_TABLE(TilesetEditorFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_CHANGE, TilesetEditorFrame::OnPaletteChanged)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, TilesetEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_HOVER, TilesetEditorFrame::OnPaletteColourHover)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, TilesetEditorFrame::OnTileEditRequested)
EVT_COMMAND(wxID_ANY, EVT_TILE_CHANGE, TilesetEditorFrame::OnTileChanged)
EVT_COMMAND(wxID_ANY, EVT_TILE_PIXEL_HOVER, TilesetEditorFrame::OnTilePixelHover)
EVT_COMMAND(wxID_ANY, EVT_TILESET_HOVER, TilesetEditorFrame::OnTileSelectionChanged)
EVT_COMMAND(wxID_ANY, EVT_TILESET_CHANGE, TilesetEditorFrame::OnTilesetChange)
EVT_SLIDER(ID_ZOOM_SLIDER, TilesetEditorFrame::OnZoom)
wxEND_EVENT_TABLE()

template <class T>
static std::string VecToCommaList(const std::vector<T> list)
{
	std::ostringstream ss;
	std::copy(list.begin(), list.end(), std::ostream_iterator<int>(ss, ","));
	auto val = ss.str();
	return val.substr(0, val.length() - 1);
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

TilesetEditorFrame::TilesetEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_tile(0),
	  m_inputBuffer(0),
	  m_animated(false),
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

void TilesetEditorFrame::OnZoom(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	m_tilesetEditor->SetPixelSize(m_zoomslider->GetValue());
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
	m_tileEditor->SetTile(m_tilesetEditor->GetSelectedTile());
	m_tileEditor->Redraw();
	m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());
	FireEvent(EVT_PROPERTIES_UPDATE);
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void TilesetEditorFrame::OnTilePixelHover(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void TilesetEditorFrame::ToggleAlpha()
{
	if (m_tilesetEditor != nullptr)
	{
		m_tilesetEditor->SetAlphaEnabled(!m_tilesetEditor->GetAlphaEnabled());
	}
}

void TilesetEditorFrame::ToggleTileNums()
{
	if (m_tilesetEditor != nullptr)
	{
		m_tilesetEditor->SetTileNumbersEnabled(!m_tilesetEditor->GetTileNumbersEnabled());
	}
}

void TilesetEditorFrame::ToggleGrid()
{
	if (m_tilesetEditor != nullptr)
	{
		m_tilesetEditor->SetBordersEnabled(!m_tilesetEditor->GetBordersEnabled());
	}
}

void TilesetEditorFrame::InsertTileBefore()
{
	try
	{
		if (m_tilesetEditor->IsSelectionValid())
		{
			m_tilesetEditor->InsertTileBefore(m_tilesetEditor->GetSelectedTile().GetIndex());
		}
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	catch (const std::exception & e)
	{
		wxMessageBox(_("Failed to add tile: ") + e.what());
	}
}

void TilesetEditorFrame::InsertTileAfter()
{
	try
	{
		if (m_tilesetEditor->IsSelectionValid())
		{
			m_tilesetEditor->InsertTileAfter(m_tilesetEditor->GetSelectedTile().GetIndex());
		}
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	catch (const std::exception & e)
	{
		wxMessageBox(_("Failed to add tile: ") + e.what());
	}
}

void TilesetEditorFrame::ExtendTileset()
{
	auto dlg = wxTextEntryDialog(this, "Enter number of tiles to extend by", "Extend Tileset", "1");

	if (dlg.ShowModal() != wxID_CANCEL)
	{
		try
		{
			int count = std::stoi(dlg.GetValue().ToStdString());
			if (count >= 1)
			{
				m_tilesetEditor->InsertTilesAtEnd(count);
			}
			else
			{
				throw std::out_of_range("bad number");
			}
			FireEvent(EVT_PROPERTIES_UPDATE);
		}
		catch (const std::exception & e)
		{
			wxMessageBox(_("Failed to extend tileset: ") + e.what());
		}
	}
}

void TilesetEditorFrame::DeleteTile()
{
	try
	{
		if (m_tilesetEditor->IsSelectionValid())
		{
			m_tilesetEditor->DeleteTileAt(m_tilesetEditor->GetSelectedTile().GetIndex());
		}
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	catch (const std::exception & e)
	{
		wxMessageBox(_("Failed to delete tile: ") + e.what());
	}
}

void TilesetEditorFrame::SwapTiles()
{
	try
	{
		if (m_tilesetEditor->IsSelectionValid())
		{
			m_tilesetEditor->SwapTile(m_tilesetEditor->GetSelectedTile().GetIndex());
		}
	}
	catch (const std::exception & e)
	{
		wxMessageBox(_("Failed to swap tile: ") + e.what());
	}
}

void TilesetEditorFrame::CutTile()
{
	try
	{
		if (m_tilesetEditor->IsSelectionValid())
		{
			m_tilesetEditor->CutTile(m_tilesetEditor->GetSelectedTile().GetIndex());
		}
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	catch (const std::exception & e)
	{
		wxMessageBox(_("Failed to cut tile: ") + e.what());
	}
}

void TilesetEditorFrame::CopyTile()
{
	try
	{
		if (m_tilesetEditor->IsSelectionValid())
		{
			m_tilesetEditor->CopyTile(m_tilesetEditor->GetSelectedTile().GetIndex());
		}
	}
	catch (const std::exception & e)
	{
		wxMessageBox(_("Failed to copy tile: ") + e.what());
	}
}

void TilesetEditorFrame::PasteTile()
{
	try
	{
		if (m_tilesetEditor->IsSelectionValid())
		{
			m_tilesetEditor->PasteTile(m_tilesetEditor->GetSelectedTile().GetIndex());
		}
	}
	catch (const std::exception & e)
	{
		wxMessageBox(_("Failed to paste tile: ") + e.what());
	}
}

void TilesetEditorFrame::ToggleDrawGrid()
{
	if (m_tileEditor != nullptr)
	{
		m_tileEditor->SetBordersEnabled(!m_tileEditor->GetBordersEnabled());
	}
}

void TilesetEditorFrame::ClearTile()
{
	m_tileEditor->Clear();
}

void TilesetEditorFrame::SelectDrawPencil()
{
}

void TilesetEditorFrame::Save()
{
}

void TilesetEditorFrame::SaveAs()
{
}

void TilesetEditorFrame::New()
{
	m_tileset = std::make_shared<Tileset>();
	m_tileset->Clear();
	m_tileset->InsertTilesBefore(0, 1);
	m_tilesetEditor->RedrawTiles();
	m_tilesetEditor->SelectTile(0);
	FireEvent(EVT_PROPERTIES_UPDATE);
}

void TilesetEditorFrame::ImportFromBin()
{
	wxFileDialog fd(this, _("Import Tileset From Binary"), "", "", "Uncompressed Tileset (*.bin)|*.bin|Compressed Tileset (*.lz77)|*.lz77|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		bool use_compression = path.substr(path.find_last_of(".") + 1) == "lz77";
		auto bytes = ReadBytes(path);
		m_tileset->SetBits(bytes, use_compression);
		m_tilesetEditor->ForceRedraw();
		m_tilesetEditor->SelectTile(0);
		m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());

		FireEvent(EVT_PROPERTIES_UPDATE);
	}
}

void TilesetEditorFrame::ImportFromPng()
{
}

void TilesetEditorFrame::ImportFromRom()
{
}

void TilesetEditorFrame::ExportAsBin()
{
	const wxString defaultFile = m_tileset->GetCompressed() ? "tileset.lz77" : "tileset.bin";
	wxFileDialog fd(this, _("Export Tileset As Binary"), "", defaultFile, "Uncompressed Tileset (*.bin)|*.bin|Compressed Tileset (*.lz77)|*.lz77|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	fd.SetFilterIndex(m_tileset->GetCompressed() ? 1 : 0);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		bool use_compression = path.substr(path.find_last_of(".") + 1) == "lz77";
		auto bytes = m_tileset->GetBits(use_compression);
		WriteBytes(bytes, path);
	}
}

void TilesetEditorFrame::ExportAsPng()
{
	wxFileDialog fd(this, _("Export Tileset As PNG"), "", "tileset.png", "PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();

		const std::size_t max_width = 16U;
		const int cols = std::min<std::size_t>(m_tileset->GetTileCount(), max_width);
		const int rows = std::max<std::size_t>(1UL, (m_tileset->GetTileCount() + max_width - 1) / max_width);
		ImageBuffer buf(cols * m_tileset->GetTileWidth(), rows * m_tileset->GetTileHeight());
		for (std::size_t i = 0; i < m_tileset->GetTileCount(); ++i)
		{
			buf.InsertTile((i % cols) * m_tileset->GetTileWidth(), (i / cols) * m_tileset->GetTileHeight(), 0, i, *m_tileset);
		}
		buf.WritePNG(fd.GetPath().ToStdString(), {m_selected_palette->GetData()});
	}
}

void TilesetEditorFrame::ExportAll()
{
}

void TilesetEditorFrame::InjectIntoRom()
{
	auto bytes = m_tileset->GetBits(m_tileset->GetCompressed());
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

void TilesetEditorFrame::OnPaletteColourHover(wxCommandEvent& evt)
{
	FireEvent(EVT_STATUSBAR_UPDATE);
	evt.Skip();
}

void TilesetEditorFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& /*evt*/) const
{
	std::ostringstream ss;
	ss << "Selected Tile " << m_tile.GetIndex();
	int colour = m_paletteEditor->GetHoveredColour();
	if (m_tileEditor->IsHoverValid())
	{
		const auto selection = m_tileEditor->GetHoveredPixel();
		int idx = m_tileEditor->GetColourAtPixel(selection);
		colour = m_tileEditor->GetColour(idx);
		ss << ": (" << selection.x << ", " << selection.y << "): " << idx;
	}
	status.SetStatusText(ss.str(), 0);
	ss.str(std::string());
	if (m_tilesetEditor->IsHoverValid())
	{
		const auto selection = m_tilesetEditor->GetHoveredTile();
		ss << "Tile at mouse: " << selection.GetIndex();
	}
	else if (colour != -1)
	{
		const auto& pal = m_selected_palette->GetData();
		const auto& name = pal->getOwner(colour);
		ss << StrPrintf("Colour at mouse: Index %d - Genesis 0x%04X, RGB #%06X %s", colour,
			pal->getGenesisColour(colour), pal->getRGB(colour), pal->getA(colour) == 0 ? " [Transparent]" : "");
		if (!name.empty())
		{
			ss << ", Palette: \"" << name << "\"";
		}
	}
	status.SetStatusText(ss.str(), 1);
	ss.str(std::string());
	if (m_zoomslider != nullptr)
	{
		ss << "Zoom: " << m_zoomslider->GetValue();
		status.SetStatusText(ss.str(), 2);
	}
}

void TilesetEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if(ArePropsInitialised() == false)
	{
		for (const auto& b : Tileset::BLOCKTYPE_STRINGS)
		{
			m_blocktype_list.Add(b.second);
		}
		props.GetGrid()->Clear();
		props.Append(new wxPropertyCategory("Main", "M"));
		props.Append(new wxStringProperty("Name", "N", ""))->Enable(false);
		props.Append(new wxStringProperty("Start Address", "SA", "0x000000"))->Enable(false);
		props.Append(new wxStringProperty("End Address", "EA", "0x000000"))->Enable(false);
		props.Append(new wxFileProperty("Filename", "FN", "untitled.bin"))->Enable(false);
		props.Append(new wxStringProperty("Original Size", "OS", "0 bytes"))->Enable(false);
		props.Append(new wxStringProperty("Uncompressed Size", "US", "0 bytes"))->Enable(false);
		props.Append(new wxPropertyCategory("Animation", "A"));
		props.Append(new wxIntProperty("Base Tileset", "ABT", 0));
		props.Append(new wxIntProperty("Start From Tile", "AST", 0));
		props.Append(new wxIntProperty("Number of tiles", "A#T", 0));
		props.Append(new wxIntProperty("Number of frames", "A#F", 0));
		props.Append(new wxIntProperty("Animation speed (FPS)", "AS", 0));
		props.Append(new wxPropertyCategory("Tileset", "T"));
		props.Append(new wxEnumProperty("Palette", "P", m_palette_list));
		props.Append(new wxStringProperty("Colour Indicies", "I", ""));
		props.Append(new wxBoolProperty("LZ77 Compressed", "C", false))->Enable(false);
		props.Append(new wxIntProperty("Tile Count", "#", 0))->Enable(false);
		props.Append(new wxIntProperty("Tile Width", "W", 0))->Enable(false);
		props.Append(new wxIntProperty("Tile Height", "H", 0))->Enable(false);
		props.Append(new wxIntProperty("Tile Bitdepth", "D", 0))->Enable(false);
		props.Append(new wxEnumProperty("Tile Block Layout", "B", m_blocktype_list))->Enable(false);
		RefreshProperties(props);
	}
	EditorFrame::InitProperties(props);
}

template <class T>
wxPGChoices UpdatePalList(std::shared_ptr<T> entry, wxPropertyGridManager& /*props*/)
{
	wxPGChoices list;
	list.Clear();
	auto rec = entry->GetRecommendedPalettes();
	for (const auto& pal : rec)
	{
		wxPGChoiceEntry e(pal);
		wxFont f = e.GetFont();
		f.SetWeight(wxFontWeight::wxFONTWEIGHT_BOLD);
		e.SetFont(f);
		list.Add(e);
	}
	for (const auto& pal : entry->GetAllPalettes())
	{
		if (list.Index(pal) < 0)
		{
			wxPGChoiceEntry e(pal);
			e.SetFgCol(wxColor(192, 192, 192));
			list.Add(e);
		}
	}
	return list;
}

void TilesetEditorFrame::RefreshProperties(wxPropertyGridManager& props) const
{
	if (m_animated_tileset_entry)
	{
		props.GetGrid()->GetProperty("P")->SetChoices(UpdatePalList(m_animated_tileset_entry, props));
		props.GetGrid()->SetPropertyValue("N", _(m_animated_tileset_entry->GetName()));
		props.GetGrid()->SetPropertyValue("OS", wxString::Format("%lu bytes", m_animated_tileset_entry->GetOrigBytes()->size()));
		props.GetGrid()->SetPropertyValue("SA", _(Hex(m_animated_tileset_entry->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("EA", _(Hex(m_animated_tileset_entry->GetEndAddress())));
		props.GetGrid()->SetPropertyValue("FN", _(m_animated_tileset_entry->GetFilename().string()));
	}
	else if (m_tileset_entry)
	{
		props.GetGrid()->GetProperty("P")->SetChoices(UpdatePalList(m_tileset_entry, props));
		props.GetGrid()->SetPropertyValue("N", _(m_tileset_entry->GetName()));
		props.GetGrid()->SetPropertyValue("OS", wxString::Format("%lu bytes", m_tileset_entry->GetOrigBytes()->size()));
		props.GetGrid()->SetPropertyValue("SA", _(Hex(m_tileset_entry->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("EA", _(Hex(m_tileset_entry->GetEndAddress())));
		props.GetGrid()->SetPropertyValue("FN", _(m_tileset_entry->GetFilename().string()));
	}
	if (m_tileset)
	{

		if (m_animated)
		{
			auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(m_tileset);
			props.GetGrid()->SetPropertyValue("ABT", ats->GetBaseTileset());
			props.GetGrid()->SetPropertyValue("AST", ats->GetStartTile().GetIndex());
			props.GetGrid()->SetPropertyValue("A#T", static_cast<int>(ats->GetFrameSizeTiles()));
			props.GetGrid()->SetPropertyValue("A#F", ats->GetAnimationFrames());
			props.GetGrid()->SetPropertyValue("AS", ats->GetAnimationSpeed());
			props.GetGrid()->GetProperty("A")->Hide(false);
		}
		else
		{
			props.GetGrid()->GetProperty("A")->Hide(true);
		}
		props.GetGrid()->SetPropertyValue("US", wxString::Format("%lu bytes", m_tileset->GetTilesetUncompressedSizeBytes()));
		props.GetGrid()->SetPropertyValue("#", wxString::Format("%lu", m_tileset->GetTileCount()));
		props.GetGrid()->SetPropertyValue("W", wxString::Format("%lu", m_tileset->GetTileWidth()));
		props.GetGrid()->SetPropertyValue("H", wxString::Format("%lu", m_tileset->GetTileHeight()));
		props.GetGrid()->SetPropertyValue("D", wxString::Format("%lu", m_tileset->GetTileBitDepth()));
		props.GetGrid()->SetPropertyValue("B", wxString(Tileset::BLOCKTYPE_STRINGS.at(m_tileset->GetTileBlockType())));
		props.GetGrid()->SetPropertyValue("C", m_tileset->GetCompressed());
		props.GetGrid()->SetPropertyValue("I", wxString(VecToCommaList(m_tileset->GetColourIndicies())));
	}
	props.GetGrid()->SetPropertyValue("P", wxString(m_selected_palette->GetName()));
}

void TilesetEditorFrame::UpdateProperties(wxPropertyGridManager& props) const
{
	EditorFrame::UpdateProperties(props);
	if (ArePropsInitialised() == true)
	{
		RefreshProperties(props);
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
		if (m_tileset_entry)
		{
			m_tileset_entry->SetDefaultPalette(m_selected_palette->GetName());
		}
		if (m_animated_tileset_entry)
		{
			m_animated_tileset_entry->SetDefaultPalette(m_selected_palette->GetName());
		}
	}
	else if (name == "I")
	{
		// Indicies change
		m_tileset->SetColourIndicies(CommaListToVec<uint8_t>(property->GetValueAsString().ToStdString()));
		m_paletteEditor->SetColourIndicies(m_tileset->GetColourIndicies());
		m_tilesetEditor->RedrawTiles();
		m_tileEditor->Refresh();
		if (m_tileset_entry != nullptr)
		{
			m_tileset_entry->SetPalIndicies(VecToCommaList(m_tileset->GetColourIndicies()));
		}
	}
	else if (name == "ABT")
	{
		auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(m_tileset);
		ats->SetBaseTileset(property->GetValuePlain().GetLong());
	}
	else if (name == "AST")
	{
		auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(m_tileset);
		ats->SetStartTile(property->GetValuePlain().GetLong());

	}
	else if (name == "A#T")
	{
		auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(m_tileset);
		ats->SetFrameSizeTiles(property->GetValuePlain().GetLong());

	}
	else if (name == "A#F")
	{
		auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(m_tileset);
		ats->SetAnimationFrames(property->GetValuePlain().GetLong());
	}
	else if (name == "AS")
	{
		auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(m_tileset);
		ats->SetAnimationSpeed(property->GetValuePlain().GetLong());
	}

	FireEvent(EVT_PROPERTIES_UPDATE);
}

void TilesetEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_NEW, "New Tileset");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_BIN, "Export Tileset...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_ALL, "Export All Tilesets...");
	AddMenuItem(fileMenu, 3, ID_FILE_EXPORT_PNG, "Export Tileset as PNG...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_BIN, "Import Tileset...");
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
	tileset_tb->AddSeparator();
	tileset_tb->AddLabel(wxID_ANY, "Zoom:");
	m_zoomslider = new wxSlider(tileset_tb, ID_ZOOM_SLIDER, 8, 1, 16, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	tileset_tb->AddControl(m_zoomslider, "Zoom");
	AddToolbar(m_mgr, *tileset_tb, "Tileset", "Tileset Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));
	
	wxAuiToolBar* draw_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	draw_tb->SetToolBitmapSize(wxSize(16, 16));
	draw_tb->AddTool(ID_DRAW_TOGGLE_GRIDLINES, "Toggle Gridlines", ilist.GetImage("gridlines"), "Toggle Gridlines", wxITEM_CHECK);
	draw_tb->AddSeparator();
	draw_tb->AddTool(ID_PENCIL, "Pencil", ilist.GetImage("pencil"), "Pencil", wxITEM_RADIO);
	AddToolbar(m_mgr, *draw_tb, "Draw", "Drawing Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1));
	
	UpdateUI();

	m_mgr.Update();
}

void TilesetEditorFrame::OnMenuClick(wxMenuEvent& evt)
{
	const auto id = evt.GetId();
	if ((id >= 20000) && (id < 31000))
	{
		switch(id)
		{
		case ID_ADD_TILE_AFTER_SEL:
			InsertTileAfter();
			break;
		case ID_ADD_TILE_BEFORE_SEL:
			InsertTileBefore();
			break;
		case ID_EXTEND_TILESET:
			ExtendTileset();
			break;
		case ID_DELETE_TILE:
			DeleteTile();
			break;
		case ID_SWAP_TILES:
			SwapTiles();
			break;
		case ID_CUT_TILE:
			CutTile();
			break;
		case ID_COPY_TILE:
			CopyTile();
			break;
		case ID_PASTE_TILE:
			PasteTile();
			break;
		case ID_FILE_NEW:
			New();
			break;
		case ID_FILE_EXPORT_BIN:
			ExportAsBin();
			break;
		case ID_FILE_EXPORT_ALL:
			ExportAll();
			break;
		case ID_FILE_EXPORT_PNG:
			ExportAsPng();
			break;
		case ID_FILE_IMPORT_BIN:
			ImportFromBin();
			break;
		case ID_FILE_IMPORT_PNG:
			ImportFromPng();
			break;
		case ID_VIEW_TOGGLE_GRIDLINES:
		case ID_TOGGLE_GRIDLINES:
			ToggleGrid();
			break;
		case ID_VIEW_TOGGLE_TILE_NOS:
		case ID_TOGGLE_TILE_NOS:
			ToggleTileNums();
			break;
		case ID_VIEW_TOGGLE_ALPHA:
		case ID_TOGGLE_ALPHA:
			ToggleAlpha();
			break;
		case ID_DRAW_TOGGLE_GRIDLINES:
			ToggleDrawGrid();
			break;
		case ID_PENCIL:
			SelectDrawPencil();
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
		case ID_ZOOM_SLIDER:
			break;
		default:
			wxMessageBox(wxString::Format("Unrecognised Event %d", evt.GetId()));
		}
		UpdateUI();
	}
}

void TilesetEditorFrame::ClearMenu(wxMenuBar& menu) const
{
	// The toolbar destructor deletes these, but doesn't clear the pointer
	m_zoomslider = nullptr;
	EditorFrame::ClearMenu(menu);
}

void TilesetEditorFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	m_tileEditor->SetGameData(gd);
	m_paletteEditor->SetGameData(gd);
	m_tilesetEditor->SetGameData(gd);
}

void TilesetEditorFrame::ClearGameData()
{
	m_gd = nullptr;
	m_selected_palette = nullptr;
	m_tileset = nullptr;
	m_tileset_entry = nullptr;
	m_animated_tileset_entry = nullptr;
	m_tilesetEditor->SetGameData(nullptr);
	m_paletteEditor->SetGameData(nullptr);
	m_tileEditor->SetGameData(nullptr);
}

void TilesetEditorFrame::SetActivePalette(std::string name)
{
	if (name == "")
	{
		name = m_gd->GetAllPalettes().cbegin()->first;
	}
	m_selected_palette = m_gd->GetPalette(name);
	m_tilesetEditor->SetActivePalette(name);
	m_paletteEditor->SelectPalette(name);
	m_tileEditor->SetActivePalette(name);
	FireEvent(EVT_PROPERTIES_UPDATE);
}

bool TilesetEditorFrame::Open(std::vector<uint8_t>& pixels, bool uses_compression, int tile_width, int tile_height, int tile_bitdepth)
{
	bool retval = false;
	retval = m_tilesetEditor->Open(pixels, uses_compression, tile_width, tile_height, tile_bitdepth);
	m_animated = false;
	m_animated_tileset_entry = nullptr;
	m_tileset_entry = nullptr;
	m_tileset = nullptr;
	if (retval)
	{
		m_tileset = m_tilesetEditor->GetTileset();
		m_tile = 0;
		m_tilesetEditor->SelectTile(m_tile.GetIndex());
		m_tileEditor->SetTileset(m_tileset);
		m_tileEditor->SetTile(m_tile);
		m_paletteEditor->SetBitsPerPixel(tile_bitdepth);
	}
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	return retval;
}

bool TilesetEditorFrame::Open(const std::string& name)
{
	auto e = m_gd->GetTileset(name);
	bool retval = m_tilesetEditor->Open(e->GetData());
	m_animated = false;
	m_animated_tileset_entry = nullptr;
	m_tileset_entry = nullptr;
	m_tileset = nullptr;
	if (retval)
	{
		m_tileset_entry = e;
		m_tileset = m_tilesetEditor->GetTileset();
		m_tileset->SetColourIndicies(CommaListToVec<uint8_t>(e->GetPaletteIndicies()));
		m_tile = 0;
		m_tilesetEditor->SelectTile(m_tile.GetIndex());
		m_tileEditor->SetTile(m_tile);
		m_tileEditor->SetTileset(m_tileset);
		SetActivePalette(m_tileset_entry->GetDefaultPalette());
		m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());
		m_paletteEditor->SetColourIndicies(m_tileset->GetColourIndicies());
	}
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	return retval;
}

bool TilesetEditorFrame::OpenAnimated(const std::string& name)
{
	auto e = m_gd->GetAnimatedTileset(name);
	bool retval = m_tilesetEditor->Open(e->GetData());
	m_animated_tileset_entry = nullptr;
	m_tileset_entry = nullptr;
	m_tileset = nullptr;
	if (retval)
	{
		m_animated_tileset_entry = e;
		m_animated = true;
		m_tileset = m_tilesetEditor->GetTileset();
		m_tile = 0;
		m_tilesetEditor->SelectTile(m_tile.GetIndex());
		m_tileEditor->SetTile(m_tile);
		m_tileEditor->SetTileset(m_tileset);
		SetActivePalette(m_animated_tileset_entry->GetDefaultPalette());
		m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());
		m_paletteEditor->SetColourIndicies(m_tileset->GetColourIndicies());
	}
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	return retval;
}
