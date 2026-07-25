#include <2d_maps/Map2DEditor.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <algorithm>
#include <main/ImageBufferWx.h>

wxBEGIN_EVENT_TABLE(Map2DEditor, wxHVScrolledWindow)
EVT_PAINT(Map2DEditor::OnPaint)
EVT_SIZE(Map2DEditor::OnSize)
EVT_MOTION(Map2DEditor::OnMouseMove)
EVT_LEAVE_WINDOW(Map2DEditor::OnMouseLeave)
EVT_LEFT_DOWN(Map2DEditor::OnMouseDown)
EVT_RIGHT_DOWN(Map2DEditor::OnMouseDown)
EVT_LEFT_UP(Map2DEditor::OnMouseUp)
EVT_RIGHT_UP(Map2DEditor::OnMouseUp)
EVT_MOUSE_CAPTURE_LOST(Map2DEditor::OnCaptureLost)
EVT_LEFT_DCLICK(Map2DEditor::OnDoubleClick)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_MAP_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_MAP_TILE_PICK, wxCommandEvent);
wxDEFINE_EVENT(EVT_MAP_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_MAP_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_MAP_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_MAP_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_MAP_ACTIVATE, wxCommandEvent);

using namespace Landstalker;

Map2DEditor::Map2DEditor(wxWindow* parent)
	: wxHVScrolledWindow(parent, wxID_ANY),
	  m_map(nullptr),
	  m_map_entry(nullptr),
	  m_tileset(nullptr),
	  m_tileset_entry(nullptr),
	  m_g(nullptr),
	  m_active_palette(nullptr),
	  m_mode(Map2DEditor::Mode::SELECT),
	  m_pixelsize(8),
	  m_selectedtile(-1),
	  m_hoveredtile(-1),
	  m_enabletilenumbers(true),
	  m_enableborders(true),
	  m_enableselection(true),
	  m_enablehover(true),
	  m_enablealpha(true),
      m_drawtile(Tile(0))
{
	SetRowColumnCount(1, 1);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	InitialiseBrushesAndPens();
}

Map2DEditor::~Map2DEditor()
{
}

bool Map2DEditor::Save(const wxString& filename, Tilemap2D::Compression compression, int base)
{
	auto orig_base = m_map->GetBase();
	m_map->SetBase(base);
	m_map->Save(filename.ToStdString(), compression);
	m_map->SetBase(orig_base);
	return true;
}

bool Map2DEditor::Open(const wxString& filename, Tilemap2D::Compression compression, int base)
{
	auto retval = m_map->Open(filename.ToStdString(), compression, base);
	ClearHistory();
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	return retval;
}

bool Map2DEditor::Open(const std::vector<Tile>& map, int width, int height, int base)
{
	std::vector<uint8_t> data;
	for (auto t : map)
	{
		data.push_back(t.GetIndex() >> 8);
		data.push_back(t.GetIndex() & 0xFF);
	}
	return Open(data, Tilemap2D::Compression::NONE, width, height, base);
}

bool Map2DEditor::Open(const std::vector<uint8_t>& map, Tilemap2D::Compression compression, int width, int height, int base)
{
	auto retval = m_map->Open(map, width, height, compression, base);
	ClearHistory();
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	return retval;
}

bool Map2DEditor::Open(std::shared_ptr<Tilemap2DEntry> map)
{
	if (m_g == nullptr)
	{
		return false;
	}
	m_map_entry = map;
	m_map = map->GetData();
	m_tileset_entry = m_g->GetTileset(map->GetTileset());
	m_tileset = m_tileset_entry->GetData();
	m_active_palette = m_g->GetPalette(m_g->GetTileset(map->GetTileset())->GetDefaultPalette());
	ClearHistory();
	RedrawAll();
	return true;
}

bool Map2DEditor::New(int width, int height, int base)
{
	m_map_entry = nullptr;
	m_map = std::make_shared<Tilemap2D>();
	m_map->Clear();
	m_map->Resize(width, height);
	m_map->FillIncrementing(base);
	ClearHistory();
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	return false;
}

void Map2DEditor::RedrawAll()
{
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	// Also reached after an import replaces the map wholesale: any selection, float or
	// in-progress operation refers to content (and possibly dimensions) that no longer
	// exist, and a stale rectangle would index out of bounds.
	ResetBoxSelectionState();
	SetDrawTile(0);
	m_selectedtile = 0;
	m_hoveredtile = -1;
	m_mode = Mode::SELECT;
	ScrollToRowColumn(0, 0);
	ForceRedraw();
}

void Map2DEditor::RedrawTiles(int index)
{
	bool refresh = false;
	if (index == -1)
	{
		ForceRedraw();
	}
	else
	{
		for (std::size_t x = 0; x < m_map->GetWidth(); ++x)
			for (std::size_t y = 0; y < m_map->GetHeight(); ++y)
			{
				if (m_map->GetTile(x, y).GetIndex() == index)
				{
					refresh = true;
				}
			}
		if (refresh)
		{
			// Called when the tile's artwork changed elsewhere (e.g. edited in the tiles
			// pane), so the cached map bitmap is stale.
			m_tiles_bmp_dirty = true;
			Refresh(true);
		}
	}
}

void Map2DEditor::RedrawMapTile(const TilePosition& tp)
{
	RedrawMapTile(tp.x + tp.y * m_map->GetWidth());
}

void Map2DEditor::RedrawMapTile(int index)
{
	// Called for data changes (unlike the hover paths), so the cached bitmap is stale.
	m_tiles_bmp_dirty = true;
	RefreshMapTile(index);
}

void Map2DEditor::RefreshMapTile(int tile)
{
	// Repaints a single cell. Hover and edits happen per mouse event, and a full-window
	// Refresh for each is what made the cursor lag.
	if ((tile < 0) || !m_tileset || !m_map)
	{
		return;
	}
	const int cellwidth = m_tileset->GetTileWidth() * m_pixelsize;
	const int cellheight = m_tileset->GetTileHeight() * m_pixelsize;
	const int w = GetTilemapWidth();
	if ((cellwidth <= 0) || (cellheight <= 0) || (w <= 0))
	{
		return;
	}
	wxRect rect((tile % w - GetVisibleColumnsBegin()) * cellwidth,
	            (tile / w - GetVisibleRowsBegin()) * cellheight,
	            cellwidth + 1, cellheight + 1);
	rect.Inflate(1, 1);
	RefreshRect(rect);
}

void Map2DEditor::SetPixelSize(int n)
{
	m_pixelsize = n;

	ForceRedraw();
}

int Map2DEditor::GetPixelSize() const
{
	return m_pixelsize;
}

void Map2DEditor::SetActivePalette(const std::string& name)
{
	if (m_g == nullptr)
	{
		return;
	}
	if (m_active_palette->GetName() != name)
	{
		m_active_palette = m_g->GetPalette(name);
		ForceRedraw();
	}
}

std::string Map2DEditor::GetTileset() const
{
	return m_tileset_entry->GetName();
}

void Map2DEditor::SetTileset(const std::string& name)
{
	if (m_g == nullptr)
	{
		return;
	}
	if (m_tileset_entry->GetName() != name)
	{
		m_tileset_entry = m_g->GetTileset(name);
		m_tileset = m_tileset_entry->GetData();
		ForceRedraw();
	}
}

std::string Map2DEditor::GetActivePalette() const
{
	return m_active_palette->GetName();
}

int Map2DEditor::GetTilemapWidth() const
{
	return m_map->GetWidth();
}

int Map2DEditor::GetTilemapHeight() const
{
	return m_map->GetHeight();
}

std::shared_ptr<Tileset> Map2DEditor::GetTileset()
{
	return m_tileset;
}

