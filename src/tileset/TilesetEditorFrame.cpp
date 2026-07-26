#include <tileset/TilesetEditorFrame.h>

#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <algorithm>
#include <array>
#include <landstalker/misc/Utils.h>
#include <landstalker/misc/Labels.h>
#include <landstalker/main/ImageBuffer.h>
#include <landstalker/palettes/Palette.h>
#include <landstalker/tileset/AnimatedTileset.h>
#include <main/ImageBufferWx.h>
#include <rooms/TilesetManagerDialog.h>
#include <tileset/TilesetImportDialog.h>
#include <wx/artprov.h>
#include <wx/dcbuffer.h>
#include <wx/numdlg.h>

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
	ID_SELECT,
	ID_PENCIL,
	ID_LINE,
	ID_RECT_FILLED,
	ID_RECT_OUTLINE,
	ID_CIRCLE_FILLED,
	ID_CIRCLE_OUTLINE,
	ID_FILL,
	ID_PICKER,
	ID_PIXEL_SELECT,
	ID_HFLIP_SEL,
	ID_VFLIP_SEL,
};

enum MENU_IDS
{
	ID_FILE_EXPORT_BIN = 20000,
	ID_FILE_EXPORT_ALL,
	ID_FILE_EXPORT_PNG,
	ID_FILE_IMPORT_BIN,
	ID_FILE_IMPORT_PNG,
	ID_EDIT,
	ID_EDIT_UNDO,
	ID_EDIT_REDO,
	ID_EDIT_TILESETS,
	ID_VIEW,
	ID_VIEW_TOGGLE_GRIDLINES,
	ID_VIEW_TOGGLE_TILE_NOS,
	ID_VIEW_TOGGLE_ALPHA,
	ID_TOOLS,
	ID_TOOLS_PALETTE,
	ID_TOOLS_TILESET_TOOLBAR,
	ID_TOOLS_DRAW_TOOLBAR,
	ID_TOOLS_TOOLS_TOOLBAR
};

wxBEGIN_EVENT_TABLE(TilesetEditorFrame, wxWindow)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_CHANGE, TilesetEditorFrame::OnPaletteChanged)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_SELECT, TilesetEditorFrame::OnPaletteColourSelect)
EVT_COMMAND(wxID_ANY, EVT_PALETTE_COLOUR_HOVER, TilesetEditorFrame::OnPaletteColourHover)
EVT_COMMAND(wxID_ANY, EVT_TILESET_SELECT, TilesetEditorFrame::OnTileEditRequested)
EVT_COMMAND(wxID_ANY, EVT_TILESET_TILE_CHANGE, TilesetEditorFrame::OnTilePixelChanged)
EVT_COMMAND(wxID_ANY, EVT_TILESET_HOVER, TilesetEditorFrame::OnTileSelectionChanged)
EVT_COMMAND(wxID_ANY, EVT_TILESET_CHANGE, TilesetEditorFrame::OnTilesetChange)
EVT_COMMAND(wxID_ANY, EVT_TILESET_COLOUR_PICK, TilesetEditorFrame::OnColourPicked)
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
	// A tileset with no stored palette indices is the normal case for a newly created one.
	// Without this the stoi below throws invalid_argument on the empty token, which the
	// catch swallows to the same result but which shows up as a first-chance exception
	// every time such a tileset is opened.
	if (input.empty())
	{
		return ret;
	}
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

// Plays an animated tileset by cycling through its frames on a timer. A frame is a fixed run of
// tiles, drawn here as a single row scaled up (nearest-neighbour) to fill the pane. It shares the
// same AnimatedTileset the editor works on, so pixel edits appear on the next rebuild for free.
class AnimatedTilesetPreview : public wxWindow
{
public:
	AnimatedTilesetPreview(wxWindow* parent)
		: wxWindow(parent, wxID_ANY),
		  m_frame(0)
	{
		SetBackgroundStyle(wxBG_STYLE_PAINT);
		m_timer.SetOwner(this);
		Bind(wxEVT_PAINT, &AnimatedTilesetPreview::OnPaint, this);
		Bind(wxEVT_TIMER, &AnimatedTilesetPreview::OnTimer, this);
	}

	void SetTileset(std::shared_ptr<Landstalker::AnimatedTileset> ats, std::shared_ptr<Landstalker::Palette> pal)
	{
		m_ats = ats;
		m_pal = pal;
		m_frame = 0;
		RebuildBitmap();
		Refresh();
		RestartTimer();
	}

	void SetPalette(std::shared_ptr<Landstalker::Palette> pal)
	{
		m_pal = pal;
		RebuildBitmap();
		Refresh();
	}

	// Rebuilds the current frame from the (possibly just edited) tileset pixels.
	void RefreshFrame()
	{
		RebuildBitmap();
		Refresh();
	}

	// Restarts the timer, picking up any change to the animation speed or frame count.
	void RestartTimer()
	{
		m_timer.Stop();
		if (!m_ats)
		{
			return;
		}
		const int frames = m_ats->GetAnimationFrames();
		const int speed = m_ats->GetAnimationSpeed();
		if ((frames > 1) && (speed > 0))
		{
			// Speed is how many 60Hz fields each frame is held for; clamp the period so a bad
			// value can't spin the timer flat out.
			m_timer.Start(std::max(16, speed * 1000 / 60));
		}
	}

	void Clear()
	{
		m_timer.Stop();
		m_ats = nullptr;
		m_pal = nullptr;
		m_bmp.reset();
		Refresh();
	}

private:
	void OnTimer(wxTimerEvent&)
	{
		if (!m_ats)
		{
			return;
		}
		const int frames = std::max(1, static_cast<int>(m_ats->GetAnimationFrames()));
		m_frame = (m_frame + 1) % frames;
		RebuildBitmap();
		Refresh();
	}

	void RebuildBitmap()
	{
		m_bmp.reset();
		if (!m_ats || !m_pal)
		{
			return;
		}
		const int fst = static_cast<int>(m_ats->GetFrameSizeTiles());
		const int frames = static_cast<int>(m_ats->GetAnimationFrames());
		if ((fst <= 0) || (frames <= 0))
		{
			return;
		}
		if (m_frame >= frames)
		{
			m_frame = 0;
		}
		const int tw = m_ats->GetTileWidth();
		const int th = m_ats->GetTileHeight();
		m_buf.Resize(tw * fst, th);
		for (int i = 0; i < fst; ++i)
		{
			m_buf.InsertTile(i * tw, 0, 0, m_frame * fst + i, *m_ats, true);
		}
		m_bmp = m_buf.MakeBitmap({ m_pal }, true);
	}

