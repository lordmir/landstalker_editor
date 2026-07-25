#include <tileset/TilesetEditor.h>

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <landstalker/misc/LZ77.h>
#include <landstalker/misc/Utils.h>

wxBEGIN_EVENT_TABLE(TilesetEditor, wxHVScrolledWindow)
EVT_PAINT(TilesetEditor::OnPaint)
EVT_SIZE(TilesetEditor::OnSize)
EVT_KEY_DOWN(TilesetEditor::OnKeyDown)
EVT_LEFT_DOWN(TilesetEditor::OnMouseDown)
EVT_RIGHT_DOWN(TilesetEditor::OnRightDown)
EVT_LEFT_UP(TilesetEditor::OnMouseUp)
EVT_RIGHT_UP(TilesetEditor::OnMouseUp)
EVT_LEFT_DCLICK(TilesetEditor::OnDoubleClick)
EVT_MOTION(TilesetEditor::OnMouseMove)
EVT_LEAVE_WINDOW(TilesetEditor::OnMouseLeave)
EVT_ENTER_WINDOW(TilesetEditor::OnMouseEnter)
EVT_MOUSE_CAPTURE_LOST(TilesetEditor::OnCaptureLost)
EVT_SET_FOCUS(TilesetEditor::OnTilesetFocus)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_TILESET_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_ACTIVATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_COLOUR_PICK, wxCommandEvent);

TilesetEditor::TilesetEditor(wxWindow* parent)
	: wxVScrolledWindow(parent, wxID_ANY),
	m_pixelsize(8),
	m_selectedtile(-1),
	m_hoveredtile(-1),
	m_columns(0),
	m_rows(0),
	m_tilewidth(0),
	m_tileheight(0),
	m_cellwidth(0),
	m_cellheight(0),
	m_enabletilenumbers(false),
	m_enableborders(true),
	m_enableselection(true),
	m_enablehover(true),
	m_enablealpha(true),
	m_enabledrawing(false),
	m_enablepixelgrid(true),
	m_drawing(false),
	m_secondary_active(false),
	m_primary_colour(1),
	m_secondary_colour(0),
	m_hoveredpixel(-1, -1),
	m_last_drawn(-1, -1),
	m_tool(Tool::Pencil),
	m_shape_active(false),
	m_shape_secondary(false),
	m_shape_start(-1, -1),
	m_shape_end(-1, -1),
	m_stroke_dirty(false),
	m_gd(nullptr),
	m_ctrlwidth(1),
	m_ctrlheight(1),
	m_redraw_all(true),
	m_pendingswap(-1)
{
	SetRowCount(m_rows);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	InitialiseBrushesAndPens();
}

TilesetEditor::TilesetEditor(wxWindow* parent, std::shared_ptr<Landstalker::Tileset> tileset)
	: TilesetEditor(parent)
{
	m_tileset = tileset;
}

TilesetEditor::~TilesetEditor()
{
}

void TilesetEditor::SetGameData(std::shared_ptr<Landstalker::GameData> gd)
{
	m_gd = gd;
	if (m_gd == nullptr)
	{
		m_tileset = nullptr;
		m_selected_palette = nullptr;
		m_selected_palette_entry = nullptr;
		m_selected_palette_name = "";
		ClearHistory();
	}
}

std::shared_ptr<Landstalker::Tileset> TilesetEditor::GetTileset()
{
	return m_tileset;
}

bool TilesetEditor::Save(wxString filename, bool compressed)
{
	return m_tileset->Save(filename.ToStdString(), compressed);
}

bool TilesetEditor::Open(wxString filename, bool compressed, int tile_width, int tile_height, int tile_bitdepth)
{
	return Open(std::make_shared<Landstalker::Tileset>(filename.ToStdString(), compressed, tile_width, tile_height, tile_bitdepth));
}

bool TilesetEditor::Open(std::shared_ptr<Landstalker::Tileset> ts)
{
	try
	{
		m_tileset = ts;

		ClearHistory();
		UpdateRowCount();
		ForceRedraw();
		return true;
	}
	catch (std::exception& e)
	{
		Landstalker::Debug(e.what());
	}
	return false;
}

bool TilesetEditor::Open(std::vector<uint8_t>& pixels, bool uses_compression, int tile_width, int tile_height, int tile_bitdepth)
{
	return Open(std::make_shared<Landstalker::Tileset>(pixels, uses_compression, tile_width, tile_height, tile_bitdepth));
}

bool TilesetEditor::New(int r, int c)
{
	m_tileset = std::make_shared<Landstalker::Tileset>(r, c);
	return true;
}

void TilesetEditor::RedrawTiles(int index)
{
	if ((index < 0) || (index >= static_cast<int>(m_tileset->GetTileCount())))
	{
		ForceRedraw();
	}
	else
	{
		m_redraw_list.insert(index);
		Refresh(true);
	}
}

wxCoord TilesetEditor::OnGetRowHeight(size_t /*row*/) const
{
	return wxCoord(m_pixelsize * m_tileset->GetTileHeight());
}

void TilesetEditor::OnDraw(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	if (m_tileset == nullptr)
	{
		dc.Clear();
		return;
	}
	if (m_redraw_all)
	{
		RenderTilesetBitmap();
	}
	else if (!m_redraw_list.empty())
	{
		UpdateTilesetBitmap();
	}
	if (m_tiles_bmp == nullptr)
	{
		dc.Clear();
		return;
	}

	// Constrain everything - the background clear included - to the damaged area: pencil
	// strokes and cursor moves invalidate only a few pixels, and the repaint cost must be
	// proportional to that, not to the window size.
	wxRect damage = GetUpdateRegion().GetBox();
	damage.Offset(0, GetVisibleRowsBegin() * m_cellheight);
	dc.SetClippingRegion(damage);
	dc.Clear();

	int s = GetVisibleRowsBegin();
	int e = std::min(static_cast<int>(GetVisibleRowsEnd()) + 1, m_rows);
	s = std::max(s, damage.GetTop() / m_cellheight);
	e = std::min(e, damage.GetBottom() / m_cellheight + 1);
	const int c0 = std::max(0, damage.GetLeft() / m_cellwidth);
	const int c1 = std::min(m_columns, damage.GetRight() / m_cellwidth + 1);
	if ((e <= s) || (c1 <= c0))
	{
		return;
	}
	const int count = static_cast<int>(m_tileset->GetTileCount());

	// Backdrop for the transparent colour: full-width rows in one rectangle, plus the
	// partial last row if it is in view.
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
	const int last_full_row = count / m_columns;
	const int full_rows_end = std::min(e, last_full_row);
	if (full_rows_end > s)
	{
		dc.DrawRectangle(0, s * m_cellheight, m_columns * m_cellwidth, (full_rows_end - s) * m_cellheight);
	}
	const int remainder = count % m_columns;
	if ((remainder > 0) && (last_full_row >= s) && (last_full_row < e))
	{
		dc.DrawRectangle(0, last_full_row * m_cellheight, remainder * m_cellwidth, m_cellheight);
	}

	// One scaled blit of the damaged cells. All tile rendering happens at native resolution
	// in m_tiles_bmp, so the per-paint GDI work stays constant regardless of zoom.
	wxMemoryDC tiles(*m_tiles_bmp);
	dc.StretchBlit({ c0 * m_cellwidth, s * m_cellheight }, { (c1 - c0) * m_cellwidth, (e - s) * m_cellheight },
		&tiles, { c0 * m_tilewidth, s * m_tileheight }, { (c1 - c0) * m_tilewidth, (e - s) * m_tileheight },
		wxCOPY, true, { c0 * m_tilewidth, s * m_tileheight });
	tiles.SelectObject(wxNullBitmap);

	// Pixel grid first, cell borders after, so the tile boundaries stay visible on top of
	// the pixel grid lines they coincide with.
	DrawPixelGrid(dc, damage);
	DrawGrid(dc, damage);
	DrawGlyphLimitOverlay(dc, damage);
	DrawSelectionBorders(dc);
	DrawPixelCursor(dc);
	DrawShapePreview(dc);
	DrawPixelSelection(dc);
}

void TilesetEditor::RenderTilesetBitmap()
{
	m_buf.Resize(m_tilewidth * m_columns, m_tileheight * m_rows);
	int x = 0;
	int y = 0;
	for (std::size_t i = 0; i < m_tileset->GetTileCount(); ++i)
	{
		m_buf.InsertTile(x * m_tilewidth, y * m_tileheight, 0, i, *m_tileset, false);
		if (++x >= m_columns)
		{
			x = 0;
			y++;
		}
	}
	m_tiles_bmp = std::make_unique<wxBitmap>(m_buf.MakeImage({ m_selected_palette }, true));
	m_redraw_all = false;
	m_redraw_list.clear();
}