std::shared_ptr<Palette> Map2DEditor::GetPalette()
{
	return GetSelectedPalette();
}

std::shared_ptr<Tilemap2D> Map2DEditor::GetMap()
{
	return m_map;
}

void Map2DEditor::SetMode(const Map2DEditor::Mode& mode)
{
	if (m_mode == mode)
	{
		return;
	}
	// Switching mode confirms a pending paste and drops the box selection.
	if (m_sel_drag != SelDrag::None)
	{
		CancelBoxDrag();
	}
	if (m_shape_active)
	{
		CancelShapeDrag();
	}
	ClearBoxSelection(true);
	m_mode = mode;
}

bool Map2DEditor::IsShapeMode(Mode mode)
{
	return (mode == Mode::LINE) || (mode == Mode::RECTANGLE_OUTLINE) ||
	       (mode == Mode::RECTANGLE_FILLED) || (mode == Mode::CIRCLE_OUTLINE) ||
	       (mode == Mode::CIRCLE_FILLED);
}

bool Map2DEditor::IsDrawMode(Mode mode)
{
	return (mode == Mode::PENCIL) || (mode == Mode::FILL) || IsShapeMode(mode);
}

Map2DEditor::Mode Map2DEditor::GetMode() const
{
	return m_mode;
}

void Map2DEditor::SetDrawTile(const Tile& tile)
{
	m_drawtile = tile;
	// The hover and shape previews render the draw tile, so cycling it with +/- (or
	// picking a new one) has to repaint them to be visible.
	if (IsDrawMode(m_mode) && IsHoverValid())
	{
		RefreshMapTile(m_hoveredtile);
	}
	if (m_shape_active)
	{
		RefreshShapeRect();
	}
}

Tile Map2DEditor::GetDrawTile()
{
	return m_drawtile;
}

bool Map2DEditor::GetTileNumbersEnabled() const
{
	return m_enabletilenumbers;
}

void Map2DEditor::SetTileNumbersEnabled(bool enabled)
{
	if (m_enabletilenumbers != enabled)
	{
		m_enabletilenumbers = enabled;
		ForceRedraw();
	}
}

bool Map2DEditor::GetSelectionEnabled() const
{
	return m_enableselection;
}

void Map2DEditor::SetSelectionEnabled(bool enabled)
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

bool Map2DEditor::GetHoverEnabled() const
{
	return m_enablehover;
}

void Map2DEditor::SetHoverEnabled(bool enabled)
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

bool Map2DEditor::GetAlphaEnabled() const
{
	return m_enablealpha;
}

void Map2DEditor::SetAlphaEnabled(bool enabled)
{
	if (m_enablealpha != enabled)
	{
		m_enablealpha = enabled;
		ForceRedraw();
	}
}

bool Map2DEditor::GetBordersEnabled() const
{
	return m_enableborders;
}

void Map2DEditor::SetBordersEnabled(bool enabled)
{
	if (m_enableborders != enabled)
	{
		m_enableborders = enabled;
		ForceRedraw();
	}
}

bool Map2DEditor::IsSelectionValid() const
{
	return IsPositionValid(GetSelection());
}

Map2DEditor::TilePosition Map2DEditor::GetSelection() const
{
	return ToPosition(m_selectedtile);
}

void Map2DEditor::SetSelection(const Map2DEditor::TilePosition& tp)
{
	SelectTile(tp);
}

bool Map2DEditor::IsHoverValid() const
{
	return IsPositionValid(GetHover());
}

Map2DEditor::TilePosition Map2DEditor::GetHover() const
{
	return ToPosition(m_hoveredtile);
}

Tile Map2DEditor::GetSelectedTile() const
{
	return GetTileAtPosition(GetSelection());
}

void Map2DEditor::SetSelectedTile(const Tile& tile)
{
	SetTileAtPosition(GetSelection(), tile);
}

Tile Map2DEditor::GetHoveredTile() const
{
	return GetTileAtPosition(GetHover());
}

void Map2DEditor::SetHoveredTile(const Tile& tile)
{
	SetTileAtPosition(GetHover(), tile);
}

Tile Map2DEditor::GetTileAtPosition(const TilePosition& tp) const
{
	if (IsPositionValid(tp))
	{
		return m_map->GetTile(tp.x, tp.y);
	}
	else
	{
		return Tile(0xFFFF);
	}
}

void Map2DEditor::SetTileAtPosition(const TilePosition& tp, const Tile& tile)
{
	// No-op writes are skipped so repeated pencil clicks don't pollute the undo history.
	if (!IsPositionValid(tp) || (m_map->GetTile(tp.x, tp.y) == tile))
	{
		return;
	}
	PushUndo();
	m_map->SetTile(tile, tp.x, tp.y);
	RedrawMapTile(tp);
	FireEvent(EVT_MAP_CHANGE, std::to_string(ToIndex(tp)));
}

bool Map2DEditor::CanUndo() const
{
	return !m_undo_stack.empty();
}

bool Map2DEditor::CanRedo() const
{
	return !m_redo_stack.empty();
}

void Map2DEditor::Undo()
{
	if (!m_map || m_undo_stack.empty())
	{
		return;
	}
	m_redo_stack.push_back(*m_map);
	auto state = std::move(m_undo_stack.back());
	m_undo_stack.pop_back();
	RestoreHistoryState(std::move(state));
}

void Map2DEditor::Redo()
{
	if (!m_map || m_redo_stack.empty())
	{
		return;
	}
	m_undo_stack.push_back(*m_map);
	auto state = std::move(m_redo_stack.back());
	m_redo_stack.pop_back();
	RestoreHistoryState(std::move(state));
}

void Map2DEditor::PushUndo()
{
	if (!m_map)
	{
		return;
	}
	PushUndoState(Landstalker::Tilemap2D(*m_map));
}

void Map2DEditor::PushUndoState(Landstalker::Tilemap2D&& state)
{
	// A new edit invalidates anything that was undone.
	m_redo_stack.clear();
	m_undo_stack.push_back(std::move(state));
	while (m_undo_stack.size() > 100)
	{
		m_undo_stack.pop_front();
	}
}

void Map2DEditor::RestoreHistoryState(Landstalker::Tilemap2D&& state)
{
	// Any box selection or pending float refers to content that is about to change.
	ResetBoxSelectionState();
	*m_map = std::move(state);
	// A restored state can have different dimensions, invalidating selection and layout.
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	const int count = static_cast<int>(m_map->GetWidth() * m_map->GetHeight());
	if (m_selectedtile >= count)
	{
		m_selectedtile = -1;
	}
	if (m_hoveredtile >= count)
	{
		m_hoveredtile = -1;
	}
	ForceRedraw();
	FireEvent(EVT_MAP_CHANGE, "");
}

void Map2DEditor::ClearHistory()
{
	m_undo_stack.clear();
	m_redo_stack.clear();
	// Called when a map is (re)opened: any selection or float belongs to the old one.
	ResetBoxSelectionState();
}

bool Map2DEditor::IsPositionValid(const TilePosition& tp) const
{
	bool valid = (tp.x >= 0 && tp.x < GetTilemapWidth());
	valid = valid && (tp.y >= 0 && tp.y < GetTilemapHeight());
	return valid;
}

wxCoord Map2DEditor::OnGetRowHeight(size_t /*row*/) const
{
	if (m_tileset)
	{
		return wxCoord(m_pixelsize * m_tileset->GetTileHeight());
	}
	else
	{
		return 0;
	}
}

wxCoord Map2DEditor::OnGetColumnWidth(size_t /*column*/) const
{
	if (m_tileset)
	{
		return wxCoord(m_pixelsize * m_tileset->GetTileWidth());
	}
	else
	{
		return 0;
	}
}