	void OnPaint(wxPaintEvent&)
	{
		wxAutoBufferedPaintDC dc(this);
		dc.SetBackground(wxBrush(GetBackgroundColour()));
		dc.Clear();
		if (!m_bmp)
		{
			return;
		}
		const wxSize client = GetClientSize();
		const int bw = m_bmp->GetWidth();
		const int bh = m_bmp->GetHeight();
		if ((bw <= 0) || (bh <= 0) || (client.x <= 0) || (client.y <= 0))
		{
			return;
		}
		// Largest integer zoom that fits, so pixels stay square and sharp.
		const int zoom = std::max(1, std::min(client.x / bw, client.y / bh));
		wxImage img = m_bmp->ConvertToImage();
		img.Rescale(bw * zoom, bh * zoom); // nearest-neighbour by default
		const int x = (client.x - bw * zoom) / 2;
		const int y = (client.y - bh * zoom) / 2;
		dc.DrawBitmap(wxBitmap(img), x, y, true);
	}

	std::shared_ptr<Landstalker::AnimatedTileset> m_ats;
	std::shared_ptr<Landstalker::Palette> m_pal;
	wxTimer m_timer;
	int m_frame;
	ImageBufferWx m_buf;
	std::shared_ptr<wxBitmap> m_bmp;
};

TilesetEditorFrame::TilesetEditorFrame(wxWindow* parent, ImageList* imglst)
	: EditorFrame(parent, wxID_ANY, imglst),
	  m_tile(0),
	  m_inputBuffer(0),
	  m_animated(false),
	  m_title("")
{
	m_mgr.SetManagedWindow(this);
	m_statusbar_timer.SetOwner(this);
	Bind(wxEVT_TIMER, &TilesetEditorFrame::OnStatusBarTimer, this);

	m_tilesetEditor = new TilesetEditor(this);
	m_paletteEditor = new PaletteEditor(this);
	m_animPreview = new AnimatedTilesetPreview(this);

	// Drawing happens directly on the tileset canvas; the limiter stops the pencil straying
	// past a glyph's width when the end credit font is open.
	m_tilesetEditor->SetDrawWidthLimiter([this](int tile) {
		return m_font_entry ? static_cast<int>(m_font_entry->GetGlyphWidth(tile)) : 0;
	});

	// add the panes to the manager
	m_mgr.SetDockSizeConstraint(0.3, 0.3);
	m_mgr.AddPane(m_paletteEditor, wxAuiPaneInfo().Bottom().Layer(1).MinSize(180, 40).BestSize(700, 100).FloatingSize(700,100).Caption("Palette"));
	// Hidden until an animated tileset is opened - it has nothing to show for a normal one.
	m_mgr.AddPane(m_animPreview, wxAuiPaneInfo().Right().Layer(1).MinSize(120, 80).BestSize(220, 140).FloatingSize(220, 140).Caption("Animation Preview").Hide());
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

void TilesetEditorFrame::RequestStatusBarUpdate()
{
	if (!m_statusbar_timer.IsRunning())
	{
		m_statusbar_timer.StartOnce(50);
	}
}

void TilesetEditorFrame::OnStatusBarTimer(wxTimerEvent& /*evt*/)
{
	// UpdateStatusBar reads the live editor state, so whatever happened during the timer
	// window is reflected in this one rebuild.
	FireEvent(EVT_STATUSBAR_UPDATE);
}

void TilesetEditorFrame::OnZoom(wxCommandEvent& evt)
{
	RequestStatusBarUpdate();
	m_tilesetEditor->SetPixelSize(m_zoomslider->GetValue());
	evt.Skip();
}

void TilesetEditorFrame::OnTileSelectionChanged(wxCommandEvent& evt)
{
	RequestStatusBarUpdate();
	evt.Skip();
}

void TilesetEditorFrame::OnTilesetChange(wxCommandEvent& evt)
{
	m_tilesetEditor->RedrawTiles();
	// Inserting or deleting shifts the selection, so track it - the glyph width property keys
	// off m_tile and would otherwise describe a different tile.
	m_tile = m_tilesetEditor->GetSelectedTile();
	m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());
	if (m_animated && m_animPreview)
	{
		m_animPreview->RefreshFrame();
	}
	FireEvent(EVT_PROPERTIES_UPDATE);
	RequestStatusBarUpdate();
	// Keeps the Undo/Redo menu enablement in step with mouse-driven edits.
	UpdateUI();
	evt.Skip();
}

void TilesetEditorFrame::OnTilePixelChanged(wxCommandEvent& evt)
{
	if (m_animated && m_animPreview)
	{
		m_animPreview->RefreshFrame();
	}
	RequestStatusBarUpdate();
	UpdateUI();
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
	if (m_tilesetEditor != nullptr)
	{
		m_tilesetEditor->SetPixelGridEnabled(!m_tilesetEditor->GetPixelGridEnabled());
	}
}

void TilesetEditorFrame::SelectDrawSelect()
{
	if (m_tilesetEditor != nullptr)
	{
		m_tilesetEditor->SetDrawingEnabled(false);
		m_tilesetEditor->SetSelectionEnabled(true);
	}
}

void TilesetEditorFrame::SelectDrawPencil()
{
	SelectDrawTool(TilesetEditor::Tool::Pencil);
}

void TilesetEditorFrame::SelectDrawTool(TilesetEditor::Tool tool)
{
	if (m_tilesetEditor != nullptr)
	{
		// The drawing tools own the mouse: no selection, and the selection-based toolbar
		// operations grey out (see UpdateUI).
		m_tilesetEditor->SetDrawingEnabled(true);
		m_tilesetEditor->SetSelectionEnabled(false);
		m_tilesetEditor->SetDrawTool(tool);
	}
}

void TilesetEditorFrame::Save()
{
}

void TilesetEditorFrame::SaveAs()
{
}