void TilesetEditor::UpdateTilesetBitmap()
{
	if (m_tiles_bmp == nullptr)
	{
		RenderTilesetBitmap();
		return;
	}
	wxAlphaPixelData data(*m_tiles_bmp);
	if (!data)
	{
		RenderTilesetBitmap();
		return;
	}
	// Re-render just the changed tiles at native resolution and write them straight into the
	// tileset bitmap's pixels. Converting the whole tileset to a wxImage on every paint - the
	// way a full redraw does - is what made pencil strokes crawl.
	ImageBufferWx tilebuf(m_tilewidth, m_tileheight);
	for (const int tile : m_redraw_list)
	{
		if ((tile < 0) || (tile >= static_cast<int>(m_tileset->GetTileCount())))
		{
			continue;
		}
		const int x0 = (tile % m_columns) * m_tilewidth;
		const int y0 = (tile / m_columns) * m_tileheight;
		// Keep the CPU-side buffer in sync so the next full re-render starts from current pixels.
		m_buf.InsertTile(x0, y0, 0, tile, *m_tileset, false);
		tilebuf.InsertTile(0, 0, 0, tile, *m_tileset, false);
		const wxImage img = tilebuf.MakeImage({ m_selected_palette }, true);
		const unsigned char* rgb = img.GetData();
		const unsigned char* alpha = img.GetAlpha();
		wxAlphaPixelData::Iterator p(data);
		for (int y = 0; y < m_tileheight; ++y)
		{
			p.MoveTo(data, x0, y0 + y);
			for (int x = 0; x < m_tilewidth; ++x)
			{
				const int i = x + y * m_tilewidth;
				const unsigned char a = alpha ? alpha[i] : 0xFF;
				// The bitmap stores premultiplied alpha, as AlphaBlend expects.
				p.Red() = (rgb[i * 3] * a) / 255;
				p.Green() = (rgb[i * 3 + 1] * a) / 255;
				p.Blue() = (rgb[i * 3 + 2] * a) / 255;
				p.Alpha() = a;
				++p;
			}
		}
	}
	m_redraw_list.clear();
}

void TilesetEditor::OnPaint(wxPaintEvent& /*evt*/)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void TilesetEditor::OnSize(wxSizeEvent& evt)
{
	this->GetClientSize(&m_ctrlwidth, &m_ctrlheight);
	if (UpdateRowCount())
	{
		ForceRedraw();
	}
	wxVScrolledWindow::HandleOnSize(evt);
	Refresh(false);
}

void TilesetEditor::OnKeyDown(wxKeyEvent& evt)
{
	if (m_enabledrawing && HandleDrawKey(evt))
	{
		return;
	}
	evt.Skip();
}

bool TilesetEditor::HandleDrawKey(wxKeyEvent& evt)
{
	const bool pixel_select = (m_tool == Tool::PixelSelect);
	const bool ctrl = evt.ControlDown();
	switch (evt.GetKeyCode())
	{
	// The main-row plus shares a key with equals, so the unshifted key cycles the primary
	// colour and the shifted one ('+' proper) the secondary; the numpad keys distinguish
	// by the Shift modifier alone.
	case '+':
	case '=':
	case WXK_NUMPAD_ADD:
		CycleColour(1, evt.ShiftDown());
		return true;
	case '-':
	case '_':
	case WXK_NUMPAD_SUBTRACT:
		CycleColour(-1, evt.ShiftDown());
		return true;
	case WXK_ESCAPE:
		return CancelActiveDrawOp();
	case WXK_DELETE:
		if (pixel_select && HasPixelSelection() && !m_sel_floating)
		{
			FillSelection(m_secondary_colour);
			return true;
		}
		return false;
	case 'c':
	case 'C':
		if (ctrl && pixel_select)
		{
			CopySelection();
			return true;
		}
		return false;
	case 'x':
	case 'X':
		if (ctrl && pixel_select)
		{
			CutSelection();
			return true;
		}
		return false;
	case 'v':
	case 'V':
		if (ctrl && pixel_select)
		{
			PastePixels();
			return true;
		}
		return false;
	case 'a':
	case 'A':
		if (ctrl && pixel_select)
		{
			SelectAllPixels();
			return true;
		}
		return false;
	case 'b':
	case 'B':
		if (ctrl && pixel_select)
		{
			SelectHoveredCell();
			return true;
		}
		return false;
	case 'h':
	case 'H':
		if (ctrl && pixel_select)
		{
			FlipSelection(true);
			return true;
		}
		return false;
	case 'e':
	case 'E':
		if (ctrl && pixel_select)
		{
			FlipSelection(false);
			return true;
		}
		return false;
	default:
		return false;
	}
}

void TilesetEditor::CycleColour(int delta, bool secondary)
{
	if (m_tileset == nullptr)
	{
		return;
	}
	const int count = 1 << m_tileset->GetTileBitDepth();
	uint8_t& colour = secondary ? m_secondary_colour : m_primary_colour;
	colour = static_cast<uint8_t>((colour + delta + count) % count);
	// The palette pane tracks palette colours, not pen indices, so translate.
	const int pal_colour = GetColour(colour);
	FireEvent(EVT_TILESET_COLOUR_PICK,
		std::to_string((secondary ? 0x100 : 0) | (pal_colour >= 0 ? pal_colour : 0)));
	if ((m_hoveredtile != -1) && (m_hoveredpixel.x >= 0))
	{
		// The pen cursor outline is drawn in the active colour.
		RefreshPixelRect(m_hoveredtile, m_hoveredpixel);
	}
}

void TilesetEditor::OnMouseDown(wxMouseEvent& evt)
{
	// Clicking the canvas takes the keyboard, so the colour-cycling keys work.
	SetFocus();
	// The tools never move the selection: selection is switched off in draw mode, and a
	// stray click must not re-target the tile the toolbar operations act on.
	if (m_enabledrawing)
	{
		m_drawing = true;
		m_secondary_active = false;
		m_last_drawn = wxPoint(-1, -1);
		StartDrawAction(evt.GetPosition());
		evt.Skip();
		return;
	}
	if (!m_enableselection) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	SelectTile(sel);
	FireEvent(EVT_TILESET_SELECT, std::to_string(m_selectedtile));
	evt.Skip();
}

void TilesetEditor::OnRightDown(wxMouseEvent& evt)
{
	if (m_enabledrawing)
	{
		m_drawing = true;
		m_secondary_active = true;
		m_last_drawn = wxPoint(-1, -1);
		StartDrawAction(evt.GetPosition());
	}
	evt.Skip();
}

void TilesetEditor::StartDrawAction(const wxPoint& mousepos)
{
	if ((m_tileset == nullptr) || (m_pixelsize <= 0))
	{
		return;
	}
	const int gx = mousepos.x / m_pixelsize;
	const int gy = GetVisibleRowsBegin() * m_tileheight + mousepos.y / m_pixelsize;
	switch (m_tool)
	{
	case Tool::Pencil:
		MouseDraw(mousepos);
		break;
	case Tool::Fill:
		FloodFillAt(gx, gy, m_secondary_active ? m_secondary_colour : m_primary_colour);
		break;
	case Tool::Picker:
		PickColourAt(gx, gy, m_secondary_active);
		break;
	case Tool::PixelSelect:
		if (!m_secondary_active)
		{
			BeginSelectionAction(gx, gy);
		}
		break;
	default:
		// Shape tools: anchor here, preview while dragging, commit on release.
		m_shape_active = true;
		m_shape_secondary = m_secondary_active;
		m_shape_start = wxPoint(gx, gy);
		m_shape_end = m_shape_start;
		RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
		break;
	}
}

void TilesetEditor::OnMouseUp(wxMouseEvent& evt)
{
	if (evt.LeftUp() && (m_sel_drag != SelDrag::None))
	{
		if (evt.RightIsDown())
		{
			CancelSelectionDrag();
		}
		else
		{
			FinishSelectionDrag();
		}
		m_drawing = false;
		evt.Skip();
		return;
	}
	if (evt.LeftUp())
	{
		if (evt.RightIsDown())
		{
			// Releasing the left button with the right still held cancels the operation
			// in progress, matching classic paint programs.
			CancelShape();
			CancelStroke();
			m_drawing = false;
			m_secondary_active = false;
			m_last_drawn = wxPoint(-1, -1);
			evt.Skip();
			return;
		}
		m_drawing = false;
		m_secondary_active = false;
	}
	if (evt.RightUp())
	{
		m_secondary_active = false;
		if (!evt.LeftIsDown())
		{
			m_drawing = false;
		}
	}
	if (!m_drawing)
	{
		m_last_drawn = wxPoint(-1, -1);
		if (m_shape_active)
		{
			CommitShape();
		}
		if (m_stroke_dirty)
		{
			// One change event per stroke, now that it is finished.
			m_stroke_dirty = false;
			FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(m_selectedtile));
		}
	}
	evt.Skip();
}

void TilesetEditor::OnDoubleClick(wxMouseEvent& evt)
{
	// Rapid clicks arrive as down/up/dclick/up; treating the dclick as another mouse-down
	// stops every second pencil click being dropped.
	if (m_enabledrawing)
	{
		OnMouseDown(evt);
		return;
	}
	int sel = ConvertXYToTile(evt.GetPosition());
	if (sel != -1)
	{
		EditTile(sel);
	}
	evt.Skip();
}

void TilesetEditor::OnMouseMove(wxMouseEvent& evt)
{
	if (!m_enablehover && !m_enabledrawing) return;
	MouseDraw(evt.GetPosition());
	evt.Skip();
}