void Map2DEditor::DrawTile(wxDC& dc, int x, int y, const Tile& tile)
{
	// Direct render of one tile, for content not in the cached map bitmap - the draw-tool
	// hover preview. Everything else goes through DrawCachedTile.
	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	wxBitmap bmp = MakeTileBitmap(m_tileset->GetTileBGRA(tile, *GetSelectedPalette()), tw, th);
	wxMemoryDC tdc(bmp);
	dc.StretchBlit({ x, y }, { tw * m_pixelsize, th * m_pixelsize },
		&tdc, { 0, 0 }, { tw, th }, wxCOPY, true, { 0, 0 });
}

void Map2DEditor::RenderTilesBitmap()
{
	// The whole map at native resolution in one image, rebuilt only when the map data or
	// palette changes. Building a bitmap per tile per paint made opening the editor crawl.
	if ((m_tileset == nullptr) || (m_map == nullptr))
	{
		return;
	}
	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	const int w = static_cast<int>(m_map->GetWidth());
	const int h = static_cast<int>(m_map->GetHeight());
	ImageBufferWx buf(w * tw, h * th);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			buf.InsertTile(x * tw, y * th, 0, m_map->GetTile(x, y), *m_tileset, false);
		}
	}
	m_tiles_bmp = std::make_unique<wxBitmap>(buf.MakeImage({ GetSelectedPalette() }, true));
	m_tiles_bmp_dirty = false;
}

void Map2DEditor::DrawSelectionBorders(wxDC& dc)
{
	const int cellwidth = m_tileset->GetTileWidth() * m_pixelsize;
	const int cellheight = m_tileset->GetTileHeight() * m_pixelsize;
	if (IsHoverValid())
	{
		const auto tp = GetHover();
		dc.SetBrush(*m_highlighted_brush);
		dc.SetPen(*m_highlighted_border_pen);
		if (m_hoveredtile == m_selectedtile)
		{
			dc.DrawRectangle({ tp.x * cellwidth + 1, tp.y * cellheight + 1, cellwidth - 2, cellheight - 2 });
		}
		else
		{
			dc.DrawRectangle({ tp.x * cellwidth, tp.y * cellheight, cellwidth, cellheight });
		}
	}
	if (IsSelectionValid())
	{
		const auto tp = GetSelection();
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*m_selected_border_pen);
		dc.DrawRectangle({ tp.x * cellwidth, tp.y * cellheight, cellwidth, cellheight });
	}
}

void Map2DEditor::InitialiseBrushesAndPens()
{
	m_alpha_brush = std::make_unique<wxBrush>();
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
	m_border_pen = std::make_unique<wxPen>(*wxMEDIUM_GREY_PEN);
	m_selected_border_pen =std::make_unique<wxPen>(*wxRED_PEN);
	m_highlighted_border_pen = std::make_unique<wxPen>(*wxBLUE_PEN);
	m_highlighted_brush = std::make_unique<wxBrush>(*wxTRANSPARENT_BRUSH);
}

void Map2DEditor::ForceRedraw()
{
	m_tiles_bmp_dirty = true;
	wxVarHScrollHelper::RefreshAll();
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

std::shared_ptr<Palette> Map2DEditor::GetSelectedPalette()
{
	return m_active_palette->GetData();
}

Map2DEditor::TilePosition Map2DEditor::ToPosition(int index) const
{
	TilePosition tp{ index % GetTilemapWidth(), index / GetTilemapWidth() };
	if (IsPositionValid(tp) == false)
	{
		tp.x = -1;
		tp.y = -1;
	}
	return tp;
}

int Map2DEditor::ToIndex(const TilePosition& tp) const
{
	if (IsPositionValid(tp))
	{
		return tp.x + tp.y * GetTilemapWidth();
	}
	return -1;
}

int Map2DEditor::ConvertXYToTileIdx(const wxPoint& point) const
{
	return ToIndex(ConvertXYToTilePos(point));
}

Map2DEditor::TilePosition Map2DEditor::ConvertXYToTilePos(const wxPoint& point) const
{
	const wxPoint cell = RawCellFromPoint(point);
	TilePosition tp{ cell.x, cell.y };
	if (!IsPositionValid(tp))
	{
		tp.x = -1;
		tp.y = -1;
	}
	return tp;
}

void Map2DEditor::SelectTile(const TilePosition& tp)
{
	int tile = ToIndex(tp);
	if (tile != m_selectedtile)
	{
		const int old = m_selectedtile;
		m_selectedtile = tile;
		// Fired after the update so handlers reading the selection see the new one. The
		// frame reacts by making the selected cell's tile the draw tile.
		FireEvent(EVT_MAP_SELECT, std::to_string(m_selectedtile));
		RefreshMapTile(old);
		RefreshMapTile(tile);
	}
}

bool Map2DEditor::HandleKeyDown(int key, int modifiers)
{
	// Esc abandons an in-progress shape drag or pencil stroke.
	if (m_shape_active && (key == WXK_ESCAPE))
	{
		CancelShapeDrag();
		return true;
	}
	if (m_stroke_active && (key == WXK_ESCAPE))
	{
		CancelStroke();
		return true;
	}
	const bool ctrl = (modifiers == wxMOD_CONTROL);
	// The H/V/P attribute keys work in more than box-select mode; unhandled combinations
	// fall through to the frame. E is a full alias for vflip: Ctrl+V stays paste, so the
	// tristate variant is only reachable as Ctrl+E.
	switch (key)
	{
	case 'h':
	case 'H':
		if (HandleAttributeKey(TileAttributes::Attribute::ATTR_HFLIP, modifiers))
		{
			return true;
		}
		break;
	case 'v':
	case 'V':
		if (!ctrl && HandleAttributeKey(TileAttributes::Attribute::ATTR_VFLIP, modifiers))
		{
			return true;
		}
		break;
	case 'e':
	case 'E':
		if (HandleAttributeKey(TileAttributes::Attribute::ATTR_VFLIP, modifiers))
		{
			return true;
		}
		break;
	case 'p':
	case 'P':
		if (HandleAttributeKey(TileAttributes::Attribute::ATTR_PRIORITY, modifiers))
		{
			return true;
		}
		break;
	default:
		break;
	}
	if (m_mode != Mode::BOX_SELECT)
	{
		return false;
	}
	switch (key)
	{
	case WXK_ESCAPE:
		return CancelActiveBoxOp();
	case WXK_DELETE:
		if ((modifiers == 0) && HasBoxSelection() && !m_sel_floating)
		{
			ClearBoxCells();
			return true;
		}
		return false;
	case 'c':
	case 'C':
		if (ctrl)
		{
			CopyBoxSelection();
			return true;
		}
		return false;
	case 'x':
	case 'X':
		if (ctrl)
		{
			CutBoxSelection();
			return true;
		}
		return false;
	case 'v':
	case 'V':
		if (ctrl)
		{
			PasteBoxCells();
			return true;
		}
		return false;
	case 'a':
	case 'A':
		if (ctrl)
		{
			SelectAllBoxCells();
			return true;
		}
		return false;
	// With a selection, +/- shifts every selected tile ID (+/-10 with Shift, +/-100 with
	// Ctrl); without one the keys fall through to the frame's draw-tile cycling.
	case '+':
	case '=':
	case WXK_NUMPAD_ADD:
		if (HasBoxSelection())
		{
			AdjustBoxTileIds((modifiers & wxMOD_CONTROL) ? 100 : ((modifiers & wxMOD_SHIFT) ? 10 : 1));
			return true;
		}
		return false;
	case '-':
	case '_':
	case WXK_NUMPAD_SUBTRACT:
		if (HasBoxSelection())
		{
			AdjustBoxTileIds((modifiers & wxMOD_CONTROL) ? -100 : ((modifiers & wxMOD_SHIFT) ? -10 : -1));
			return true;
		}
		return false;
	default:
		return false;
	}
}

bool Map2DEditor::HandleAttributeKey(TileAttributes::Attribute attr, int modifiers)
{
	const bool plain = (modifiers == 0);
	const bool ctrl = (modifiers == wxMOD_CONTROL);
	const bool alt = (modifiers == wxMOD_ALT);
	if ((m_mode == Mode::BOX_SELECT) && HasBoxSelection())
	{
		// Plain = full flip (tristate + mirrored layout), Ctrl = tristate set/clear in
		// place, Alt = invert every cell's own bit.
		if (plain)
		{
			ToggleBoxAttribute(attr, AttrToggleMode::FlipMirror);
			return true;
		}
		if (ctrl)
		{
			ToggleBoxAttribute(attr, AttrToggleMode::TriState);
			return true;
		}
		if (alt)
		{
			ToggleBoxAttribute(attr, AttrToggleMode::Toggle);
			return true;
		}
		return false;
	}
	if (IsDrawMode(m_mode) && plain)
	{
		// In the draw modes the keys flip the draw tile itself.
		Tile t = m_drawtile;
		t.Attributes().toggleAttribute(attr);
		SetDrawTile(t);
		return true;
	}
	if ((m_mode == Mode::SELECT) && plain && IsSelectionValid())
	{
		Tile t = GetSelectedTile();
		t.Attributes().toggleAttribute(attr);
		SetSelectedTile(t);
		return true;
	}
	return false;
}

void Map2DEditor::AdjustBoxTileIds(int delta)
{
	if (!HasBoxSelection())
	{
		return;
	}
	const auto adjust = [&](Tile t)
	{
		t.SetIndex(static_cast<uint16_t>(((t.GetIndex() + delta) % 1024 + 1024) % 1024));
		return t;
	};
	if (m_sel_floating)
	{
		for (auto& t : m_float_tiles)
		{
			t = adjust(t);
		}
		RenderBoxFloatBitmap();
		RefreshBoxRect(m_sel_rect);
		return;
	}
	auto snapshot = Tilemap2D(*m_map);
	bool changed = false;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			const auto t = adjust(m_map->GetTile(m_sel_rect.x + x, m_sel_rect.y + y));
			if (m_map->GetTile(m_sel_rect.x + x, m_sel_rect.y + y) != t)
			{
				m_map->SetTile(t, m_sel_rect.x + x, m_sel_rect.y + y);
				changed = true;
			}
		}
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
		FireEvent(EVT_MAP_CHANGE, "");
	}
}