void TilesetEditorFrame::ShowAnimationPreview(std::shared_ptr<Landstalker::AnimatedTileset> ats)
{
	if (m_animPreview == nullptr)
	{
		return;
	}
	m_animPreview->SetTileset(ats, m_selected_palette ? m_selected_palette->GetData() : nullptr);
	auto& pane = m_mgr.GetPane(m_animPreview);
	if (pane.IsOk() && !pane.IsShown())
	{
		pane.Show();
		m_mgr.Update();
	}
}

void TilesetEditorFrame::HideAnimationPreview()
{
	if (m_animPreview == nullptr)
	{
		return;
	}
	m_animPreview->Clear();
	auto& pane = m_mgr.GetPane(m_animPreview);
	if (pane.IsOk() && pane.IsShown())
	{
		pane.Hide();
		m_mgr.Update();
	}
}

void TilesetEditorFrame::ShowTilesetManagerDialog()
{
	if (!m_gd)
	{
		return;
	}
	// Open on whatever is being edited, so the dialog lands on the tileset in front of you.
	std::string select;
	if (m_animated_tileset_entry)
	{
		select = m_animated_tileset_entry->GetName();
	}
	else if (m_tileset_entry)
	{
		select = m_tileset_entry->GetName();
	}

	TilesetManagerDialog dlg(this, m_gd, select);
	dlg.ShowModal();

	const auto to_open = dlg.GetTilesetToOpen();
	if (!dlg.HasChanges() && to_open.empty())
	{
		return;
	}
	// Adding, deleting, renaming or moving a tileset changes the name and number of entries
	// under both Tilesets and Blocksets, so the navigation tree is rebuilt from the game
	// data rather than patched item by item. That closes this editor, hence naming what to
	// reopen - the double-clicked entry if there was one, else whatever was already open.
	std::string target = to_open.empty() ? select : to_open;
	bool animated = to_open.empty() ? (m_animated_tileset_entry != nullptr)
		: dlg.IsTilesetToOpenAnimated();
	// A rename or delete can leave the old name meaningless; fall back to the first tileset.
	if (target.empty() ||
		(animated ? m_gd->GetRoomData()->GetAnimatedTileset(target) == nullptr
				  : m_gd->GetRoomData()->GetAllTilesets().count(target) == 0))
	{
		const auto tilesets = m_gd->GetRoomData()->GetTilesets();
		target = tilesets.empty() ? std::string() : tilesets.front()->GetName();
		animated = false;
	}

	std::wstring path;
	if (!target.empty())
	{
		const std::wstring name(target.cbegin(), target.cend());
		if (animated)
		{
			// Animations are nested under the tileset they belong to.
			const auto anim = m_gd->GetRoomData()->GetAnimatedTileset(target);
			const auto parent = anim
				? m_gd->GetRoomData()->GetTileset(static_cast<uint8_t>(anim->GetIndex().first))
				: nullptr;
			if (parent)
			{
				const auto parent_name = parent->GetName();
				path = L"Tilesets/" + std::wstring(parent_name.cbegin(), parent_name.cend()) + L"/" + name;
			}
		}
		else
		{
			path = L"Tilesets/" + name;
		}
	}

	if (!dlg.HasChanges())
	{
		// Nothing moved, so the tree is still correct - just follow the double-click.
		wxCommandEvent evt(EVT_GO_TO_NAV_ITEM);
		evt.SetString(wxString(path));
		evt.SetClientData(this);
		wxPostEvent(this, evt);
		return;
	}

	wxCommandEvent evt(EVT_REBUILD_NAV_TREE);
	evt.SetInt(-1);
	evt.SetString(wxString(path));
	evt.SetClientData(this);
	wxPostEvent(this, evt);
}

void TilesetEditorFrame::ImportFromBin()
{
	wxFileDialog fd(this, _("Import Tileset From Binary"), "", "", "Uncompressed Tileset (*.bin)|*.bin|Compressed Tileset (*.lz77)|*.lz77|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_CANCEL)
	{
		std::string path = fd.GetPath().ToStdString();
		bool use_compression = path.substr(path.find_last_of(".") + 1) == "lz77";
		auto bytes = Landstalker::ReadBytes(path);
		m_tileset->SetBits(bytes, use_compression);
		m_tilesetEditor->ForceRedraw();
		m_tilesetEditor->SelectTile(0);
		m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());

		FireEvent(EVT_PROPERTIES_UPDATE);
	}
}