void TilesetEditor::OnMouseLeave(wxMouseEvent& evt)
{
	if (m_sel_drag != SelDrag::None)
	{
		// The mouse is captured; the selection drag continues outside the window.
		evt.Skip();
		return;
	}
	// The stroke pauses while the pointer is outside - no motion events arrive out there -
	// and OnMouseEnter decides whether it resumes from the real button state. Dropping the
	// anchor makes re-entry start a fresh segment rather than joining a line across the
	// excursion. An in-progress shape is cancelled outright: its anchor would be stale by
	// the time the pointer returns.
	CancelShape();
	m_hoveredpixel = wxPoint(-1, -1);
	m_last_drawn = wxPoint(-1, -1);
	if (m_stroke_dirty)
	{
		m_stroke_dirty = false;
		FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(m_selectedtile));
	}
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		const int old = m_hoveredtile;
		m_hoveredtile = -1;
		FireEvent(EVT_TILESET_HOVER, std::to_string(m_hoveredtile));
		RefreshTileRect(old);
	}
	evt.Skip();
}

void TilesetEditor::OnMouseEnter(wxMouseEvent& evt)
{
	// Resume a stroke that left the canvas with the button still held, or end it if the
	// button was released while outside - that release never reaches this window.
	if (m_drawing)
	{
		if (evt.LeftIsDown())
		{
			m_secondary_active = false;
		}
		else if (evt.RightIsDown())
		{
			m_secondary_active = true;
		}
		else
		{
			m_drawing = false;
			m_secondary_active = false;
		}
	}
	m_last_drawn = wxPoint(-1, -1);
	evt.Skip();
}

void TilesetEditor::OnTilesetFocus(wxFocusEvent& evt)
{
	FireEvent(EVT_TILESET_ACTIVATE, "");
	evt.Skip();
}

int TilesetEditor::ConvertXYToTile(const wxPoint& point)
{
	if (m_tileset == nullptr)
	{
		return -1;
	}
	int s = GetVisibleRowsBegin();
	int x = point.x / (m_pixelsize * m_tileset->GetTileWidth());
	int y = s + point.y / (m_pixelsize * m_tileset->GetTileHeight());
	int sel = x + y * m_columns;
	if ((sel >= static_cast<int>(m_tileset->GetTileCount())) || (x < 0) || (y < 0) || (x >= m_columns))
	{
		sel = -1;
	}
	return sel;
}

bool TilesetEditor::ConvertXYToTilePixel(const wxPoint& point, int& tile, wxPoint& pixel) const
{
	tile = -1;
	pixel = wxPoint(-1, -1);
	if ((m_tileset == nullptr) || (m_cellwidth <= 0) || (m_cellheight <= 0))
	{
		return false;
	}
	int s = GetVisibleRowsBegin();
	int x = point.x / m_cellwidth;
	int y = s + point.y / m_cellheight;
	int sel = x + y * m_columns;
	if ((sel >= static_cast<int>(m_tileset->GetTileCount())) || (x < 0) || (y < 0) || (x >= m_columns))
	{
		return false;
	}
	tile = sel;
	pixel.x = (point.x % m_cellwidth) / m_pixelsize;
	pixel.y = (point.y % m_cellheight) / m_pixelsize;
	return true;
}

void TilesetEditor::MouseDraw(const wxPoint& mousepos)
{
	int tile = -1;
	wxPoint pixel(-1, -1);
	ConvertXYToTilePixel(mousepos, tile, pixel);
	const int old_tile = m_hoveredtile;
	const wxPoint old_pixel = m_hoveredpixel;
	m_hoveredpixel = pixel;
	// Repaint only what changed - a full-window Refresh on every mouse event makes the
	// pencil crawl on large tilesets.
	// Hover events are safe to fire per sample: the frame batches status bar rebuilds
	// behind a short timer, so the cost here is just a posted flag-set.
	if (m_enablehover && (tile != old_tile))
	{
		m_hoveredtile = tile;
		FireEvent(EVT_TILESET_HOVER, std::to_string(m_hoveredtile));
		RefreshTileRect(old_tile);
		RefreshTileRect(tile);
	}
	if (m_enabledrawing && (pixel != old_pixel))
	{
		if (tile == old_tile)
		{
			// The status bar shows the pixel coordinates; the tile-change branch above
			// already fired for cross-tile moves.
			FireEvent(EVT_TILESET_HOVER, std::to_string(m_hoveredtile));
		}
		RefreshPixelRect(old_tile, old_pixel);
		RefreshPixelRect(tile, pixel);
	}
	if (m_drawing && m_enabledrawing && (m_pixelsize > 0) && (m_tool == Tool::Picker))
	{
		// Dragging with the picker keeps sampling, like holding an eyedropper.
		PickColourAt(mousepos.x / m_pixelsize,
			GetVisibleRowsBegin() * m_tileheight + mousepos.y / m_pixelsize, m_secondary_active);
	}
	if (m_enabledrawing && (m_pixelsize > 0) && (m_tool == Tool::PixelSelect) &&
	    (m_sel_drag != SelDrag::None))
	{
		UpdateSelectionDrag(mousepos.x / m_pixelsize,
			GetVisibleRowsBegin() * m_tileheight + mousepos.y / m_pixelsize);
	}
	if (m_drawing && m_enabledrawing && (m_pixelsize > 0) && m_shape_active)
	{
		// Shape drag in progress: track the end point and repaint old and new extents.
		const int sgx = mousepos.x / m_pixelsize;
		const int sgy = GetVisibleRowsBegin() * m_tileheight + mousepos.y / m_pixelsize;
		if (m_shape_end != wxPoint(sgx, sgy))
		{
			const wxPoint old_end = m_shape_end;
			m_shape_end = wxPoint(sgx, sgy);
			RefreshRect(GlobalPixelBoxToClient(m_shape_start, old_end));
			RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
		}
	}
	if (m_drawing && m_enabledrawing && (m_pixelsize > 0) && (m_tool == Tool::Pencil))
	{
		if (!m_stroke_dirty)
		{
			// Provisional snapshot: pushed to the undo stack only once this stroke actually
			// changes a pixel.
			m_stroke_snapshot = m_tileset->GetBits(false);
		}
		const uint8_t colour = m_secondary_active ? m_secondary_colour : m_primary_colour;
		const int gx = mousepos.x / m_pixelsize;
		const int gy = GetVisibleRowsBegin() * m_tileheight + mousepos.y / m_pixelsize;
		bool changed = false;
		wxRect damage;
		if (m_last_drawn.x < 0)
		{
			changed = PaintGlobalPixel(gx, gy, colour, damage);
		}
		else
		{
			// Joined to the previous sample so fast strokes don't leave gaps.
			PlotShapeLine(m_last_drawn, wxPoint(gx, gy), [&](int x, int y)
				{
					changed |= PaintGlobalPixel(x, y, colour, damage);
				});
		}
		m_last_drawn = wxPoint(gx, gy);
		if (changed)
		{
			if (!m_stroke_dirty)
			{
				PushUndo(std::move(m_stroke_snapshot));
			}
			m_stroke_dirty = true;
			damage.Inflate(1, 1);
			RefreshRect(damage);
			// No Update() here: WM_PAINT is low priority, so leaving the repaint pending lets
			// Windows batch several mouse samples into one paint instead of forcing a full
			// synchronous cycle per event. The Bresenham join above keeps the line unbroken
			// regardless of how far apart the samples land.
		}
	}
}

bool TilesetEditor::PaintGlobalPixel(int gx, int gy, uint8_t colour, wxRect& damage)
{
	if ((gx < 0) || (gy < 0) || (gx >= m_columns * m_tilewidth))
	{
		return false;
	}
	const int tile = (gx / m_tilewidth) + (gy / m_tileheight) * m_columns;
	if (tile >= static_cast<int>(m_tileset->GetTileCount()))
	{
		return false;
	}
	const int px = gx % m_tilewidth;
	const int py = gy % m_tileheight;
	const int limit = m_draw_width_limiter ? m_draw_width_limiter(tile) : 0;
	if ((limit > 0) && (px >= limit))
	{
		return false;
	}
	auto& pixels = m_tileset->GetTilePixels(tile);
	const std::size_t idx = px + py * m_tilewidth;
	if ((idx >= pixels.size()) || (pixels[idx] == colour))
	{
		return false;
	}
	pixels[idx] = colour;
	m_redraw_list.insert(tile);
	const int s = GetVisibleRowsBegin();
	damage.Union(wxRect(gx * m_pixelsize, (gy - s * m_tileheight) * m_pixelsize,
	                    m_pixelsize + 1, m_pixelsize + 1));
	return true;
}

wxRect TilesetEditor::GlobalPixelBoxToClient(const wxPoint& a, const wxPoint& b) const
{
	const int s = GetVisibleRowsBegin() * m_tileheight;
	const int x0 = std::min(a.x, b.x);
	const int x1 = std::max(a.x, b.x);
	const int y0 = std::min(a.y, b.y);
	const int y1 = std::max(a.y, b.y);
	wxRect rect(x0 * m_pixelsize, (y0 - s) * m_pixelsize,
	            (x1 - x0 + 1) * m_pixelsize + 1, (y1 - y0 + 1) * m_pixelsize + 1);
	rect.Inflate(2, 2);
	return rect;
}

