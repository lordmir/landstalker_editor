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
EVT_LEFT_DCLICK(Map2DEditor::OnDoubleClick)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_MAP_SELECT, wxCommandEvent);
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
	  m_selectable(true),
	  m_selectedtile(-1),
	  m_hoveredtile(-1),
	  m_tilebase(0),
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
	SetDrawTile(0);
	m_selectedtile = 0;
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
					m_redraw_list.insert(x + y * m_map->GetWidth());
					refresh = true;
				}
			}
		if (refresh)
		{
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
	m_redraw_list.insert(index);
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
	m_mode = mode;
}

Map2DEditor::Mode Map2DEditor::GetMode() const
{
	return m_mode;
}

void Map2DEditor::SetDrawTile(const Tile& tile)
{
	m_drawtile = tile;
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
		m_redraw_list.insert(m_selectedtile);
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
		m_redraw_list.insert(m_selectedtile);
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
	// A new edit invalidates anything that was undone.
	m_redo_stack.clear();
	m_undo_stack.push_back(*m_map);
	while (m_undo_stack.size() > 100)
	{
		m_undo_stack.pop_front();
	}
}

void Map2DEditor::RestoreHistoryState(Landstalker::Tilemap2D&& state)
{
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

bool Map2DEditor::UpdateRowCount()
{
	return false;
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
	int sx = GetVisibleColumnsBegin();
	int sy = GetVisibleRowsBegin();
	int x = sx + point.x / (m_pixelsize * m_tileset->GetTileWidth());
	int y = sy + point.y / (m_pixelsize * m_tileset->GetTileHeight());
	TilePosition tp{ x,y };
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
	if ((m_selectedtile != -1) && (tile != m_selectedtile))
	{
		m_redraw_list.insert(m_selectedtile);
	}
	if (tile != m_selectedtile)
	{
		const int old = m_selectedtile;
		FireEvent(EVT_MAP_SELECT, std::to_string(m_selectedtile));
		m_selectedtile = tile;
		m_drawtile = tile;
		RefreshMapTile(old);
		RefreshMapTile(tile);
	}
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
	// Direct painting has no per-cell cache to maintain; these are only bookkeeping now.
	m_redraw_list.clear();
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

	const bool preview = m_enablehover && (m_mode != Mode::SELECT) && IsHoverValid();
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
	if (!m_enablehover) return;
	int sel = ConvertXYToTileIdx(evt.GetPosition());
	if ((m_hoveredtile != -1) && (sel != m_hoveredtile))
	{
		m_redraw_list.insert(m_hoveredtile);
	}
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
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		const int old = m_hoveredtile;
		m_redraw_list.insert(m_hoveredtile);
		m_hoveredtile = -1;
		FireEvent(EVT_MAP_HOVER, std::to_string(m_hoveredtile));
		RefreshMapTile(old);
	}
	evt.Skip();
}

void Map2DEditor::OnMouseDown(wxMouseEvent& evt)
{
	if (!m_enableselection) return;
	auto sel = ConvertXYToTilePos(evt.GetPosition());
	switch (m_mode)
	{
	case Mode::SELECT:
		SelectTile(sel);
		break;
	case Mode::PENCIL:
		SetHoveredTile(m_drawtile);
		break;
	}
	evt.Skip();
}

void Map2DEditor::OnDoubleClick(wxMouseEvent& evt)
{
	if (m_mode == Mode::SELECT)
	{
		FireTilesetEvent(EVT_MAP_EDIT_REQUEST, std::to_string(GetSelectedTile().GetIndex()));
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