bool Map2DEditor::HasBoxSelection() const
{
	return (m_sel_rect.width > 0) && (m_sel_rect.height > 0);
}

wxPoint Map2DEditor::RawCellFromPoint(const wxPoint& point) const
{
	const int cw = m_tileset->GetTileWidth() * m_pixelsize;
	const int ch = m_tileset->GetTileHeight() * m_pixelsize;
	return { static_cast<int>(GetVisibleColumnsBegin()) + point.x / cw,
	         static_cast<int>(GetVisibleRowsBegin()) + point.y / ch };
}

void Map2DEditor::BeginBoxAction(int cx, int cy)
{
	if (m_map == nullptr)
	{
		return;
	}
	const int w = GetTilemapWidth();
	const int h = GetTilemapHeight();
	if (HasBoxSelection() && m_sel_rect.Contains(wxPoint(cx, cy)))
	{
		m_sel_anchor = wxPoint(cx - m_sel_rect.x, cy - m_sel_rect.y);
		m_sel_op_changed = false;
		if (m_sel_floating)
		{
			// Dragging a pending paste just moves the float; it stays unconfirmed.
			m_sel_drag = SelDrag::Move;
		}
		else
		{
			m_sel_snapshot = std::make_unique<Tilemap2D>(*m_map);
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
			LiftBoxSelection(m_sel_drag == SelDrag::Move);
		}
		CaptureMouse();
	}
	else
	{
		// Clicking outside confirms a pending paste, clears the selection and starts a
		// fresh marquee from here.
		ClearBoxSelection(true);
		if ((cx >= 0) && (cy >= 0) && (cx < w) && (cy < h))
		{
			m_sel_drag = SelDrag::Marquee;
			m_sel_anchor = wxPoint(cx, cy);
			m_sel_rect = wxRect(cx, cy, 1, 1);
			RefreshBoxRect(m_sel_rect);
			CaptureMouse();
		}
	}
}

void Map2DEditor::UpdateBoxDrag(int cx, int cy)
{
	const int w = GetTilemapWidth();
	const int h = GetTilemapHeight();
	switch (m_sel_drag)
	{
	case SelDrag::Marquee:
	{
		const int px = std::clamp(cx, 0, w - 1);
		const int py = std::clamp(cy, 0, h - 1);
		const wxRect next(wxPoint(std::min(m_sel_anchor.x, px), std::min(m_sel_anchor.y, py)),
		                  wxSize(std::abs(px - m_sel_anchor.x) + 1, std::abs(py - m_sel_anchor.y) + 1));
		if (next != m_sel_rect)
		{
			RefreshBoxRect(m_sel_rect);
			m_sel_rect = next;
			RefreshBoxRect(m_sel_rect);
		}
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
	{
		wxPoint tl(cx - m_sel_anchor.x, cy - m_sel_anchor.y);
		tl.x = std::clamp(tl.x, 0, w - m_sel_rect.width);
		tl.y = std::clamp(tl.y, 0, h - m_sel_rect.height);
		if (tl != m_sel_rect.GetTopLeft())
		{
			RefreshBoxRect(m_sel_rect);
			m_sel_rect.x = tl.x;
			m_sel_rect.y = tl.y;
			if (m_sel_drag == SelDrag::Stamp)
			{
				// Continuous duplication: every step leaves a copy on the map.
				m_sel_op_changed |= StampBoxFloating();
			}
			RefreshBoxRect(m_sel_rect);
		}
		break;
	}
	default:
		break;
	}
}

void Map2DEditor::FinishBoxDrag()
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
				m_sel_op_changed |= StampBoxFloating();
			}
			m_sel_floating = false;
			m_float_bmp.reset();
			// A drag that ends where it started leaves the map untouched (erase and
			// re-stamp cancel out); comparing avoids a junk undo entry for that case.
			bool push = false;
			if (m_sel_snapshot && m_sel_op_changed)
			{
				push = !(*m_map == *m_sel_snapshot);
			}
			if (push)
			{
				PushUndoState(std::move(*m_sel_snapshot));
				FireEvent(EVT_MAP_CHANGE, "");
			}
			RefreshBoxRect(m_sel_rect);
		}
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot.reset();
	m_sel_op_changed = false;
}

void Map2DEditor::CancelBoxDrag()
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
		RefreshBoxRect(old);
		break;
	}
	case SelDrag::Move:
	case SelDrag::Duplicate:
	case SelDrag::Stamp:
		if (m_sel_from_paste)
		{
			// Cancelling mid-drag drops the pending paste entirely.
			DiscardBoxFloating();
			const wxRect old = m_sel_rect;
			m_sel_rect = wxRect();
			RefreshBoxRect(old);
		}
		else if (m_sel_snapshot)
		{
			// Puts the map back exactly as it was before the lift.
			m_sel_floating = false;
			m_float_bmp.reset();
			RestoreHistoryState(std::move(*m_sel_snapshot));
		}
		break;
	default:
		break;
	}
	m_sel_drag = SelDrag::None;
	m_sel_snapshot.reset();
	m_sel_op_changed = false;
}