std::vector<wxPoint> TilesetEditor::MakeShapePoints(Tool tool, const wxPoint& a, const wxPoint& b) const
{
	// Geometry shared with the sprite editor - see ImageBufferWx.
	switch (tool)
	{
	case Tool::Line:             return MakeShapeToolPoints(ShapeTool::Line, a, b);
	case Tool::RectangleOutline: return MakeShapeToolPoints(ShapeTool::RectangleOutline, a, b);
	case Tool::RectangleFilled:  return MakeShapeToolPoints(ShapeTool::RectangleFilled, a, b);
	case Tool::CircleOutline:    return MakeShapeToolPoints(ShapeTool::CircleOutline, a, b);
	case Tool::CircleFilled:     return MakeShapeToolPoints(ShapeTool::CircleFilled, a, b);
	default:                     return {};
	}
}

void TilesetEditor::CommitShape()
{
	if (!m_shape_active)
	{
		return;
	}
	m_shape_active = false;
	// Erase the preview regardless of whether the commit changes anything.
	RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
	if (m_tileset == nullptr)
	{
		return;
	}
	auto snapshot = m_tileset->GetBits(false);
	const uint8_t colour = m_shape_secondary ? m_secondary_colour : m_primary_colour;
	bool changed = false;
	wxRect damage;
	for (const auto& p : MakeShapePoints(m_tool, m_shape_start, m_shape_end))
	{
		changed |= PaintGlobalPixel(p.x, p.y, colour, damage);
	}
	if (changed)
	{
		PushUndo(std::move(snapshot));
		FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(m_selectedtile));
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
}

void TilesetEditor::CancelShape()
{
	if (m_shape_active)
	{
		m_shape_active = false;
		RefreshRect(GlobalPixelBoxToClient(m_shape_start, m_shape_end));
	}
}

void TilesetEditor::FloodFillAt(int gx, int gy, uint8_t colour)
{
	// Fills within the clicked tile only: adjacent tiles in the grid are unrelated pieces of
	// artwork, so bleeding across cell boundaries is never what is wanted.
	if ((m_tileset == nullptr) || (gx < 0) || (gy < 0) || (gx >= m_columns * m_tilewidth))
	{
		return;
	}
	const int tile = (gx / m_tilewidth) + (gy / m_tileheight) * m_columns;
	if (tile >= static_cast<int>(m_tileset->GetTileCount()))
	{
		return;
	}
	const int px = gx % m_tilewidth;
	const int py = gy % m_tileheight;
	const int limit = m_draw_width_limiter ? m_draw_width_limiter(tile) : 0;
	const int width = ((limit > 0) && (limit < m_tilewidth)) ? limit : m_tilewidth;
	if (px >= width)
	{
		return;
	}
	auto& pixels = m_tileset->GetTilePixels(tile);
	const std::size_t start = px + py * m_tilewidth;
	if (start >= pixels.size())
	{
		return;
	}
	const uint8_t target = pixels[start];
	if (target == colour)
	{
		return;
	}
	auto snapshot = m_tileset->GetBits(false);
	std::vector<wxPoint> stack{ wxPoint(px, py) };
	while (!stack.empty())
	{
		const wxPoint p = stack.back();
		stack.pop_back();
		if ((p.x < 0) || (p.x >= width) || (p.y < 0) || (p.y >= m_tileheight))
		{
			continue;
		}
		uint8_t& value = pixels[p.x + p.y * m_tilewidth];
		if (value != target)
		{
			continue;
		}
		value = colour;
		stack.emplace_back(p.x + 1, p.y);
		stack.emplace_back(p.x - 1, p.y);
		stack.emplace_back(p.x, p.y + 1);
		stack.emplace_back(p.x, p.y - 1);
	}
	PushUndo(std::move(snapshot));
	m_redraw_list.insert(tile);
	RefreshTileRect(tile);
	FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(tile));
}

void TilesetEditor::PickColourAt(int gx, int gy, bool secondary)
{
	const int c = GetColourAtGlobalPixel(gx, gy);
	if (c < 0)
	{
		return;
	}
	uint8_t& target = secondary ? m_secondary_colour : m_primary_colour;
	if (target == c)
	{
		return;
	}
	target = static_cast<uint8_t>(c);
	const int pal_colour = GetColour(c);
	FireEvent(EVT_TILESET_COLOUR_PICK,
		std::to_string((secondary ? 0x100 : 0) | (pal_colour >= 0 ? pal_colour : 0)));
}

void TilesetEditor::CancelStroke()
{
	if (!m_stroke_dirty)
	{
		return;
	}
	m_stroke_dirty = false;
	if (!m_undo_stack.empty())
	{
		// The stroke pushed its pre-state when its first pixel landed; pop that back
		// without disturbing the redo stack.
		auto state = std::move(m_undo_stack.back());
		m_undo_stack.pop_back();
		RestoreHistoryState(std::move(state));
	}
}

bool TilesetEditor::CancelActiveDrawOp()
{
	if (m_shape_active)
	{
		CancelShape();
		m_drawing = false;
		return true;
	}
	if (m_stroke_dirty)
	{
		CancelStroke();
		m_drawing = false;
		return true;
	}
	if (m_sel_drag != SelDrag::None)
	{
		CancelSelectionDrag();
		return true;
	}
	if (m_sel_floating)
	{
		// Esc cancels a pending paste outright rather than confirming it.
		DiscardFloating();
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshSelectionRect(old);
		return true;
	}
	if (HasPixelSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshSelectionRect(old);
		return true;
	}
	return false;
}

bool TilesetEditor::HasPixelSelection() const
{
	return (m_sel_rect.width > 0) && (m_sel_rect.height > 0);
}

int TilesetEditor::GetColourAtGlobalPixel(int gx, int gy) const
{
	if ((m_tileset == nullptr) || (gx < 0) || (gy < 0) || (gx >= m_columns * m_tilewidth))
	{
		return -1;
	}
	const int tile = (gx / m_tilewidth) + (gy / m_tileheight) * m_columns;
	if (tile >= static_cast<int>(m_tileset->GetTileCount()))
	{
		return -1;
	}
	const int px = gx % m_tilewidth;
	const int limit = m_draw_width_limiter ? m_draw_width_limiter(tile) : 0;
	if ((limit > 0) && (px >= limit))
	{
		// Pixels beyond a glyph's width are undrawable, so they never travel with a
		// selection either.
		return -1;
	}
	const auto& pixels = m_tileset->GetTilePixels(tile);
	const std::size_t idx = px + (gy % m_tileheight) * m_tilewidth;
	return (idx < pixels.size()) ? pixels[idx] : -1;
}

void TilesetEditor::BeginSelectionAction(int gx, int gy)
{
	const int cw = m_columns * m_tilewidth;
	const int ch = m_rows * m_tileheight;
	if (HasPixelSelection() && m_sel_rect.Contains(wxPoint(gx, gy)))
	{
		m_sel_anchor = wxPoint(gx - m_sel_rect.x, gy - m_sel_rect.y);
		m_sel_op_changed = false;
		if (m_sel_floating)
		{
			// Dragging a pending paste just moves the float; it stays unconfirmed.
			m_sel_drag = SelDrag::Move;
		}
		else
		{
			m_sel_snapshot = m_tileset->GetBits(false);
			m_sel_snapshot_valid = true;
			if (wxGetKeyState(WXK_CONTROL))
			{
				m_sel_drag = SelDrag::Stamp;
			}
			else if (wxGetKeyState(WXK_SHIFT))
			{
				m_sel_drag = SelDrag::Duplicate;
			}
			else
			{
				m_sel_drag = SelDrag::Move;
			}
			LiftSelection(m_sel_drag == SelDrag::Move);
		}
		CaptureMouse();
	}
	else
	{
		// Clicking outside confirms a pending paste, clears the selection and starts a
		// fresh marquee from here.
		ClearPixelSelection(true);
		if ((gx >= 0) && (gy >= 0) && (gx < cw) && (gy < ch))
		{
			m_sel_drag = SelDrag::Marquee;
			m_sel_anchor = wxPoint(gx, gy);
			m_sel_rect = wxRect(gx, gy, 1, 1);
			RefreshSelectionRect(m_sel_rect);
			CaptureMouse();
		}
	}
}