void TilesetEditorFrame::ImportFromPng()
{
	wxFileDialog fd(this, _("Import Tileset From PNG"), "", "",
		"PNG Image (*.png)|*.png|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_CANCEL)
	{
		return;
	}
	const std::string path = fd.GetPath().ToStdString();
	const auto image = Landstalker::ImageBuffer::ReadIndexedPNG(path);
	if (!image.ok)
	{
		wxMessageBox("The PNG image could not be decoded.", "Import Tileset from PNG",
			wxOK | wxICON_ERROR, this);
		return;
	}
	if (!image.indexed)
	{
		wxMessageBox("The PNG must be a colour-indexed (palette) image.", "Import Tileset from PNG",
			wxOK | wxICON_ERROR, this);
		return;
	}

	// Read the tile geometry and depth from the tileset being edited; the image is cut to fit it,
	// dropping any partial column or row at the far right / bottom edge.
	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	const std::size_t bit_depth = m_tileset->GetTileBitDepth();
	const std::size_t max_tiles = 1024;

	const std::size_t recognised = std::min(m_tileset->CountWholeTiles(image.width, image.height), max_tiles);
	if (recognised == 0)
	{
		wxMessageBox(wxString::Format("The image is too small to hold a whole %dx%d tile.", tw, th),
			"Import Tileset from PNG", wxOK | wxICON_ERROR, this);
		return;
	}

	// Reject the whole import if any colour in those tiles lies outside the tileset's palette.
	const int max_index = m_tileset->MaxColourIndexInTiles(image.pixels, image.width, image.height, recognised);
	const int palette_size = 1 << bit_depth;
	if (max_index >= palette_size)
	{
		wxMessageBox(wxString::Format("The image uses colour index %d, but this tileset is %d bits "
			"per pixel (indices 0-%d).", max_index, static_cast<int>(bit_depth), palette_size - 1),
			"Import Tileset from PNG", wxOK | wxICON_ERROR, this);
		return;
	}

	// Work out which palette flow applies. A room tileset (one RoomData owns) matches against the
	// room palettes; any other >=4bpp tileset can overwrite its own palette; below 4bpp there is no
	// palette to import.
	const std::string name = m_tileset_entry ? m_tileset_entry->GetName()
		: (m_animated_tileset_entry ? m_animated_tileset_entry->GetName() : std::string());
	const bool is_room = m_gd->GetRoomData() && !name.empty() &&
		m_gd->GetRoomData()->GetTileset(name) != nullptr;
	TilesetImportDialog::PaletteMode mode = TilesetImportDialog::PaletteMode::None;
	if (bit_depth >= 4)
	{
		mode = is_room ? TilesetImportDialog::PaletteMode::RoomMatch
			: TilesetImportDialog::PaletteMode::Overwrite;
	}

	// The tileset's present palette, for previewing tiles that are not being recoloured.
	std::array<uint16_t, 16> current_palette{};
	current_palette.fill(0);
	if (m_selected_palette)
	{
		// Only read as many entries as the depth uses: a sub-4bpp palette (e.g. the 2bpp font) may
		// hold fewer than 16 colours, and getGenesisColour is unchecked.
		const auto pal = m_selected_palette->GetData();
		for (int i = 0; i < 16 && i < palette_size; ++i)
		{
			current_palette[i] = pal->getGenesisColour(static_cast<uint8_t>(i));
		}
	}
	wxArrayString room_names;
	if (mode == TilesetImportDialog::PaletteMode::RoomMatch)
	{
		const auto& palettes = m_gd->GetRoomData()->GetRoomPalettes();
		for (std::size_t i = 0; i < palettes.size(); ++i)
		{
			room_names.Add(wxString(m_gd->GetRoomData()->GetRoomPaletteDisplayName(static_cast<uint8_t>(i))));
		}
	}
	const std::string overwrite_name = m_selected_palette ? m_selected_palette->GetName() : std::string();

	TilesetImportDialog dlg(this, m_gd, image, tw, th, static_cast<int>(bit_depth),
		static_cast<int>(recognised), mode, room_names, overwrite_name, current_palette);
	if (dlg.ShowModal() != wxID_OK)
	{
		return;
	}

	const int count = dlg.GetTileCount();
	m_tileset->SetTilesFromIndexedImage(image.pixels, image.width, image.height,
		static_cast<std::size_t>(count));

	// The end credit font carries a per-glyph advance width; give each imported glyph the full tile
	// width so nothing is clipped until the user narrows it.
	if (m_font_entry)
	{
		for (int i = 0; i < count; ++i)
		{
			m_font_entry->SetGlyphWidth(static_cast<std::size_t>(i), static_cast<uint8_t>(tw));
		}
	}

	// Apply the palette choice.
	if (mode == TilesetImportDialog::PaletteMode::RoomMatch)
	{
		int pidx = dlg.RoomExistingIndex();
		if (dlg.RoomCreateNew())
		{
			if (const auto added = m_gd->GetRoomData()->AddRoomPalette())
			{
				pidx = *added;
				const auto pal = m_gd->GetRoomData()->GetRoomPalette(static_cast<uint8_t>(pidx))->GetData();
				const auto cols = dlg.RoomColours();
				for (int n = 0; n < static_cast<int>(cols.size()); ++n)
				{
					pal->SetNthUnlockedGenesisColour(static_cast<uint8_t>(n), cols[n]);
				}
				const auto new_name = dlg.RoomNewName();
				if (!new_name.empty())
				{
					Landstalker::Labels::Update(Landstalker::Labels::C_ROOM_PALETTES, pidx, new_name);
				}
			}
		}
		if (pidx >= 0 && m_tileset_entry)
		{
			// Point the tileset at the matched/created room palette so it previews with those colours.
			const auto internal = m_gd->GetRoomData()->GetRoomPalette(static_cast<uint8_t>(pidx))->GetName();
			m_tileset_entry->SetDefaultPalette(internal);
			SetActivePalette(internal);
		}
	}
	else if (mode == TilesetImportDialog::PaletteMode::Overwrite && dlg.Overwrite() && m_selected_palette)
	{
		// Overwrite only the palette's editable slots, reading each from the same index in the image.
		const auto pal = m_selected_palette->GetData();
		Landstalker::Palette::Colour colour;
		for (int n = 0; n < pal->GetSize(); ++n)
		{
			const uint8_t index = pal->GetNthUnlockedIndex(static_cast<uint8_t>(n));
			colour.FromRGB(index < image.palette.size() ? image.palette[index] : 0);
			pal->SetNthUnlockedGenesisColour(static_cast<uint8_t>(n), colour.GetGenesis());
		}
	}

	// The import resizes the tileset, so recompute the grid's row count before redrawing - otherwise
	// the editor keeps showing the old number of rows.
	m_tilesetEditor->UpdateRowCount();
	m_tilesetEditor->ForceRedraw();
	m_tilesetEditor->SelectTile(0);
	m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());
	FireEvent(EVT_PROPERTIES_UPDATE);
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
		Landstalker::WriteBytes(bytes, path);
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
		ImageBufferWx buf(cols * m_tileset->GetTileWidth(), rows * m_tileset->GetTileHeight());
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
	CheckMenuItem(ID_TOOLS_TILESET_TOOLBAR, IsToolbarVisible("Tileset"));
	CheckMenuItem(ID_TOOLS_DRAW_TOOLBAR, IsToolbarVisible("Draw"));
	CheckMenuItem(ID_TOOLS_TOOLS_TOOLBAR, IsToolbarVisible("Tools"));
	if (m_tilesetEditor != nullptr)
	{
		CheckMenuItem(ID_VIEW_TOGGLE_ALPHA, !m_tilesetEditor->GetAlphaEnabled());
		CheckToolbarItem("Tileset", ID_TOGGLE_ALPHA, !m_tilesetEditor->GetAlphaEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_GRIDLINES, m_tilesetEditor->GetBordersEnabled());
		CheckToolbarItem("Tileset", ID_TOGGLE_GRIDLINES, m_tilesetEditor->GetBordersEnabled());
		CheckMenuItem(ID_VIEW_TOGGLE_TILE_NOS, m_tilesetEditor->GetTileNumbersEnabled());
		CheckToolbarItem("Tileset", ID_TOGGLE_TILE_NOS, m_tilesetEditor->GetTileNumbersEnabled());
		CheckToolbarItem("Draw", ID_DRAW_TOGGLE_GRIDLINES, m_tilesetEditor->GetPixelGridEnabled());
		const bool drawing = m_tilesetEditor->GetDrawingEnabled();
		const auto tool = m_tilesetEditor->GetDrawTool();
		CheckToolbarItem("Tools", ID_SELECT, !drawing);
		CheckToolbarItem("Tools", ID_PENCIL, drawing && (tool == TilesetEditor::Tool::Pencil));
		CheckToolbarItem("Tools", ID_LINE, drawing && (tool == TilesetEditor::Tool::Line));
		CheckToolbarItem("Tools", ID_RECT_FILLED, drawing && (tool == TilesetEditor::Tool::RectangleFilled));
		CheckToolbarItem("Tools", ID_RECT_OUTLINE, drawing && (tool == TilesetEditor::Tool::RectangleOutline));
		CheckToolbarItem("Tools", ID_CIRCLE_FILLED, drawing && (tool == TilesetEditor::Tool::CircleFilled));
		CheckToolbarItem("Tools", ID_CIRCLE_OUTLINE, drawing && (tool == TilesetEditor::Tool::CircleOutline));
		CheckToolbarItem("Tools", ID_FILL, drawing && (tool == TilesetEditor::Tool::Fill));
		CheckToolbarItem("Tools", ID_PICKER, drawing && (tool == TilesetEditor::Tool::Picker));
		CheckToolbarItem("Tools", ID_PIXEL_SELECT, drawing && (tool == TilesetEditor::Tool::PixelSelect));
		// The flips act on the pixel selection, which only that tool creates.
		EnableToolbarItem("Tileset", ID_HFLIP_SEL, drawing && (tool == TilesetEditor::Tool::PixelSelect));
		EnableToolbarItem("Tileset", ID_VFLIP_SEL, drawing && (tool == TilesetEditor::Tool::PixelSelect));
		// The drawing tools disable selection, so the operations that act on the selected
		// tile grey out with them.
		const bool sel = !drawing;
		// An animated tileset's size is fixed at frames x frame-size, so anything that changes the
		// tile count is disabled; pixel edits, copy, paste and swap (which preserve the count) stay.
		const bool resize = sel && !m_animated;
		EnableToolbarItem("Tileset", ID_ADD_TILE_BEFORE_SEL, resize);
		EnableToolbarItem("Tileset", ID_ADD_TILE_AFTER_SEL, resize);
		EnableToolbarItem("Tileset", ID_EXTEND_TILESET, resize);
		EnableToolbarItem("Tileset", ID_DELETE_TILE, resize);
		EnableToolbarItem("Tileset", ID_CUT_TILE, resize);
		EnableToolbarItem("Tileset", ID_SWAP_TILES, sel);
		EnableToolbarItem("Tileset", ID_COPY_TILE, sel);
		EnableToolbarItem("Tileset", ID_PASTE_TILE, sel);
		EnableMenuItem(ID_EDIT_UNDO, m_tilesetEditor->CanUndo());
		EnableMenuItem(ID_EDIT_REDO, m_tilesetEditor->CanRedo());
		EnableToolbarItem("Tileset", ID_EDIT_UNDO, m_tilesetEditor->CanUndo());
		EnableToolbarItem("Tileset", ID_EDIT_REDO, m_tilesetEditor->CanRedo());
	}
}