bool Map2DEditor::CancelActiveBoxOp()
{
	if (m_sel_drag != SelDrag::None)
	{
		CancelBoxDrag();
		return true;
	}
	if (m_sel_floating)
	{
		// Esc cancels a pending paste outright rather than confirming it.
		DiscardBoxFloating();
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshBoxRect(old);
		return true;
	}
	if (HasBoxSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshBoxRect(old);
		return true;
	}
	return false;
}

void Map2DEditor::ClearBoxSelection(bool confirm_floating)
{
	if (m_sel_floating)
	{
		if (confirm_floating)
		{
			ConfirmBoxFloating();
		}
		else
		{
			DiscardBoxFloating();
		}
	}
	if (HasBoxSelection())
	{
		const wxRect old = m_sel_rect;
		m_sel_rect = wxRect();
		RefreshBoxRect(old);
	}
}

void Map2DEditor::ConfirmBoxFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	auto snapshot = Tilemap2D(*m_map);
	const bool changed = StampBoxFloating();
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		FireEvent(EVT_MAP_CHANGE, "");
	}
	RefreshBoxRect(m_sel_rect);
}

void Map2DEditor::DiscardBoxFloating()
{
	if (!m_sel_floating)
	{
		return;
	}
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_float_bmp.reset();
	m_float_tiles.clear();
	RefreshBoxRect(m_sel_rect);
}

void Map2DEditor::LiftBoxSelection(bool erase_source)
{
	m_float_tiles = ReadBoxRect(m_sel_rect);
	if (erase_source)
	{
		bool changed = false;
		for (int y = 0; y < m_sel_rect.height; ++y)
		{
			for (int x = 0; x < m_sel_rect.width; ++x)
			{
				if (m_map->GetTile(m_sel_rect.x + x, m_sel_rect.y + y) != Tile())
				{
					m_map->SetTile(Tile(), m_sel_rect.x + x, m_sel_rect.y + y);
					changed = true;
				}
			}
		}
		if (changed)
		{
			m_tiles_bmp_dirty = true;
			RefreshBoxRect(m_sel_rect);
			m_sel_op_changed = true;
		}
	}
	m_sel_floating = true;
	m_sel_from_paste = false;
	RenderBoxFloatBitmap();
	RefreshBoxRect(m_sel_rect);
}

bool Map2DEditor::StampBoxFloating()
{
	if (m_float_tiles.size() !=
	    static_cast<std::size_t>(m_sel_rect.width) * static_cast<std::size_t>(m_sel_rect.height))
	{
		return false;
	}
	bool changed = false;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			const Tile& t = m_float_tiles[x + y * m_sel_rect.width];
			if (m_map->GetTile(m_sel_rect.x + x, m_sel_rect.y + y) != t)
			{
				m_map->SetTile(t, m_sel_rect.x + x, m_sel_rect.y + y);
				changed = true;
			}
		}
	}
	if (changed)
	{
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
	}
	return changed;
}

void Map2DEditor::RenderBoxFloatBitmap()
{
	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	ImageBufferWx buf(m_sel_rect.width * tw, m_sel_rect.height * th);
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			buf.InsertTile(x * tw, y * th, 0, m_float_tiles[x + y * m_sel_rect.width],
				*m_tileset, false);
		}
	}
	m_float_bmp = std::make_unique<wxBitmap>(buf.MakeImage({ GetSelectedPalette() }, true));
}

void Map2DEditor::ClearBoxCells()
{
	if (!HasBoxSelection() || m_sel_floating)
	{
		return;
	}
	auto snapshot = Tilemap2D(*m_map);
	bool changed = false;
	for (int y = 0; y < m_sel_rect.height; ++y)
	{
		for (int x = 0; x < m_sel_rect.width; ++x)
		{
			if (m_map->GetTile(m_sel_rect.x + x, m_sel_rect.y + y) != Tile())
			{
				m_map->SetTile(Tile(), m_sel_rect.x + x, m_sel_rect.y + y);
				changed = true;
			}
		}
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
		FireEvent(EVT_MAP_CHANGE, "");
	}
}

void Map2DEditor::ToggleBoxAttribute(TileAttributes::Attribute attr, AttrToggleMode toggle_mode)
{
	if (!HasBoxSelection())
	{
		return;
	}
	const int w = m_sel_rect.width;
	const int h = m_sel_rect.height;
	// In FlipMirror mode H/V flips mirror the block's cell layout too, so the result reads
	// as a true mirror image of the selection; priority has no spatial meaning and leaves
	// positions alone, as do the other toggle modes.
	const bool mirror_x = (toggle_mode == AttrToggleMode::FlipMirror) &&
		(attr == TileAttributes::Attribute::ATTR_HFLIP);
	const bool mirror_y = (toggle_mode == AttrToggleMode::FlipMirror) &&
		(attr == TileAttributes::Attribute::ATTR_VFLIP);
	const auto transform = [&](const std::vector<Tile>& src)
	{
		// Tristate: if any cell lacks the bit, set it everywhere; only clear once all
		// have it. Toggle mode instead inverts each cell's own bit.
		bool all_set = true;
		for (const auto& t : src)
		{
			all_set = all_set && t.Attributes().getAttribute(attr);
		}
		std::vector<Tile> out(src.size());
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const int sx = mirror_x ? (w - 1 - x) : x;
				const int sy = mirror_y ? (h - 1 - y) : y;
				Tile t = src[sx + sy * w];
				if (toggle_mode == AttrToggleMode::Toggle)
				{
					t.Attributes().toggleAttribute(attr);
				}
				else if (all_set)
				{
					t.Attributes().clearAttribute(attr);
				}
				else
				{
					t.Attributes().setAttribute(attr);
				}
				out[x + y * w] = t;
			}
		}
		return out;
	};
	if (m_sel_floating)
	{
		m_float_tiles = transform(m_float_tiles);
		RenderBoxFloatBitmap();
		RefreshBoxRect(m_sel_rect);
		return;
	}
	auto snapshot = Tilemap2D(*m_map);
	const auto result = transform(ReadBoxRect(m_sel_rect));
	bool changed = false;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const Tile& t = result[x + y * w];
			if (m_map->GetTile(m_sel_rect.x + x, m_sel_rect.y + y) != t)
			{
				m_map->SetTile(t, m_sel_rect.x + x, m_sel_rect.y + y);
				changed = true;
			}
		}
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		RefreshBoxRect(m_sel_rect);
		FireEvent(EVT_MAP_CHANGE, "");
	}
}

void Map2DEditor::CopyBoxSelection()
{
	if (!HasBoxSelection())
	{
		return;
	}
	m_cell_clipboard.rect = m_sel_rect;
	m_cell_clipboard.tiles = m_sel_floating ? m_float_tiles : ReadBoxRect(m_sel_rect);
}

void Map2DEditor::CutBoxSelection()
{
	if (!HasBoxSelection())
	{
		return;
	}
	CopyBoxSelection();
	if (m_sel_floating)
	{
		// Cutting a pending paste just removes the float; the map never had it.
		DiscardBoxFloating();
	}
	else
	{
		ClearBoxCells();
	}
}

void Map2DEditor::PasteBoxCells()
{
	if (m_cell_clipboard.tiles.empty())
	{
		return;
	}
	ClearBoxSelection(true);
	wxRect r = m_cell_clipboard.rect;
	// A clipboard from a larger map may not fit at all; clamp what we can.
	if ((r.width > GetTilemapWidth()) || (r.height > GetTilemapHeight()))
	{
		return;
	}
	r.x = std::clamp(r.x, 0, GetTilemapWidth() - r.width);
	r.y = std::clamp(r.y, 0, GetTilemapHeight() - r.height);
	m_sel_rect = r;
	m_float_tiles = m_cell_clipboard.tiles;
	m_sel_floating = true;
	m_sel_from_paste = true;
	RenderBoxFloatBitmap();
	RefreshBoxRect(m_sel_rect);
}