void TilesetEditor::UpdateSelectionDrag(int gx, int gy)
{
	const int cw = m_columns * m_tilewidth;
	const int ch = m_rows * m_tileheight;
	switch (m_sel_drag)
	{
	case SelDrag::Marquee:
	{
		const int px = std::clamp(gx, 0, cw - 1);
		const int py = std::clamp(gy, 0, ch - 1);
		const wxRect next(wxPoint(std::min(m_sel_anchor.x, px), std::min(m_sel_anchor.y, py)),
		                  wxSize(std::abs(px - m_sel_anchor.x) + 1, std::abs(py - m_sel_anchor.y) + 1));
		if (next != m_sel_rect)
		{
			RefreshSelectionRect(m_sel_rect);
			m_sel_rect = next;
			RefreshSelectionRect(m_sel_rect);
		}
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
	{
		wxPoint tl(gx - m_sel_anchor.x, gy - m_sel_anchor.y);
		tl.x = std::clamp(tl.x, 0, cw - m_sel_rect.width);
		tl.y = std::clamp(tl.y, 0, ch - m_sel_rect.height);
		if (tl != m_sel_rect.GetTopLeft())
		{
			RefreshSelectionRect(m_sel_rect);
			m_sel_rect.x = tl.x;
			m_sel_rect.y = tl.y;
			if (m_sel_drag == SelDrag::Stamp)
			{
				// Continuous duplication: every step leaves a copy on the canvas.
				m_sel_op_changed |= StampFloating();
			}
			RefreshSelectionRect(m_sel_rect);
		}
		break;
	}
	default:
		break;
	}
}

void TilesetEditor::FinishSelectionDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if ((m_sel_drag == SelDrag::Move) || (m_sel_drag == SelDrag::Duplicate) ||
	    (m_sel_drag == SelDrag::Stamp))
	{
		if (!m_sel_from_paste)
		{
			if (m_sel_drag != SelDrag::Stamp)
			{
				m_sel_op_changed |= StampFloating();
			}
			m_sel_floating = false;
			m_float_bmp.reset();
			// A drag that ends where it started leaves the tileset untouched (erase and
			// re-stamp cancel out); comparing against the snapshot avoids a junk undo
			// entry for that case.
			bool push = false;
			if (m_sel_snapshot_valid && m_sel_op_changed)
			{
				push = (m_tileset->GetBits(false) != m_sel_snapshot);
			}
			if (push)
			{
				PushUndo(std::move(m_sel_snapshot));
				FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(m_selectedtile));
			}
			RefreshSelectionRect(m_sel_rect);
		}
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
}

void TilesetEditor::CancelSelectionDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	switch (m_sel_drag)
	{
	case SelDrag::Marquee:
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshSelectionRect(old);
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
		if (m_sel_from_paste)
		{
			// Cancelling mid-drag drops the pending paste entirely.
			DiscardFloating();
			const wxRect old = m_sel_rect;
			m_sel_rect = wxRect();
			RefreshSelectionRect(old);
		}
		else if (m_sel_snapshot_valid)
		{
			// Puts the tileset back exactly as it was before the lift.
			m_sel_floating = false;
			m_float_bmp.reset();
			RestoreHistoryState(std::move(m_sel_snapshot));
		}
		break;
	default:
		break;
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
}

void TilesetEditor::ClearPixelSelection(bool confirm_floating)
{
	if (m_sel_floating)
	{
		if (confirm_floating)
		{
			ConfirmFloating();
		}
		else
		{
			DiscardFloating();
		}
	}
	if (HasPixelSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshSelectionRect(old);
	}
}

void TilesetEditor::ConfirmFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	auto snapshot = m_tileset->GetBits(false);
	const bool changed = StampFloating();
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	if (changed)
	{
		PushUndo(std::move(snapshot));
		FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(m_selectedtile));
	}
	RefreshSelectionRect(m_sel_rect);
}

void TilesetEditor::DiscardFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	m_float_data.clear();
	RefreshSelectionRect(m_sel_rect);
}

void TilesetEditor::LiftSelection(bool erase_source)
{
	m_float_data = ReadRect(m_sel_rect);
	if (erase_source)
	{
		bool changed = false;
		wxRect damage;
		for (int y = 0; y < m_sel_rect.height; ++y)
		{
			for (int x = 0; x < m_sel_rect.width; ++x)
			{
				changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y,
				                            m_secondary_colour, damage);
			}
		}
		if (changed)
		{
			damage.Inflate(1, 1);
			RefreshRect(damage);
			m_sel_op_changed = true;
		}
	}
	m_sel_floating = true;
	m_sel_from_paste = false;
	RenderFloatBitmap();
	RefreshSelectionRect(m_sel_rect);
}

bool TilesetEditor::StampFloating()
{
	if (m_float_data.size() !=
	    static_cast<std::size_t>(m_sel_rect.width) * static_cast<std::size_t>(m_sel_rect.height))
	{
		return false;
	}
	bool changed = false;
	wxRect damage;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			const uint8_t v = m_float_data[x + y * m_sel_rect.width];
			if (v == SEL_TRANSPARENT)
			{
				continue;
			}
			changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y, v, damage);
		}
	}
	if (changed)
	{
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
	return changed;
}

void TilesetEditor::RenderFloatBitmap()
{
	wxImage img(m_sel_rect.width, m_sel_rect.height);
	img.SetAlpha();
	unsigned char* rgb = img.GetData();
	unsigned char* alpha = img.GetAlpha();
	for (std::size_t i = 0; i < m_float_data.size(); ++i)
	{
		const uint8_t v = m_float_data[i];
		const int pal_colour = (v == SEL_TRANSPARENT) ? -1 : GetColour(v);
		if ((pal_colour < 0) || (m_selected_palette == nullptr))
		{
			alpha[i] = 0;
			continue;
		}
		const uint32_t c = m_selected_palette->getBGRA(pal_colour);
		rgb[i * 3] = c & 0xFF;
		rgb[i * 3 + 1] = (c >> 8) & 0xFF;
		rgb[i * 3 + 2] = (c >> 16) & 0xFF;
		alpha[i] = c >> 24;
	}
	m_float_bmp = std::make_unique<wxBitmap>(img, 32);
}

void TilesetEditor::FillSelection(uint8_t colour)
{
	if (!HasPixelSelection() || m_sel_floating)
	{
		return;
	}
	auto snapshot = m_tileset->GetBits(false);
	bool changed = false;
	wxRect damage;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y, colour, damage);
		}
	}
	if (changed)
	{
		PushUndo(std::move(snapshot));
		FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(m_selectedtile));
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
}

void TilesetEditor::FlipSelection(bool horizontal)
{
	if (!HasPixelSelection())
	{
		return;
	}
	const int w = m_sel_rect.width;
	const int h = m_sel_rect.height;
	const auto flip = [&](const std::vector<uint8_t>& src)
	{
		std::vector<uint8_t> out(src.size());
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				out[x + y * w] = horizontal ? src[(w - 1 - x) + y * w]
				                            : src[x + (h - 1 - y) * w];
			}
		}
		return out;
	};
	if (m_sel_floating)
	{
		m_float_data = flip(m_float_data);
		RenderFloatBitmap();
		RefreshSelectionRect(m_sel_rect);
		return;
	}
	auto snapshot = m_tileset->GetBits(false);
	const auto flipped = flip(ReadRect(m_sel_rect));
	bool changed = false;
	wxRect damage;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const uint8_t v = flipped[x + y * w];
			if (v == SEL_TRANSPARENT)
			{
				continue;
			}
			changed |= PaintGlobalPixel(m_sel_rect.x + x, m_sel_rect.y + y, v, damage);
		}
	}
	if (changed)
	{
		PushUndo(std::move(snapshot));
		FireEvent(EVT_TILESET_TILE_CHANGE, std::to_string(m_selectedtile));
		damage.Inflate(1, 1);
		RefreshRect(damage);
	}
}

void TilesetEditor::CopySelection()
{
	if (!HasPixelSelection())
	{
		return;
	}
	m_pixel_clipboard.rect = m_sel_rect;
	m_pixel_clipboard.data = m_sel_floating ? m_float_data : ReadRect(m_sel_rect);
}

void TilesetEditor::CutSelection()
{
	if (!HasPixelSelection())
	{
		return;
	}
	CopySelection();
	if (m_sel_floating)
	{
		// Cutting a pending paste just removes the float; the tileset never had it.
		DiscardFloating();
	}
	else
	{
		FillSelection(m_secondary_colour);
	}
}

void TilesetEditor::PastePixels()
{
	if (m_pixel_clipboard.data.empty())
	{
		return;
	}
	ClearPixelSelection(true);
	wxRect r = m_pixel_clipboard.rect;
	r.x = std::clamp(r.x, 0, std::max(0, m_columns * m_tilewidth - r.width));
	r.y = std::clamp(r.y, 0, std::max(0, m_rows * m_tileheight - r.height));
	m_sel_rect = r;
	m_float_data = m_pixel_clipboard.data;
	m_sel_floating = true;
	m_sel_from_paste = true;
	RenderFloatBitmap();
	RefreshSelectionRect(m_sel_rect);
}

void TilesetEditor::SelectAllPixels()
{
	if (m_tileset == nullptr)
	{
		return;
	}
	ClearPixelSelection(true);
	m_sel_rect = wxRect(0, 0, m_columns * m_tilewidth, m_rows * m_tileheight);
	RefreshSelectionRect(m_sel_rect);
}

void TilesetEditor::SelectHoveredCell()
{
	const int tile = (m_hoveredtile != -1) ? m_hoveredtile : m_selectedtile;
	if (tile < 0)
	{
		return;
	}
	ClearPixelSelection(true);
	m_sel_rect = wxRect((tile % m_columns) * m_tilewidth, (tile / m_columns) * m_tileheight,
	                    m_tilewidth, m_tileheight);
	RefreshSelectionRect(m_sel_rect);
}