void TilesetEditorFrame::OnTileEditRequested(wxCommandEvent& evt)
{
	int tileId = std::stoi(evt.GetString().ToStdString());
	if (tileId >= 0)
	{
		m_tile = tileId;
	}
	RequestStatusBarUpdate();
	if (m_font_entry)
	{
		// The glyph width property follows the selection.
		FireEvent(EVT_PROPERTIES_UPDATE);
	}
	evt.Skip();
}

void TilesetEditorFrame::OnPaletteChanged(wxCommandEvent& evt)
{
	m_paletteEditor->Refresh();
	m_tilesetEditor->RedrawTiles();
	if (m_animated && m_animPreview)
	{
		m_animPreview->RefreshFrame();
	}
	evt.Skip();
}

void TilesetEditorFrame::OnColourPicked(wxCommandEvent& evt)
{
	// Keeps the palette pane's primary/secondary markers in step with keyboard cycling.
	const int value = std::stoi(evt.GetString().ToStdString());
	if (value & 0x100)
	{
		m_paletteEditor->SetSecondaryColour(value & 0xFF);
	}
	else
	{
		m_paletteEditor->SetPrimaryColour(value & 0xFF);
	}
	RequestStatusBarUpdate();
	evt.Skip();
}

void TilesetEditorFrame::OnPaletteColourSelect(wxCommandEvent& evt)
{
	m_tilesetEditor->SetPrimaryColour(m_paletteEditor->GetPrimaryColour());
	m_tilesetEditor->SetSecondaryColour(m_paletteEditor->GetSecondaryColour());
	// The status bar shows the pen colours.
	RequestStatusBarUpdate();
	evt.Skip();
}

void TilesetEditorFrame::OnPaletteColourHover(wxCommandEvent& evt)
{
	RequestStatusBarUpdate();
	evt.Skip();
}