void Map2DEditor::SelectAllBoxCells()
{
	if (m_map == nullptr)
	{
		return;
	}
	ClearBoxSelection(true);
	m_sel_rect = wxRect(0, 0, GetTilemapWidth(), GetTilemapHeight());
	RefreshBoxRect(m_sel_rect);
}

std::vector<Tile> Map2DEditor::ReadBoxRect(const wxRect& rect) const
{
	std::vector<Tile> out;
	out.reserve(static_cast<std::size_t>(rect.width) * static_cast<std::size_t>(rect.height));
	for (int y = 0; y < rect.height; ++y)
	{
		for (int x = 0; x < rect.width; ++x)
		{
			out.push_back(m_map->GetTile(rect.x + x, rect.y + y));
		}
	}
	return out;
}

void Map2DEditor::RefreshBoxRect(const wxRect& rect)
{
	if ((rect.width <= 0) || (rect.height <= 0) || !m_tileset)
	{
		return;
	}
	const int cw = m_tileset->GetTileWidth() * m_pixelsize;
	const int ch = m_tileset->GetTileHeight() * m_pixelsize;
	wxRect r((rect.x - static_cast<int>(GetVisibleColumnsBegin())) * cw,
	         (rect.y - static_cast<int>(GetVisibleRowsBegin())) * ch,
	         rect.width * cw + 1, rect.height * ch + 1);
	r.Inflate(2, 2);
	RefreshRect(r);
}

void Map2DEditor::DrawBoxSelection(wxDC& dc)
{
	if (!HasBoxSelection())
	{
		return;
	}
	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	const int cw = tw * m_pixelsize;
	const int ch = th * m_pixelsize;
	if (m_sel_floating && (m_float_bmp != nullptr))
	{
		wxMemoryDC mem(*m_float_bmp);
		dc.StretchBlit(m_sel_rect.x * cw, m_sel_rect.y * ch,
		               m_sel_rect.width * cw, m_sel_rect.height * ch,
		               &mem, 0, 0, m_sel_rect.width * tw, m_sel_rect.height * th, wxCOPY, true);
		mem.SelectObject(wxNullBitmap);
	}
	// White underlay + black dashes stays visible over any artwork.
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(*wxWHITE_PEN);
	dc.DrawRectangle(m_sel_rect.x * cw, m_sel_rect.y * ch,
	                 m_sel_rect.width * cw + 1, m_sel_rect.height * ch + 1);
	dc.SetPen(wxPen(*wxBLACK, 1, wxPENSTYLE_SHORT_DASH));
	dc.DrawRectangle(m_sel_rect.x * cw, m_sel_rect.y * ch,
	                 m_sel_rect.width * cw + 1, m_sel_rect.height * ch + 1);
}

void Map2DEditor::ResetBoxSelectionState()
{
	m_sel_rect = wxRect();
	m_sel_drag = SelDrag::None;
	m_sel_floating = false;
	m_sel_from_paste = false;
	m_sel_op_changed = false;
	m_sel_snapshot.reset();
	m_float_bmp.reset();
	m_float_tiles.clear();
	// The cell clipboard survives on purpose, so content can be pasted across maps.
	m_shape_active = false;
	m_stroke_active = false;
	m_stroke_snapshot.reset();
	m_stroke_changed = false;
}

void Map2DEditor::BeginShapeDrag(int cx, int cy)
{
	// Shapes must start on the map; the drag itself may then wander outside and clamp.
	if ((m_map == nullptr) || !IsPositionValid({ cx, cy }))
	{
		return;
	}
	m_shape_active = true;
	m_shape_anchor = wxPoint(cx, cy);
	m_shape_current = m_shape_anchor;
	CaptureMouse();
	RefreshShapeRect();
}

void Map2DEditor::UpdateShapeDrag(int cx, int cy)
{
	const wxPoint next(std::clamp(cx, 0, GetTilemapWidth() - 1),
	                   std::clamp(cy, 0, GetTilemapHeight() - 1));
	if (next != m_shape_current)
	{
		RefreshShapeRect();
		m_shape_current = next;
		RefreshShapeRect();
	}
}

void Map2DEditor::CommitShapeDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_shape_active)
	{
		return;
	}
	m_shape_active = false;
	auto snapshot = Tilemap2D(*m_map);
	bool changed = false;
	for (const auto& pt : MakeShapeCells())
	{
		if (IsPositionValid({ pt.x, pt.y }) && (m_map->GetTile(pt.x, pt.y) != m_drawtile))
		{
			m_map->SetTile(m_drawtile, pt.x, pt.y);
			changed = true;
		}
	}
	if (changed)
	{
		PushUndoState(std::move(snapshot));
		m_tiles_bmp_dirty = true;
		FireEvent(EVT_MAP_CHANGE, "");
	}
	// Repaint regardless, to clear the preview overlay.
	RefreshShapeRect();
}

void Map2DEditor::CancelShapeDrag()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_shape_active)
	{
		return;
	}
	m_shape_active = false;
	RefreshShapeRect();
}

std::vector<wxPoint> Map2DEditor::MakeShapeCells() const
{
	ShapeTool tool = ShapeTool::Line;
	switch (m_mode)
	{
	case Mode::LINE:              tool = ShapeTool::Line;             break;
	case Mode::RECTANGLE_OUTLINE: tool = ShapeTool::RectangleOutline; break;
	case Mode::RECTANGLE_FILLED:  tool = ShapeTool::RectangleFilled;  break;
	case Mode::CIRCLE_OUTLINE:    tool = ShapeTool::CircleOutline;    break;
	case Mode::CIRCLE_FILLED:     tool = ShapeTool::CircleFilled;     break;
	default:
		return {};
	}
	return MakeShapeToolPoints(tool, m_shape_anchor, m_shape_current);
}

void Map2DEditor::RefreshShapeRect()
{
	// Every shape lies within the drag's bounding rectangle, so that is all that repaints.
	RefreshBoxRect(wxRect(wxPoint(std::min(m_shape_anchor.x, m_shape_current.x),
	                              std::min(m_shape_anchor.y, m_shape_current.y)),
	                      wxSize(std::abs(m_shape_current.x - m_shape_anchor.x) + 1,
	                             std::abs(m_shape_current.y - m_shape_anchor.y) + 1)));
}

void Map2DEditor::DrawShapePreview(wxDC& dc)
{
	if (!m_shape_active)
	{
		return;
	}
	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	const int cw = tw * m_pixelsize;
	const int ch = th * m_pixelsize;
	// One native-resolution bitmap of the draw tile, blitted per cell: a filled shape can
	// cover hundreds of cells and building a bitmap for each is visibly slow.
	wxBitmap bmp = MakeTileBitmap(m_tileset->GetTileBGRA(m_drawtile, *GetSelectedPalette()), tw, th);
	wxMemoryDC tdc(bmp);
	dc.SetPen(*wxTRANSPARENT_PEN);
	for (const auto& pt : MakeShapeCells())
	{
		if (!IsPositionValid({ pt.x, pt.y }))
		{
			continue;
		}
		// Backdrop first, so the tile's transparent pixels show the checkerboard rather
		// than the map cell underneath.
		dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
		dc.DrawRectangle(pt.x * cw, pt.y * ch, cw, ch);
		dc.StretchBlit({ pt.x * cw, pt.y * ch }, { cw, ch }, &tdc, { 0, 0 }, { tw, th },
			wxCOPY, true, { 0, 0 });
	}
	tdc.SelectObject(wxNullBitmap);
}