std::vector<uint8_t> TilesetEditor::ReadRect(const wxRect& rect) const
{
	std::vector<uint8_t> out(static_cast<std::size_t>(rect.width) * static_cast<std::size_t>(rect.height),
	                         SEL_TRANSPARENT);
	for (int y = 0; y < rect.height; ++y)
	{
		for (int x = 0; x < rect.width; ++x)
		{
			const int c = GetColourAtGlobalPixel(rect.x + x, rect.y + y);
			if (c >= 0)
			{
				out[x + y * rect.width] = static_cast<uint8_t>(c);
			}
		}
	}
	return out;
}

void TilesetEditor::RefreshSelectionRect(const wxRect& rect)
{
	if ((rect.width <= 0) || (rect.height <= 0))
	{
		return;
	}
	RefreshRect(GlobalPixelBoxToClient(rect.GetTopLeft(), rect.GetBottomRight()));
}

void TilesetEditor::DrawPixelSelection(wxDC& dc)
{
	if (!HasPixelSelection())
	{
		return;
	}
	if (m_sel_floating && (m_float_bmp != nullptr))
	{
		wxMemoryDC mem(*m_float_bmp);
		dc.StretchBlit(m_sel_rect.x * m_pixelsize, m_sel_rect.y * m_pixelsize,
		               m_sel_rect.width * m_pixelsize, m_sel_rect.height * m_pixelsize,
		               &mem, 0, 0, m_sel_rect.width, m_sel_rect.height, wxCOPY, true);
		mem.SelectObject(wxNullBitmap);
	}
	// White underlay + black dashes stays visible over any artwork.
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*wxWHITE_PEN);
	dc.DrawRectangle(m_sel_rect.x * m_pixelsize, m_sel_rect.y * m_pixelsize,
	                 m_sel_rect.width * m_pixelsize + 1, m_sel_rect.height * m_pixelsize + 1);
	dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_SHORT_DASH));
	dc.DrawRectangle(m_sel_rect.x * m_pixelsize, m_sel_rect.y * m_pixelsize,
	                 m_sel_rect.width * m_pixelsize + 1, m_sel_rect.height * m_pixelsize + 1);
}

void TilesetEditor::ResetSelectionState()
{
	m_sel_rect = wxRect();
	m_sel_drag = SelDrag::None;
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
	m_float_bmp.reset();
	m_float_data.clear();
	// The pixel clipboard survives on purpose, so content can be pasted across tilesets.
}

void TilesetEditor::OnCaptureLost(wxMouseCaptureLostEvent& /*evt*/)
{
	// Capture already gone - just drop the drag state.
	m_sel_drag = SelDrag::None;
	m_sel_snapshot_valid = false;
	m_sel_op_changed = false;
}

void TilesetEditor::RefreshTileRect(int tile)
{
	if ((tile < 0) || (m_columns <= 0))
	{
		return;
	}
	const int s = GetVisibleRowsBegin();
	wxRect rect((tile % m_columns) * m_cellwidth, (tile / m_columns - s) * m_cellheight,
	            m_cellwidth + 1, m_cellheight + 1);
	// Cover the hover border pen.
	rect.Inflate(1, 1);
	RefreshRect(rect);
}

void TilesetEditor::RefreshPixelRect(int tile, const wxPoint& pixel)
{
	if ((tile < 0) || (pixel.x < 0) || (pixel.y < 0) || (m_columns <= 0))
	{
		return;
	}
	const int s = GetVisibleRowsBegin();
	wxRect rect((tile % m_columns) * m_cellwidth + pixel.x * m_pixelsize,
	            (tile / m_columns - s) * m_cellheight + pixel.y * m_pixelsize,
	            m_pixelsize + 1, m_pixelsize + 1);
	// Cover the pixel cursor pen, which straddles the pixel boundary.
	rect.Inflate(3, 3);
	RefreshRect(rect);
}

int TilesetEditor::ValidateColour(int colour) const
{
	if (m_tileset == nullptr)
	{
		return -1;
	}
	auto cmap = m_tileset->GetColourIndicies();
	if (cmap.empty())
	{
		if (colour < (1 << m_tileset->GetTileBitDepth()))
		{
			return colour;
		}
	}
	else
	{
		const int result = static_cast<int>(std::find(cmap.begin(), cmap.end(), colour) - cmap.begin());
		if (result < (1 << m_tileset->GetTileBitDepth()))
		{
			return result;
		}
	}
	return -1;
}

wxColour TilesetEditor::GetPaletteColour(int index) const
{
	if ((m_selected_palette == nullptr) || (index < 0))
	{
		return *wxBLACK;
	}
	const auto cmap = m_tileset->GetColourIndicies();
	const uint32_t colour = m_selected_palette->getBGRA(
		(index < static_cast<int>(cmap.size())) ? cmap[index] : index);
	return wxColour(colour & 0xFFFFFF);
}

bool TilesetEditor::UpdateRowCount()
{
	if (!m_tileset)
	{
		if (m_columns > 0 || m_rows > 0)
		{
			m_columns = 0;
			m_rows = 0;
			m_tilewidth = 0;
			m_tileheight = 0;
			SetRowCount(m_rows);
			return true;
		}
		return false;
	}
	m_tilewidth = m_tileset->GetTileWidth();
	m_cellwidth = m_pixelsize * m_tilewidth;
	m_tileheight = m_tileset->GetTileHeight();
	m_cellheight = m_pixelsize * m_tileheight;
	int columns = std::max<int>(1, m_ctrlwidth / m_cellwidth);
	int rows = std::max<int>(1, (m_tileset->GetTileCount() + columns - 1) / columns);
	if ((columns != m_columns) || (rows != m_rows))
	{
		m_columns = columns;
		m_rows = rows;
		SetRowCount(m_rows);
		return true;
	}
	return false;
}

void TilesetEditor::DrawGrid(wxDC& dest, const wxRect& damage)
{
	if (!m_enableborders && !m_enabletilenumbers)
	{
		return;
	}
	dest.SetPen(*m_border_pen);
	dest.SetBrush(*wxTRANSPARENT_BRUSH);
	dest.SetTextForeground(wxColour(255, 255, 255));
	dest.SetTextBackground(wxColour(150, 150, 150));
	dest.SetBackgroundMode(wxSOLID);

	if (m_pixelsize > 3)
	{
		m_border_pen->SetStyle(wxPENSTYLE_SOLID);
	}
	else
	{
		m_border_pen->SetStyle(wxPENSTYLE_TRANSPARENT);
	}

	int s = GetVisibleRowsBegin();
	int e = std::min(static_cast<int>(GetVisibleRowsEnd()) + 1, m_rows);
	s = std::max(s, damage.GetTop() / m_cellheight);
	e = std::min(e, damage.GetBottom() / m_cellheight + 1);
	const int c0 = std::max(0, damage.GetLeft() / m_cellwidth);
	const int c1 = std::min(m_columns, damage.GetRight() / m_cellwidth + 1);
	const int count = static_cast<int>(m_tileset->GetTileCount());
	for (int y = s; y < e; ++y)
	{
		for (int x = c0; x < c1; ++x)
		{
			const int i = x + y * m_columns;
			if (i >= count)
			{
				break;
			}
			if (m_enableborders)
			{
				dest.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth + 1, m_cellheight + 1 });
			}
			if (m_enabletilenumbers)
			{
				auto label = (wxString::Format("%03u", i));
				auto extent = dest.GetTextExtent(label);
				if ((extent.GetWidth() < m_cellwidth - 2) && (extent.GetHeight() < m_cellheight - 2))
				{
					dest.DrawText(label, { x * m_cellwidth + 2, y * m_cellheight + 2 });
				}
			}
		}
	}
}

void TilesetEditor::DrawPixelGrid(wxDC& dc, const wxRect& damage)
{
	// Only worth showing when zoomed in far enough for the pencil to be usable.
	if (!m_enabledrawing || !m_enablepixelgrid || (m_pixelsize < 4))
	{
		return;
	}
	// Solid pen on purpose: GDI rasterizes dotted lines on the CPU, and a window full of
	// them made every full repaint (e.g. switching tools) noticeably laggy.
	dc.SetPen(wxPen(wxColour(96, 96, 96)));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	const int s = GetVisibleRowsBegin();
	const int e = std::min(static_cast<int>(GetVisibleRowsEnd()) + 1, m_rows);
	// Only the grid lines crossing the damaged area within the given pixel-space region.
	const auto draw_region = [&](int px0, int px1, int py0, int py1)
	{
		px0 = std::max(px0, damage.GetLeft() / m_pixelsize);
		px1 = std::min(px1, damage.GetRight() / m_pixelsize + 1);
		py0 = std::max(py0, damage.GetTop() / m_pixelsize);
		py1 = std::min(py1, damage.GetBottom() / m_pixelsize + 1);
		if ((px1 < px0) || (py1 < py0))
		{
			return;
		}
		for (int y = py0; y <= py1; ++y)
		{
			dc.DrawLine(px0 * m_pixelsize, y * m_pixelsize, px1 * m_pixelsize, y * m_pixelsize);
		}
		for (int x = px0; x <= px1; ++x)
		{
			dc.DrawLine(x * m_pixelsize, py0 * m_pixelsize, x * m_pixelsize, py1 * m_pixelsize);
		}
	};
	// The full-width rows, then the partial last row - no grid over cells with no tile.
	const int count = static_cast<int>(m_tileset->GetTileCount());
	const int last_full_row = count / m_columns;
	const int remainder = count % m_columns;
	const int full_rows_end = std::min(e, last_full_row);
	if (full_rows_end > s)
	{
		draw_region(0, m_tilewidth * m_columns, s * m_tileheight, full_rows_end * m_tileheight);
	}
	if ((remainder > 0) && (last_full_row >= s) && (last_full_row < e))
	{
		draw_region(0, m_tilewidth * remainder, last_full_row * m_tileheight, (last_full_row + 1) * m_tileheight);
	}
}