void TilesetEditorFrame::UpdateStatusBar(wxStatusBar& status, wxCommandEvent& /*evt*/) const
{
	std::ostringstream ss;
	ss << "Selected Tile " << m_tile.GetIndex();
	int colour = m_paletteEditor->GetHoveredColour();
	if (m_tilesetEditor->IsPixelHoverValid())
	{
		const auto pixel = m_tilesetEditor->GetHoveredPixel();
		int idx = m_tilesetEditor->GetColourAtPixel(m_tilesetEditor->GetHoveredTile(), pixel);
		if (idx >= 0)
		{
			colour = m_tilesetEditor->GetColour(idx);
			ss << ": (" << pixel.x << ", " << pixel.y << "): " << idx;
		}
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
		ss << Landstalker::StrPrintf("Colour at mouse: Index %d - Genesis 0x%04X, RGB #%06X %s", colour,
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
		ss << "Zoom: " << m_zoomslider->GetValue() << ", ";
	}
	ss << "Pen: " << static_cast<int>(m_tilesetEditor->GetPrimaryColour())
	   << " / " << static_cast<int>(m_tilesetEditor->GetSecondaryColour());
	if (m_tilesetEditor->IsPixelHoverValid())
	{
		const int idx = m_tilesetEditor->GetColourAtPixel(m_tilesetEditor->GetHoveredTile(),
			m_tilesetEditor->GetHoveredPixel());
		if (idx >= 0)
		{
			ss << ", Hover: " << idx;
		}
	}
	status.SetStatusText(ss.str(), 2);
}

void TilesetEditorFrame::InitProperties(wxPropertyGridManager& props) const
{
	if(ArePropsInitialised() == false)
	{
		for (const auto& b : Landstalker::Tileset::BLOCKTYPE_STRINGS)
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
		props.Append(new wxPropertyCategory("Font", "F"));
		props.Append(new wxIntProperty("Glyph Width", "FW", 0));
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
		props.GetGrid()->SetPropertyValue("N", wxString(m_animated_tileset_entry->GetName()));
		props.GetGrid()->SetPropertyValue("OS", wxString::Format("%zu bytes", m_animated_tileset_entry->GetOrigBytes()->size()));
		props.GetGrid()->SetPropertyValue("SA", wxString(Landstalker::Hex(m_animated_tileset_entry->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("EA", wxString(Landstalker::Hex(m_animated_tileset_entry->GetEndAddress())));
		props.GetGrid()->SetPropertyValue("FN", wxString(m_animated_tileset_entry->GetFilename().string()));
	}
	else if (m_tileset_entry)
	{
		props.GetGrid()->GetProperty("P")->SetChoices(UpdatePalList(m_tileset_entry, props));
		props.GetGrid()->SetPropertyValue("N", wxString(m_tileset_entry->GetName()));
		props.GetGrid()->SetPropertyValue("OS", wxString::Format("%zu bytes", m_tileset_entry->GetOrigBytes()->size()));
		props.GetGrid()->SetPropertyValue("SA", wxString(Landstalker::Hex(m_tileset_entry->GetStartAddress())));
		props.GetGrid()->SetPropertyValue("EA", wxString(Landstalker::Hex(m_tileset_entry->GetEndAddress())));
		props.GetGrid()->SetPropertyValue("FN", wxString(m_tileset_entry->GetFilename().string()));
	}
	if (m_tileset)
	{

		if (m_animated)
		{
			auto ats = std::static_pointer_cast<Landstalker::AnimatedTileset, Landstalker::Tileset>(m_tileset);
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
		props.GetGrid()->SetPropertyValue("US", wxString::Format("%zu bytes", m_tileset->GetTilesetUncompressedSizeBytes()));
		props.GetGrid()->SetPropertyValue("#", wxString::Format("%zu", m_tileset->GetTileCount()));
		props.GetGrid()->SetPropertyValue("W", wxString::Format("%zu", m_tileset->GetTileWidth()));
		props.GetGrid()->SetPropertyValue("H", wxString::Format("%zu", m_tileset->GetTileHeight()));
		props.GetGrid()->SetPropertyValue("D", wxString::Format("%zu", m_tileset->GetTileBitDepth()));
		props.GetGrid()->SetPropertyValue("B", wxString(Landstalker::Tileset::BLOCKTYPE_STRINGS.at(m_tileset->GetTileBlockType())));
		props.GetGrid()->SetPropertyValue("C", m_tileset->GetCompressed());
		props.GetGrid()->SetPropertyValue("I", wxString(VecToCommaList(m_tileset->GetColourIndicies())));
	}
	// The property grid is shared between editors, so the font category is not necessarily present.
	if (auto* font_category = props.GetGrid()->GetProperty("F"))
	{
		if (m_font_entry)
		{
			props.GetGrid()->SetPropertyValue("FW", static_cast<int>(m_font_entry->GetGlyphWidth(m_tile.GetIndex())));
		}
		font_category->Hide(m_font_entry == nullptr);
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
		if (m_tileset_entry != nullptr)
		{
			m_tileset_entry->SetPalIndicies(VecToCommaList(m_tileset->GetColourIndicies()));
		}
	}
	else if (name == "FW")
	{
		// A width narrower than the glyph's own pixels is ignored, so the grid may snap back to a
		// larger value on the properties update fired below.
		if (m_font_entry)
		{
			m_font_entry->SetGlyphWidth(m_tile.GetIndex(), static_cast<uint8_t>(property->GetValuePlain().GetLong()));
		}
	}
	else if (name == "ABT")
	{
		auto ats = std::static_pointer_cast<Landstalker::AnimatedTileset, Landstalker::Tileset>(m_tileset);
		ats->SetBaseTileset(property->GetValuePlain().GetLong());
	}
	else if (name == "AST")
	{
		auto ats = std::static_pointer_cast<Landstalker::AnimatedTileset, Landstalker::Tileset>(m_tileset);
		ats->SetStartTile(property->GetValuePlain().GetLong());

	}
	else if (name == "A#T")
	{
		auto ats = std::static_pointer_cast<Landstalker::AnimatedTileset, Landstalker::Tileset>(m_tileset);
		ats->SetFrameSizeTiles(property->GetValuePlain().GetLong());
		// The frame size drives the one-frame-per-row layout and the preview's row width.
		m_tilesetEditor->SetFixedColumns(static_cast<int>(ats->GetFrameSizeTiles()));
		if (m_animPreview)
		{
			m_animPreview->RefreshFrame();
			m_animPreview->RestartTimer();
		}
	}
	else if (name == "A#F")
	{
		auto ats = std::static_pointer_cast<Landstalker::AnimatedTileset, Landstalker::Tileset>(m_tileset);
		ats->SetAnimationFrames(property->GetValuePlain().GetLong());
		if (m_animPreview)
		{
			m_animPreview->RefreshFrame();
			m_animPreview->RestartTimer();
		}
	}
	else if (name == "AS")
	{
		auto ats = std::static_pointer_cast<Landstalker::AnimatedTileset, Landstalker::Tileset>(m_tileset);
		ats->SetAnimationSpeed(property->GetValuePlain().GetLong());
		if (m_animPreview)
		{
			m_animPreview->RestartTimer();
		}
	}

	FireEvent(EVT_PROPERTIES_UPDATE);
}

void TilesetEditorFrame::InitMenu(wxMenuBar& menu, ImageList& ilist) const
{
	auto* parent = m_mgr.GetManagedWindow();

	ClearMenu(menu);
	auto& fileMenu = *menu.GetMenu(menu.FindMenu("File"));
	AddMenuItem(fileMenu, 0, ID_FILE_EXPORT_BIN, "Export Tileset...");
	AddMenuItem(fileMenu, 1, ID_FILE_EXPORT_ALL, "Export All Tilesets...");
	AddMenuItem(fileMenu, 2, ID_FILE_EXPORT_PNG, "Export Tileset as PNG...");
	AddMenuItem(fileMenu, 3, ID_FILE_IMPORT_BIN, "Import Tileset...");
	AddMenuItem(fileMenu, 4, ID_FILE_IMPORT_PNG, "Import Tileset from PNG...");
	// The manager is reachable from the room editor too, but this is where someone looking
	// to add or reorder a tileset would go first.
	auto& editMenu = AddMenu(menu, 1, ID_EDIT, "Edit");
	AddMenuItem(editMenu, 0, ID_EDIT_UNDO, "Undo\tCtrl+Z");
	AddMenuItem(editMenu, 1, ID_EDIT_REDO, "Redo\tCtrl+Y");
	AddMenuItem(editMenu, 2, ID_EDIT_TILESETS, "Tilesets...\tF10");
	auto& viewMenu = AddMenu(menu, 2, ID_VIEW, "View");
	AddMenuItem(viewMenu, 0, ID_VIEW_TOGGLE_GRIDLINES, "Gridlines", wxITEM_CHECK);
	AddMenuItem(viewMenu, 1, ID_VIEW_TOGGLE_TILE_NOS, "Tile Numbers", wxITEM_CHECK);
	AddMenuItem(viewMenu, 2, ID_VIEW_TOGGLE_ALPHA, "Show Alpha as Black", wxITEM_CHECK);
	auto& toolsMenu = AddMenu(menu, 3, ID_TOOLS, "Tools");
	AddMenuItem(toolsMenu, 0, ID_TOOLS_PALETTE, "Palette", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 1, ID_TOOLS_TILESET_TOOLBAR, "Tileset Toolbar", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 2, ID_TOOLS_DRAW_TOOLBAR, "Draw Toolbar", wxITEM_CHECK);
	AddMenuItem(toolsMenu, 3, ID_TOOLS_TOOLS_TOOLBAR, "Tools Toolbar", wxITEM_CHECK);
	
	wxAuiToolBar* tileset_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	tileset_tb->SetToolBitmapSize(wxSize(16, 16));
	// Same IDs as the Edit menu entries, so they share the handler and enable state.
	tileset_tb->AddTool(ID_EDIT_UNDO, "Undo", ilist.GetImage("undo"), "Undo (Ctrl+Z)");
	tileset_tb->AddTool(ID_EDIT_REDO, "Redo", ilist.GetImage("redo"), "Redo (Ctrl+Y)");
	tileset_tb->AddSeparator();
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
	tileset_tb->AddTool(ID_HFLIP_SEL, "Flip Selection Horizontally", ilist.GetImage("hflip"),
		"Flip Selection Horizontally (Ctrl+H)");
	tileset_tb->AddTool(ID_VFLIP_SEL, "Flip Selection Vertically", ilist.GetImage("vflip"),
		"Flip Selection Vertically (Ctrl+E)");
	tileset_tb->AddSeparator();
	tileset_tb->AddLabel(wxID_ANY, "Zoom:");
	// Zoom runs high enough to draw individual pixels comfortably.
	m_zoomslider = new wxSlider(tileset_tb, ID_ZOOM_SLIDER, 8, 1, 32, wxDefaultPosition, wxSize(80, wxDefaultCoord));
	tileset_tb->AddControl(m_zoomslider, "Zoom");
	AddToolbar(m_mgr, *tileset_tb, "Tileset", "Tileset Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1).Position(1));
	
	wxAuiToolBar* draw_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORIZONTAL);
	draw_tb->SetToolBitmapSize(wxSize(16, 16));
	draw_tb->AddTool(ID_DRAW_TOGGLE_GRIDLINES, "Toggle Pixel Gridlines", ilist.GetImage("gridlines"), "Toggle Pixel Gridlines", wxITEM_CHECK);
	AddToolbar(m_mgr, *draw_tb, "Draw", "Drawing Tools", wxAuiPaneInfo().ToolbarPane().Top().Row(1));

	// Check items with the exclusivity managed in UpdateUI, matching the room editor's tools
	// toolbar: wxAuiToolBar cannot untoggle a radio item programmatically, which left the
	// pencil looking permanently selected.
	wxAuiToolBar* tools_tb = new wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL);
	tools_tb->SetToolBitmapSize(wxSize(16, 16));
	tools_tb->AddTool(ID_SELECT, "Select", ilist.GetImage("mouse"), "Select", wxITEM_CHECK);
	tools_tb->AddTool(ID_PIXEL_SELECT, "Select Pixels", ilist.GetImage("select_rect"),
		"Select Pixels (drag to move, Shift+drag to copy, Ctrl+drag to stamp, Ctrl+C/X/V, Ctrl+H/E to flip)", wxITEM_CHECK);
	tools_tb->AddTool(ID_PICKER, "Colour Picker", ilist.GetImage("dropper"),
		"Colour Picker (left click: primary, right click: secondary)", wxITEM_CHECK);
	tools_tb->AddTool(ID_PENCIL, "Pencil", ilist.GetImage("pencil"), "Pencil", wxITEM_CHECK);
	tools_tb->AddTool(ID_LINE, "Draw Line", ilist.GetImage("line"), "Draw Line", wxITEM_CHECK);
	tools_tb->AddTool(ID_RECT_FILLED, "Draw Filled Rectangle", ilist.GetImage("rect_filled"), "Draw Filled Rectangle", wxITEM_CHECK);
	tools_tb->AddTool(ID_RECT_OUTLINE, "Draw Outlined Rectangle", ilist.GetImage("rect_outline"), "Draw Outlined Rectangle", wxITEM_CHECK);
	tools_tb->AddTool(ID_CIRCLE_FILLED, "Draw Filled Circle", ilist.GetImage("circle_filled"), "Draw Filled Circle", wxITEM_CHECK);
	tools_tb->AddTool(ID_CIRCLE_OUTLINE, "Draw Outlined Circle", ilist.GetImage("circle_outline"), "Draw Outlined Circle", wxITEM_CHECK);
	tools_tb->AddTool(ID_FILL, "Fill", ilist.GetImage("fill"), "Fill", wxITEM_CHECK);
	AddToolbar(m_mgr, *tools_tb, "Tools", "Tools", wxAuiPaneInfo().ToolbarPane().Left().Row(1).Position(1));

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
		case ID_EDIT_UNDO:
			m_tilesetEditor->Undo();
			break;
		case ID_EDIT_REDO:
			m_tilesetEditor->Redo();
			break;
		case ID_EDIT_TILESETS:
			ShowTilesetManagerDialog();
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
		case ID_SELECT:
			SelectDrawSelect();
			break;
		case ID_PIXEL_SELECT:
			SelectDrawTool(TilesetEditor::Tool::PixelSelect);
			break;
		case ID_PICKER:
			SelectDrawTool(TilesetEditor::Tool::Picker);
			break;
		case ID_HFLIP_SEL:
			m_tilesetEditor->FlipSelection(true);
			break;
		case ID_VFLIP_SEL:
			m_tilesetEditor->FlipSelection(false);
			break;
		case ID_PENCIL:
			SelectDrawPencil();
			break;
		case ID_LINE:
			SelectDrawTool(TilesetEditor::Tool::Line);
			break;
		case ID_RECT_FILLED:
			SelectDrawTool(TilesetEditor::Tool::RectangleFilled);
			break;
		case ID_RECT_OUTLINE:
			SelectDrawTool(TilesetEditor::Tool::RectangleOutline);
			break;
		case ID_CIRCLE_FILLED:
			SelectDrawTool(TilesetEditor::Tool::CircleFilled);
			break;
		case ID_CIRCLE_OUTLINE:
			SelectDrawTool(TilesetEditor::Tool::CircleOutline);
			break;
		case ID_FILL:
			SelectDrawTool(TilesetEditor::Tool::Fill);
			break;
		case ID_TOOLS_PALETTE:
			SetPaneVisibility(m_paletteEditor, !IsPaneVisible(m_paletteEditor));
			break;
		case ID_TOOLS_TILESET_TOOLBAR:
			SetToolbarVisibility("Tileset", !IsToolbarVisible("Tileset"));
			break;
		case ID_TOOLS_DRAW_TOOLBAR:
			SetToolbarVisibility("Draw", !IsToolbarVisible("Draw"));
			break;
		case ID_TOOLS_TOOLS_TOOLBAR:
			SetToolbarVisibility("Tools", !IsToolbarVisible("Tools"));
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

void TilesetEditorFrame::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
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
	m_font_entry = nullptr;
	HideAnimationPreview();
	m_tilesetEditor->SetGameData(nullptr);
	m_paletteEditor->SetGameData(nullptr);
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
	if (m_animPreview && m_selected_palette)
	{
		m_animPreview->SetPalette(m_selected_palette->GetData());
	}
	// No properties event here: every caller (Open, OpenAnimated, OnPropertyChange) fires
	// one itself, and each refresh costs ~100ms of property grid rebuild.
}

bool TilesetEditorFrame::Open(std::vector<uint8_t>& pixels, bool uses_compression, int tile_width, int tile_height, int tile_bitdepth)
{
	bool retval = false;
	retval = m_tilesetEditor->Open(pixels, uses_compression, tile_width, tile_height, tile_bitdepth);
	m_animated = false;
	m_animated_tileset_entry = nullptr;
	m_font_entry = nullptr;
	m_tileset_entry = nullptr;
	m_tileset = nullptr;
	m_tilesetEditor->SetFixedColumns(0);
	HideAnimationPreview();
	if (retval)
	{
		m_tileset = m_tilesetEditor->GetTileset();
		m_tile = 0;
		m_tilesetEditor->SelectTile(m_tile.GetIndex());
		m_paletteEditor->SetBitsPerPixel(tile_bitdepth);
	}
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	return retval;
}

bool TilesetEditorFrame::Open(const std::string& name)
{
	auto e = m_gd ? m_gd->GetTileset(name) : nullptr;
	if (!e)
	{
		return false;
	}
	bool retval = m_tilesetEditor->Open(e->GetData());
	m_animated = false;
	m_animated_tileset_entry = nullptr;
	m_font_entry = nullptr;
	m_tileset_entry = nullptr;
	m_tileset = nullptr;
	m_tilesetEditor->SetFixedColumns(0);
	HideAnimationPreview();
	if (retval)
	{
		m_tileset_entry = e;
		m_font_entry = std::dynamic_pointer_cast<Landstalker::EndCreditFontEntry>(e);
		m_tileset = m_tilesetEditor->GetTileset();
		m_tileset->SetColourIndicies(CommaListToVec<uint8_t>(e->GetPaletteIndicies()));
		m_tile = 0;
		m_tilesetEditor->SelectTile(m_tile.GetIndex());
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
	auto e = m_gd ? m_gd->GetAnimatedTileset(name) : nullptr;
	if (!e)
	{
		return false;
	}
	bool retval = m_tilesetEditor->Open(e->GetData());
	m_animated_tileset_entry = nullptr;
	m_font_entry = nullptr;
	m_tileset_entry = nullptr;
	m_tileset = nullptr;
	if (retval)
	{
		m_animated_tileset_entry = e;
		m_animated = true;
		m_tileset = m_tilesetEditor->GetTileset();
		auto ats = std::static_pointer_cast<Landstalker::AnimatedTileset, Landstalker::Tileset>(m_tileset);
		// One animation frame per row: a frame is a fixed run of tiles, so pinning the column
		// count to the frame size lines the frames up vertically.
		m_tilesetEditor->SetFixedColumns(static_cast<int>(ats->GetFrameSizeTiles()));
		m_tile = 0;
		m_tilesetEditor->SelectTile(m_tile.GetIndex());
		SetActivePalette(m_animated_tileset_entry->GetDefaultPalette());
		m_paletteEditor->SetBitsPerPixel(m_tileset->GetTileBitDepth());
		m_paletteEditor->SetColourIndicies(m_tileset->GetColourIndicies());
		ShowAnimationPreview(ats);
	}
	UpdateUI();
	FireEvent(EVT_PROPERTIES_UPDATE);
	return retval;
}