void Map2DEditor::FloodFillAt(int cx, int cy)
{
	if ((m_map == nullptr) || !IsPositionValid({ cx, cy }))
	{
		return;
	}
	const Tile target = m_map->GetTile(cx, cy);
	if (target == m_drawtile)
	{
		return;
	}
	auto snapshot = Tilemap2D(*m_map);
	// Painted cells no longer match the target, which doubles as the visited check.
	std::vector<wxPoint> stack{ { cx, cy } };
	while (!stack.empty())
	{
		const wxPoint p = stack.back();
		stack.pop_back();
		if (!IsPositionValid({ p.x, p.y }) || (m_map->GetTile(p.x, p.y) != target))
		{
			continue;
		}
		m_map->SetTile(m_drawtile, p.x, p.y);
		stack.push_back({ p.x + 1, p.y });
		stack.push_back({ p.x - 1, p.y });
		stack.push_back({ p.x, p.y + 1 });
		stack.push_back({ p.x, p.y - 1 });
	}
	PushUndoState(std::move(snapshot));
	m_tiles_bmp_dirty = true;
	Refresh();
	FireEvent(EVT_MAP_CHANGE, "");
}

void Map2DEditor::BeginStroke(int cx, int cy)
{
	// Strokes must start on the map; the drag itself may then wander outside and clamp.
	if ((m_map == nullptr) || !IsPositionValid({ cx, cy }))
	{
		return;
	}
	m_stroke_active = true;
	m_stroke_changed = false;
	m_stroke_snapshot = std::make_unique<Tilemap2D>(*m_map);
	m_stroke_last = wxPoint(cx, cy);
	CaptureMouse();
	if (m_map->GetTile(cx, cy) != m_drawtile)
	{
		m_map->SetTile(m_drawtile, cx, cy);
		m_stroke_changed = true;
		m_tiles_bmp_dirty = true;
		RedrawMapTile(cx + cy * GetTilemapWidth());
	}
}

void Map2DEditor::StrokeTo(int cx, int cy)
{
	const wxPoint next(std::clamp(cx, 0, GetTilemapWidth() - 1),
	                   std::clamp(cy, 0, GetTilemapHeight() - 1));
	if (next == m_stroke_last)
	{
		return;
	}
	// Mouse moves arrive coalesced, so consecutive samples can be several cells apart;
	// joining them with a line keeps fast strokes continuous, as in the pixel editors.
	PlotShapeLine(m_stroke_last, next, [&](int px, int py)
	{
		if (IsPositionValid({ px, py }) && (m_map->GetTile(px, py) != m_drawtile))
		{
			m_map->SetTile(m_drawtile, px, py);
			m_stroke_changed = true;
			m_tiles_bmp_dirty = true;
			RedrawMapTile(px + py * GetTilemapWidth());
		}
	});
	m_stroke_last = next;
}

void Map2DEditor::EndStroke()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_stroke_active)
	{
		return;
	}
	m_stroke_active = false;
	// Repainting cells with the tiles they already held leaves nothing to undo.
	const bool push = m_stroke_changed && m_stroke_snapshot && !(*m_map == *m_stroke_snapshot);
	if (push)
	{
		PushUndoState(std::move(*m_stroke_snapshot));
		FireEvent(EVT_MAP_CHANGE, "");
	}
	m_stroke_snapshot.reset();
	m_stroke_changed = false;
}

void Map2DEditor::CancelStroke()
{
	if (HasCapture())
	{
		ReleaseMouse();
	}
	if (!m_stroke_active)
	{
		return;
	}
	m_stroke_active = false;
	if (m_stroke_snapshot && m_stroke_changed)
	{
		// Puts the map back exactly as it was before the stroke started.
		*m_map = std::move(*m_stroke_snapshot);
		m_tiles_bmp_dirty = true;
		Refresh();
	}
	m_stroke_snapshot.reset();
	m_stroke_changed = false;
}

void Map2DEditor::OnDraw(wxDC& dc)
{
	// Same pipeline as the tileset and blockset editors: content renders at native
	// resolution into m_tiles_bmp when data changes, and each paint is one scaled blit of
	// the damaged cells plus overlays, all clipped to the damaged area. The previous
	// design - a zoom-scaled cache filled with per-cell draws - made opening slow.
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	if (!m_tileset || !m_map)
	{
		dc.Clear();
		return;
	}
	if (m_tiles_bmp_dirty || (m_tiles_bmp == nullptr))
	{
		RenderTilesBitmap();
	}
	if (m_tiles_bmp == nullptr)
	{
		dc.Clear();
		return;
	}

	const int tw = static_cast<int>(m_tileset->GetTileWidth());
	const int th = static_cast<int>(m_tileset->GetTileHeight());
	const int cellwidth = tw * m_pixelsize;
	const int cellheight = th * m_pixelsize;
	const int w = static_cast<int>(m_map->GetWidth());
	const int h = static_cast<int>(m_map->GetHeight());
	if ((cellwidth <= 0) || (cellheight <= 0))
	{
		dc.Clear();
		return;
	}

	wxRect damage = GetUpdateRegion().GetBox();
	damage.Offset(GetVisibleColumnsBegin() * cellwidth, GetVisibleRowsBegin() * cellheight);
	dc.SetClippingRegion(damage);
	dc.Clear();

	const int sx = std::max(static_cast<int>(GetVisibleColumnsBegin()), damage.GetLeft() / cellwidth);
	const int ex = std::min({ static_cast<int>(GetVisibleColumnsEnd()) + 1, w, damage.GetRight() / cellwidth + 1 });
	const int sy = std::max(static_cast<int>(GetVisibleRowsBegin()), damage.GetTop() / cellheight);
	const int ey = std::min({ static_cast<int>(GetVisibleRowsEnd()) + 1, h, damage.GetBottom() / cellheight + 1 });
	if ((ex <= sx) || (ey <= sy))
	{
		return;
	}

	// Checkerboard backdrop in one rectangle - the map is a full rectangle of tiles.
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
	dc.DrawRectangle(sx * cellwidth, sy * cellheight, (ex - sx) * cellwidth, (ey - sy) * cellheight);

	// One scaled blit of the damaged cells out of the native-resolution map bitmap.
	wxMemoryDC tiles(*m_tiles_bmp);
	dc.StretchBlit({ sx * cellwidth, sy * cellheight }, { (ex - sx) * cellwidth, (ey - sy) * cellheight },
		&tiles, { sx * tw, sy * th }, { (ex - sx) * tw, (ey - sy) * th },
		wxCOPY, true, { sx * tw, sy * th });
	tiles.SelectObject(wxNullBitmap);

	DrawOverlays(dc, sx, ex, sy, ey);
	DrawSelectionBorders(dc);
	DrawBoxSelection(dc);
	DrawShapePreview(dc);
}