void TilesetEditor::DrawGlyphLimitOverlay(wxDC& dc, const wxRect& damage)
{
	// Marks the columns past a glyph's width on variable-width font tilesets; the pencil
	// refuses them, so show why. The limiter returns 0 (no limit) for everything else.
	if (!m_draw_width_limiter || (m_tileset == nullptr))
	{
		return;
	}
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(wxColour(200, 64, 64), wxBRUSHSTYLE_CROSSDIAG_HATCH));
	dc.SetBackgroundMode(wxTRANSPARENT);
	int s = GetVisibleRowsBegin();
	int e = std::min(static_cast<int>(GetVisibleRowsEnd()) + 1, m_rows);
	s = std::max(s, damage.GetTop() / m_cellheight);
	e = std::min(e, damage.GetBottom() / m_cellheight + 1);
	const int c0 = std::max(0, damage.GetLeft() / m_cellwidth);
	const int c1 = std::min(m_columns, damage.GetRight() / m_cellwidth + 1);
	const int count = static_cast<int>(m_tileset->GetTileCount());
	for (int y = s; y < e; ++y)
	{
		for (int x = c0; x < c1; ++x)
		{
			const int i = x + y * m_columns;
			if (i >= count)
			{
				break;
			}
			const int limit = m_draw_width_limiter(i);
			if ((limit > 0) && (limit < m_tilewidth))
			{
				dc.DrawRectangle(x * m_cellwidth + limit * m_pixelsize, y * m_cellheight,
				                 (m_tilewidth - limit) * m_pixelsize + 1, m_cellheight + 1);
			}
		}
	}
}

void TilesetEditor::DrawPixelCursor(wxDC& dc)
{
	if (!m_enabledrawing || (m_hoveredtile == -1) ||
	    (m_hoveredpixel.x < 0) || (m_hoveredpixel.y < 0) ||
	    (m_tool == Tool::PixelSelect))
	{
		return;
	}
	const int cx = (m_hoveredtile % m_columns) * m_cellwidth + m_hoveredpixel.x * m_pixelsize;
	const int cy = (m_hoveredtile / m_columns) * m_cellheight + m_hoveredpixel.y * m_pixelsize;
	wxPen cursor(GetPaletteColour(m_secondary_active ? m_secondary_colour : m_primary_colour));
	cursor.SetWidth(std::min((m_pixelsize + 1) / 2, 3));
	dc.SetPen(cursor);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(cx, cy, m_pixelsize + 1, m_pixelsize + 1);
}

void TilesetEditor::DrawShapePreview(wxDC& dc)
{
	if (!m_shape_active)
	{
		return;
	}
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(GetPaletteColour(m_shape_secondary ? m_secondary_colour : m_primary_colour)));
	for (const auto& p : MakeShapePoints(m_tool, m_shape_start, m_shape_end))
	{
		dc.DrawRectangle(p.x * m_pixelsize, p.y * m_pixelsize, m_pixelsize, m_pixelsize);
	}
}

void TilesetEditor::DrawSelectionBorders(wxDC& dc)
{
	if (m_hoveredtile != -1)
	{
		auto x = m_hoveredtile % m_columns;
		auto y = m_hoveredtile / m_columns;
		dc.SetBrush(*m_highlighted_brush);
		dc.SetPen(*m_highlighted_border_pen);
		if (m_hoveredtile == m_selectedtile)
		{
			dc.DrawRectangle({ x * m_cellwidth + 1, y * m_cellheight + 1, m_cellwidth - 2, m_cellheight - 2});
		}
		else
		{
			dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight});
		}
	}
	if (m_selectedtile != -1)
	{
		auto x = m_selectedtile % m_columns;
		auto y = m_selectedtile / m_columns;
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*m_selected_border_pen);
		dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight});
	}
}

void TilesetEditor::InitialiseBrushesAndPens()
{
	m_alpha_brush = std::make_unique<wxBrush>(*wxBLACK);
	m_stipple = std::make_unique<wxBitmap>(6, 6);
	std::unique_ptr<wxMemoryDC> imagememDC(new wxMemoryDC());
	imagememDC->SelectObject(*m_stipple);
	imagememDC->SetBackground(*wxGREY_BRUSH);
	imagememDC->Clear();
	imagememDC->SetBrush(*wxLIGHT_GREY_BRUSH);
	imagememDC->SetPen(*wxTRANSPARENT_PEN);
	imagememDC->DrawRectangle(0, 0, 3, 3);
	imagememDC->DrawRectangle(3, 3, 5, 5);
	imagememDC->SelectObject(wxNullBitmap);
	m_alpha_brush->SetStyle(wxBRUSHSTYLE_STIPPLE_MASK);
	m_alpha_brush->SetStipple(*m_stipple);
	// Amber, so tile boundaries stand apart from the grey pixel grid when drawing.
	m_border_pen = std::make_unique<wxPen>(wxColour(200, 150, 60));
	m_selected_border_pen = std::make_unique<wxPen>(*wxRED_PEN);
	m_highlighted_border_pen = std::make_unique<wxPen>(*wxBLUE_PEN);
	m_highlighted_brush = std::make_unique<wxBrush>(*wxTRANSPARENT_BRUSH);
}

void TilesetEditor::ForceRedraw()
{
	m_redraw_all = true;
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

void TilesetEditor::SetPixelSize(int n)
{
	m_pixelsize = n;
	UpdateRowCount();
	ForceRedraw();
}

int TilesetEditor::GetPixelSize() const
{
	return m_pixelsize;
}

int TilesetEditor::GetTilemapSize() const
{
	return m_tileset->GetTileCount();
}

bool TilesetEditor::GetCompressed() const
{
	return m_tileset->GetCompressed();
}

void TilesetEditor::SetColourMap(const std::vector<uint8_t>& cmap)
{
	m_tileset->SetColourIndicies(cmap);
}

std::vector<uint8_t> TilesetEditor::GetColourMap() const
{
	return m_tileset->GetColourIndicies();
}

void TilesetEditor::SetActivePalette(const std::string& name)
{
	if (m_selected_palette_name != name)
	{
		m_selected_palette_name = name;
		m_selected_palette_entry = m_gd->GetPalette(m_selected_palette_name);
		m_selected_palette = m_selected_palette_entry->GetData();
		ForceRedraw();
	}
}

std::string TilesetEditor::GetActivePalette() const
{
	return m_selected_palette_name;
}

std::array<bool, 16> TilesetEditor::GetLockedColours() const
{
	return m_tileset->GetLockedColours();
}

bool TilesetEditor::GetTileNumbersEnabled() const
{
	return m_enabletilenumbers;
}

void TilesetEditor::SetTileNumbersEnabled(bool enabled)
{
	if (enabled != m_enabletilenumbers)
	{
		m_enabletilenumbers = enabled;
		Refresh();
	}
}

bool TilesetEditor::GetSelectionEnabled() const
{
	return m_enableselection;
}

void TilesetEditor::SetSelectionEnabled(bool enabled)
{
	if (m_enableselection != enabled)
	{
		if (enabled == false)
		{
			m_selectedtile = -1;
		}
		m_enableselection = enabled;
		Refresh(false);
	}
}

bool TilesetEditor::GetHoverEnabled() const
{
	return m_enablehover;
}

void TilesetEditor::SetHoverEnabled(bool enabled)
{
	if (m_enablehover != enabled)
	{
		if (enabled == false)
		{
			m_hoveredtile = -1;
		}
		m_enablehover = enabled;
		Refresh(false);
	}
}

bool TilesetEditor::GetAlphaEnabled() const
{
	return m_enablealpha;
}

void TilesetEditor::SetAlphaEnabled(bool enabled)
{
	if (m_enablealpha != enabled)
	{
		m_enablealpha = enabled;
		ForceRedraw();
	}
}

bool TilesetEditor::GetBordersEnabled() const
{
	return m_enableborders;
}

void TilesetEditor::SetBordersEnabled(bool enabled)
{
	if (m_enableborders != enabled)
	{
		m_enableborders = enabled;
		Refresh();
	}
}

bool TilesetEditor::GetDrawingEnabled() const
{
	return m_enabledrawing;
}

void TilesetEditor::SetDrawingEnabled(bool enabled)
{
	if (m_enabledrawing != enabled)
	{
		if (!enabled)
		{
			// Leaving draw mode confirms a pending paste and drops the selection.
			if (m_sel_drag != SelDrag::None)
			{
				CancelSelectionDrag();
			}
			ClearPixelSelection(true);
		}
		m_enabledrawing = enabled;
		m_drawing = false;
		m_secondary_active = false;
		m_last_drawn = wxPoint(-1, -1);
		m_shape_active = false;
		Refresh();
	}
}

TilesetEditor::Tool TilesetEditor::GetDrawTool() const
{
	return m_tool;
}

void TilesetEditor::SetDrawTool(Tool tool)
{
	if (m_tool != tool)
	{
		CancelShape();
		if (m_sel_drag != SelDrag::None)
		{
			CancelSelectionDrag();
		}
		// Switching tool confirms a pending paste and drops the selection.
		ClearPixelSelection(true);
		m_tool = tool;
	}
}

bool TilesetEditor::GetPixelGridEnabled() const
{
	return m_enablepixelgrid;
}

void TilesetEditor::SetPixelGridEnabled(bool enabled)
{
	if (m_enablepixelgrid != enabled)
	{
		m_enablepixelgrid = enabled;
		Refresh();
	}
}

void TilesetEditor::SetPrimaryColour(uint8_t colour)
{
	const int result = ValidateColour(colour);
	if (result >= 0)
	{
		m_primary_colour = static_cast<uint8_t>(result);
	}
}

uint8_t TilesetEditor::GetPrimaryColour() const
{
	return m_primary_colour;
}

void TilesetEditor::SetSecondaryColour(uint8_t colour)
{
	const int result = ValidateColour(colour);
	if (result >= 0)
	{
		m_secondary_colour = static_cast<uint8_t>(result);
	}
}

uint8_t TilesetEditor::GetSecondaryColour() const
{
	return m_secondary_colour;
}

void TilesetEditor::SetDrawWidthLimiter(std::function<int(int)> limiter)
{
	m_draw_width_limiter = std::move(limiter);
}

bool TilesetEditor::IsPixelHoverValid() const
{
	return (m_hoveredtile != -1) && (m_hoveredpixel.x >= 0) && (m_hoveredpixel.y >= 0);
}

wxPoint TilesetEditor::GetHoveredPixel() const
{
	return m_hoveredpixel;
}

int TilesetEditor::GetColourAtPixel(const Landstalker::Tile& tile, const wxPoint& point) const
{
	if ((m_tileset == nullptr) || (tile.GetIndex() >= m_tileset->GetTileCount()))
	{
		return -1;
	}
	const auto pixels = m_tileset->GetTile(tile);
	const std::size_t idx = point.x + point.y * m_tileset->GetTileWidth();
	if (idx >= pixels.size())
	{
		return -1;
	}
	return pixels[idx];
}

int TilesetEditor::GetColour(int index) const
{
	const auto cmap = m_tileset->GetColourIndicies();
	if (index >= static_cast<int>(cmap.size()))
	{
		if (index < (1 << m_tileset->GetTileBitDepth()))
		{
			return index;
		}
	}
	else if (index >= 0)
	{
		return cmap[index];
	}
	return -1;
}

bool TilesetEditor::IsSelectionValid() const
{
	return ((m_selectedtile != -1) && (m_selectedtile < static_cast<int>(m_tileset->GetTileCount())));
}

Landstalker::Tile TilesetEditor::GetSelectedTile() const
{
	return Landstalker::Tile(m_selectedtile);
}

bool TilesetEditor::IsHoverValid() const
{
	return m_hoveredtile != -1;
}

Landstalker::Tile TilesetEditor::GetHoveredTile() const
{
	return Landstalker::Tile(m_hoveredtile);
}

bool TilesetEditor::CanUndo() const
{
	return !m_undo_stack.empty();
}

bool TilesetEditor::CanRedo() const
{
	return !m_redo_stack.empty();
}

void TilesetEditor::Undo()
{
	if (!m_tileset || m_undo_stack.empty())
	{
		return;
	}
	m_redo_stack.push_back(m_tileset->GetBits(false));
	auto state = std::move(m_undo_stack.back());
	m_undo_stack.pop_back();
	RestoreHistoryState(std::move(state));
}

void TilesetEditor::Redo()
{
	if (!m_tileset || m_redo_stack.empty())
	{
		return;
	}
	m_undo_stack.push_back(m_tileset->GetBits(false));
	auto state = std::move(m_redo_stack.back());
	m_redo_stack.pop_back();
	RestoreHistoryState(std::move(state));
}

void TilesetEditor::PushUndo(std::vector<uint8_t>&& state)
{
	// A new edit invalidates anything that was undone.
	m_redo_stack.clear();
	m_undo_stack.push_back(std::move(state));
	// A few kilobytes per entry, so a deep history is affordable - but not unbounded.
	while (m_undo_stack.size() > 100)
	{
		m_undo_stack.pop_front();
	}
}

void TilesetEditor::RestoreHistoryState(std::vector<uint8_t>&& state)
{
	// Any selection or pending float refers to content that is about to change.
	ResetSelectionState();
	m_tileset->SetBits(state, false);
	// A restored state can have a different tile count, invalidating the selection and the
	// layout both.
	if (m_selectedtile >= static_cast<int>(m_tileset->GetTileCount()))
	{
		m_selectedtile = static_cast<int>(m_tileset->GetTileCount()) - 1;
	}
	UpdateRowCount();
	ForceRedraw();
	FireEvent(EVT_TILESET_CHANGE, "");
}

void TilesetEditor::ClearHistory()
{
	m_undo_stack.clear();
	m_redo_stack.clear();
	m_stroke_dirty = false;
	// Called when a tileset is (re)opened: any selection or float belongs to the old one.
	ResetSelectionState();
}

void TilesetEditor::SelectTile(int tile)
{
	if ((m_selectedtile != -1) && (tile != m_selectedtile))
	{
		if (m_pendingswap != -1)
		{
			PushUndo(m_tileset->GetBits(false));
			m_redraw_list.insert(tile);
			m_redraw_list.insert(m_selectedtile);
			m_tileset->SwapTile(m_pendingswap, tile);
			m_pendingswap = -1;
			Refresh();
		}
	}
	if (tile != m_selectedtile)
	{
		// Repaint just the two affected cells. A full refresh here sat directly in front of
		// the first pixel of every pencil stroke that starts on a new tile.
		const int old = m_selectedtile;
		m_selectedtile = tile;
		RefreshTileRect(old);
		RefreshTileRect(tile);
	}
}

void TilesetEditor::InsertTileBefore(const Landstalker::Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		PushUndo(m_tileset->GetBits(false));
		m_tileset->InsertTilesBefore(tile.GetIndex());
		for (int i = tile.GetIndex(); i < static_cast<int>(m_tileset->GetTileCount()); ++i)
		{
			m_redraw_list.insert(i);
		}
		SelectTile(m_selectedtile + 1);
		UpdateRowCount();
		Refresh();
		FireEvent(EVT_TILESET_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void TilesetEditor::InsertTileAfter(const Landstalker::Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		PushUndo(m_tileset->GetBits(false));
		m_tileset->InsertTilesBefore(tile.GetIndex() + 1);
		for (std::size_t i = tile.GetIndex(); i < m_tileset->GetTileCount(); ++i)
		{
			m_redraw_list.insert(i);
		}
		UpdateRowCount();
		Refresh();
		FireEvent(EVT_TILESET_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void TilesetEditor::InsertTilesAtEnd(int count)
{
	PushUndo(m_tileset->GetBits(false));
	m_tileset->InsertTilesBefore(m_tileset->GetTileCount(), count);
	UpdateRowCount();
	Refresh();
	FireEvent(EVT_TILESET_CHANGE, "");
}

void TilesetEditor::DeleteTileAt(const Landstalker::Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		PushUndo(m_tileset->GetBits(false));
		m_tileset->DeleteTile(tile.GetIndex());
		if (!IsSelectionValid())
		{
			SelectTile(m_tileset->GetTileCount() - 1);
		}
		ForceRedraw();
		FireEvent(EVT_TILESET_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void TilesetEditor::CutTile(const Landstalker::Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_clipboard = m_tileset->GetTile(tile);
		DeleteTileAt(tile);
	}
}

void TilesetEditor::CopyTile(const Landstalker::Tile& tile) const
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_clipboard = m_tileset->GetTile(tile);
	}
}

void TilesetEditor::PasteTile(const Landstalker::Tile& tile)
{
	if ((tile.GetIndex() <= m_tileset->GetTileCount()) && !IsClipboardEmpty())
	{
		PushUndo(m_tileset->GetBits(false));
		m_tileset->GetTilePixels(tile.GetTileValue()) = m_clipboard;
		m_redraw_list.insert(tile.GetTileValue());
		Refresh();
		FireEvent(EVT_TILESET_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void TilesetEditor::SwapTile(const Landstalker::Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_pendingswap = tile.GetIndex();
	}
}

void TilesetEditor::EditTile(const Landstalker::Tile& tile)
{
	if (tile.GetIndex() < m_tileset->GetTileCount())
	{
		FireEvent(EVT_TILESET_EDIT_REQUEST, std::to_string(tile.GetIndex()));
	}
}

bool TilesetEditor::IsClipboardEmpty() const
{
	return m_clipboard.empty();
}


void TilesetEditor::FireEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(&m_tileset);
	wxPostEvent(this->GetParent(), evt);
}