void Map2DEditor::DrawOverlays(wxDC& dc, int sx, int ex, int sy, int ey)
{
	const int cellwidth = m_tileset->GetTileWidth() * m_pixelsize;
	const int cellheight = m_tileset->GetTileHeight() * m_pixelsize;
	dc.SetTextForeground(wxColour(255, 255, 255));
	dc.SetTextBackground(wxColour(150, 150, 150));
	dc.SetBackgroundMode(wxSOLID);

	if (m_pixelsize > 3)
	{
		m_border_pen->SetStyle(wxPENSTYLE_SOLID);
	}
	else
	{
		m_border_pen->SetStyle(wxPENSTYLE_TRANSPARENT);
	}

	const bool preview = m_enablehover && IsDrawMode(m_mode) && !m_shape_active && IsHoverValid();
	for (int y = sy; y < ey; ++y)
	{
		for (int x = sx; x < ex; ++x)
		{
			auto t = m_map->GetTile(x, y);
			if (preview && (ToIndex({ x, y }) == m_hoveredtile))
			{
				// Draw-mode preview of the draw tile under the cursor - not part of the map
				// data, so it is not in the cached bitmap. Restore the backdrop first so its
				// transparent pixels show the checkerboard, not the underlying map tile.
				t = m_drawtile;
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
				dc.DrawRectangle(x * cellwidth, y * cellheight, cellwidth, cellheight);
				DrawTile(dc, x * cellwidth, y * cellheight, t);
			}
			if (m_enableborders)
			{
				dc.SetPen(*m_border_pen);
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				dc.DrawRectangle({ x * cellwidth, y * cellheight, cellwidth + 1, cellheight + 1 });
			}
			if (m_enabletilenumbers)
			{
				auto label = wxString::Format("%03d%c%c%c", t.GetIndex(), t.Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP) ? 'H' : ' ',
					t.Attributes().getAttribute(TileAttributes::Attribute::ATTR_VFLIP) ? 'V' : ' ',
					t.Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY) ? '!' : ' ');
				auto extent = dc.GetTextExtent(label);
				if ((extent.GetWidth() < cellwidth - 2) && (extent.GetHeight() < cellheight - 2))
				{
					dc.DrawText(label, { x * cellwidth + 2, y * cellheight + 2 });
				}
			}
		}
	}
}

void Map2DEditor::OnPaint(wxPaintEvent& /*evt*/)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void Map2DEditor::OnSize(wxSizeEvent& evt)
{
	wxVarHScrollHelper::HandleOnSize(evt);
	wxVarVScrollHelper::HandleOnSize(evt);
	Refresh(false);
	evt.Skip();
}

void Map2DEditor::OnMouseMove(wxMouseEvent& evt)
{
	if (m_sel_drag != SelDrag::None)
	{
		const wxPoint cell = RawCellFromPoint(evt.GetPosition());
		UpdateBoxDrag(cell.x, cell.y);
		evt.Skip();
		return;
	}
	if (m_shape_active)
	{
		const wxPoint cell = RawCellFromPoint(evt.GetPosition());
		UpdateShapeDrag(cell.x, cell.y);
		evt.Skip();
		return;
	}
	if (m_stroke_active)
	{
		// Paint, then fall through so the hover tracking stays current.
		const wxPoint cell = RawCellFromPoint(evt.GetPosition());
		StrokeTo(cell.x, cell.y);
	}
	if (!m_enablehover) return;
	int sel = ConvertXYToTileIdx(evt.GetPosition());
	if (sel != m_hoveredtile)
	{
		const int old = m_hoveredtile;
		m_hoveredtile = sel;
		FireEvent(EVT_MAP_HOVER, std::to_string(m_hoveredtile));
		RefreshMapTile(old);
		RefreshMapTile(m_hoveredtile);
	}
	evt.Skip();
}

void Map2DEditor::OnMouseLeave(wxMouseEvent& evt)
{
	if ((m_sel_drag != SelDrag::None) || m_shape_active || m_stroke_active)
	{
		// The mouse is captured; the drag continues outside the window.
		evt.Skip();
		return;
	}
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		const int old = m_hoveredtile;
		m_hoveredtile = -1;
		FireEvent(EVT_MAP_HOVER, std::to_string(m_hoveredtile));
		RefreshMapTile(old);
	}
	evt.Skip();
}

void Map2DEditor::OnMouseDown(wxMouseEvent& evt)
{
	// Clicking the canvas takes the keyboard, so Esc and the Ctrl shortcuts work without
	// first having to tab into the window.
	SetFocus();
	if (!m_enableselection) return;
	auto sel = ConvertXYToTilePos(evt.GetPosition());
	if (evt.RightDown())
	{
		// Right-click in any draw mode picks up the tile under the cursor.
		if (IsDrawMode(m_mode) && !m_shape_active && !m_stroke_active && IsPositionValid(sel))
		{
			SetDrawTile(m_map->GetTile(sel.x, sel.y));
			FireEvent(EVT_MAP_TILE_PICK, std::to_string(m_drawtile.GetIndex()));
		}
		evt.Skip();
		return;
	}
	switch (m_mode)
	{
	case Mode::SELECT:
		SelectTile(sel);
		break;
	case Mode::PENCIL:
		BeginStroke(sel.x, sel.y);
		break;
	case Mode::BOX_SELECT:
	{
		const wxPoint cell = RawCellFromPoint(evt.GetPosition());
		BeginBoxAction(cell.x, cell.y);
		break;
	}
	case Mode::FILL:
		FloodFillAt(sel.x, sel.y);
		break;
	default:
		if (IsShapeMode(m_mode))
		{
			const wxPoint cell = RawCellFromPoint(evt.GetPosition());
			BeginShapeDrag(cell.x, cell.y);
		}
		break;
	}
	evt.Skip();
}

void Map2DEditor::OnMouseUp(wxMouseEvent& evt)
{
	if (evt.LeftUp() && (m_sel_drag != SelDrag::None))
	{
		if (evt.RightIsDown())
		{
			// Releasing the left button with the right still held cancels the operation.
			CancelBoxDrag();
		}
		else
		{
			FinishBoxDrag();
		}
	}
	else if (evt.LeftUp() && m_shape_active)
	{
		if (evt.RightIsDown())
		{
			CancelShapeDrag();
		}
		else
		{
			CommitShapeDrag();
		}
	}
	else if (evt.LeftUp() && m_stroke_active)
	{
		if (evt.RightIsDown())
		{
			CancelStroke();
		}
		else
		{
			EndStroke();
		}
	}
	evt.Skip();
}

void Map2DEditor::OnCaptureLost(wxMouseCaptureLostEvent& /*evt*/)
{
	// Capture already gone - drop the box-drag state, clear any shape preview off the
	// screen, and commit an in-progress stroke so its painted cells stay undoable.
	m_sel_drag = SelDrag::None;
	m_sel_snapshot.reset();
	m_sel_op_changed = false;
	if (m_shape_active)
	{
		CancelShapeDrag();
	}
	if (m_stroke_active)
	{
		EndStroke();
	}
}

void Map2DEditor::OnDoubleClick(wxMouseEvent& evt)
{
	if (m_mode == Mode::SELECT)
	{
		FireTilesetEvent(EVT_MAP_EDIT_REQUEST, std::to_string(GetSelectedTile().GetIndex()));
	}
	else if (IsDrawMode(m_mode))
	{
		// Rapid clicks arrive as a double-click; treat it as another press so quick
		// pencil dabs and shape starts aren't dropped.
		OnMouseDown(evt);
		return;
	}
	evt.Skip();
}

void Map2DEditor::FireEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(&m_map);
	wxPostEvent(this->GetParent(), evt);
}

void Map2DEditor::FireTilesetEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(&m_tileset);
	wxPostEvent(this->GetParent(), evt);
}

bool Map2DEditor::InsertRow(int row)
{
	if (row > static_cast<int>(m_map->GetHeight()))
	{
		return false;
	}
	PushUndo();
	auto pos = GetSelection();
	m_map->InsertRow(row, GetSelectedTile());
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

bool Map2DEditor::DeleteRow(int row)
{
	if (row >= static_cast<int>(m_map->GetHeight()))
	{
		return false;
	}
	PushUndo();
	auto pos = GetSelection();
	m_map->DeleteRow(row);
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

bool Map2DEditor::InsertColumn(int column)
{
	if (column > static_cast<int>(m_map->GetWidth()))
	{
		return false;
	}
	PushUndo();
	auto pos = GetSelection();
	m_map->InsertColumn(column, GetSelectedTile());
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

bool Map2DEditor::DeleteColumn(int column)
{
	if (column >= static_cast<int>(m_map->GetWidth()))
	{
		return false;
	}
	PushUndo();
	auto pos = GetSelection();
	m_map->DeleteColumn(column);
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

