#include "Map2DEditor.h"
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <algorithm>

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

Map2DEditor::Map2DEditor(wxWindow* parent)
	: wxHVScrolledWindow(parent, wxID_ANY),
	m_pixelsize(8),
	m_mode(Map2DEditor::Mode::SELECT),
	m_drawtile(Tile(0)),
	m_redraw_all(true),
	m_enablealpha(true),
	m_enableborders(true),
	m_enabletilenumbers(true),
	m_enableselection(true),
	m_enablehover(true)
{
	SetRowColumnCount(1, 1);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	InitialiseBrushesAndPens();
}

Map2DEditor::~Map2DEditor()
{
}

void Map2DEditor::SetColour(int c)
{
}

bool Map2DEditor::Save(const wxString& filename, Tilemap2D::Compression compression, int base)
{
	m_map->Save(filename.ToStdString(), compression);
	return true;
}

bool Map2DEditor::Open(const wxString& filename, Tilemap2D::Compression compression, int base)
{
	auto retval = m_map->Open(filename.ToStdString(), compression, base);
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
	return Open(data, Tilemap2D::NONE, width, height, base);
}

bool Map2DEditor::Open(const std::vector<uint8_t>& map, Tilemap2D::Compression compression, int width, int height, int base)
{
	auto retval = m_map->Open(map, width, height, compression, base);
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
		for (int x = 0; x < m_map->GetWidth(); ++x)
			for (int y = 0; y < m_map->GetHeight(); ++y)
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
	m_redraw_list.insert(tp.x + tp.y * m_map->GetWidth());
	Refresh(true);
}

void Map2DEditor::RedrawMapTile(int index)
{
	m_redraw_list.insert(index);
	Refresh(true);
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
	return std::shared_ptr<Palette>(&GetSelectedPalette());
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
		return Tile(-1);
	}
}

void Map2DEditor::SetTileAtPosition(const TilePosition& tp, const Tile& tile)
{
	m_map->SetTile(tile, tp.x, tp.y);
	RedrawMapTile(tp);
	FireEvent(EVT_MAP_CHANGE, std::to_string(ToIndex(tp)));
}

bool Map2DEditor::IsPositionValid(const TilePosition& tp) const
{
	bool valid = (tp.x >= 0 && tp.x < GetTilemapWidth());
	valid = valid && (tp.y >= 0 && tp.y < GetTilemapHeight());
	return valid;
}

wxCoord Map2DEditor::OnGetRowHeight(size_t row) const
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

wxCoord Map2DEditor::OnGetColumnWidth(size_t column) const
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
	wxPen pen = dc.GetPen();
	wxBrush brush = dc.GetBrush();

	pen.SetStyle(wxPENSTYLE_TRANSPARENT);
	brush.SetStyle(wxBRUSHSTYLE_SOLID);
	dc.SetPen(pen);

	auto tile_pixels = m_tileset->GetTileBGRA(tile, GetSelectedPalette());

	for (int i = 0; i < tile_pixels.size(); ++i)
	{
		int xx = x + (i % m_tileset->GetTileWidth()) * m_pixelsize;
		int yy = y + (i / m_tileset->GetTileWidth()) * m_pixelsize;
		brush.SetColour(wxColour(tile_pixels[i]));
		dc.SetBrush(brush);
		// Has alpha
		if ((tile_pixels[i] & 0xFF000000) > 0)
		{
			dc.DrawRectangle({ xx, yy, m_pixelsize, m_pixelsize });
		}
	}
}

bool Map2DEditor::DrawTileAtPosition(wxDC& dc, int x, int y)
{
	bool retval = false;
	wxPosition s = GetVisibleBegin();
	wxPosition e = GetVisibleEnd();

	int cellwidth = m_tileset->GetTileWidth() * m_pixelsize;
	int cellheight = m_tileset->GetTileHeight() * m_pixelsize;

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	if ((x >= s.GetCol()) && (x < e.GetCol()) &&
		(y >= s.GetRow()) && (y < e.GetRow()))
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
		dc.DrawRectangle({ x * cellwidth, y * cellheight, cellwidth, cellheight });
		auto t = m_map->GetTile(x, y);
		if (m_enablehover && (m_mode != Mode::SELECT) && IsHoverValid() && ToIndex({ x,y }) == m_hoveredtile)
		{
			// Preview when in draw mode
			t = m_drawtile;
		}
		DrawTile(dc, x * cellwidth, y * cellheight, t);
		if (m_enableborders)
		{
			dc.SetPen(*m_border_pen);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle({ x * cellwidth, y * cellheight, cellwidth + 1, cellheight + 1 });
		}
		if (m_enabletilenumbers)
		{
			auto label = wxString::Format("%03d%c%c%c", t.GetIndex(), t.Attributes().getAttribute(TileAttributes::ATTR_HFLIP) ? 'H' : ' ',
				t.Attributes().getAttribute(TileAttributes::ATTR_VFLIP) ? 'V' : ' ',
				t.Attributes().getAttribute(TileAttributes::ATTR_PRIORITY) ? 'P' : ' ');
			auto extent = dc.GetTextExtent(label);
			if ((extent.GetWidth() < cellwidth - 2) && (extent.GetHeight() < cellheight - 2))
			{
				dc.DrawText(label, { x * cellwidth + 2, y * cellheight + 2 });
			}
		}
		retval = true;
	}
	return retval;
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

void Map2DEditor::PaintBitmap(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();
	int sx = GetVisibleColumnsBegin() * m_tileset->GetTileWidth() * m_pixelsize;
	int sy = GetVisibleRowsBegin() * m_tileset->GetTileHeight() * m_pixelsize;

	int vX, vY, vW, vH;                 // Dimensions of client area in pixels
	wxRegionIterator upd(GetUpdateRegion()); // get the update rect list
	while (upd)
	{
		vX = upd.GetX();
		vY = upd.GetY();
		vW = upd.GetW();
		vH = upd.GetH();
		// Alternatively we can do this:
		// wxRect rect(upd.GetRect());
		// Repaint this rectangle
		dc.Blit(vX + sx, vY + sy, vW, vH, &m_memdc, vX + sx, vY + sy);
		upd++;
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
	m_redraw_all = true;
	wxVarHScrollHelper::RefreshAll();
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

Palette& Map2DEditor::GetSelectedPalette()
{
	return *m_active_palette->GetData();
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
		FireEvent(EVT_MAP_SELECT, std::to_string(m_selectedtile));
		m_selectedtile = tile;
		m_drawtile = tile;
		Refresh();
	}
}

void Map2DEditor::OnDraw(wxDC& dc)
{
	if (!m_tileset)
	{
		return;
	}
	int columns = m_map->GetWidth();
	int rows = m_map->GetHeight();
	int cellwidth = m_tileset->GetTileWidth() * m_pixelsize;
	int cellheight = m_tileset->GetTileHeight() * m_pixelsize;

	if (m_redraw_all == true)
	{
		m_bmp.Create(cellwidth * columns + 1, cellheight * rows + 1);
	}
	m_memdc.SelectObject(m_bmp);
	if (m_redraw_all == true)
	{
		m_memdc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
		m_memdc.Clear();
	}

	m_memdc.SetTextForeground(wxColour(255, 255, 255));
	m_memdc.SetTextBackground(wxColour(150, 150, 150));
	m_memdc.SetBackgroundMode(wxSOLID);

	if (m_pixelsize > 3)
	{
		m_border_pen->SetStyle(wxPENSTYLE_SOLID);
	}
	else
	{
		m_border_pen->SetStyle(wxPENSTYLE_TRANSPARENT);
	}

	m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
	if (m_redraw_all == true)
	{
		m_redraw_list.clear();
		for (int x = 0; x < m_map->GetWidth(); ++x)
			for (int y = 0; y < m_map->GetHeight(); ++y)
			{
				int i = x + y * m_map->GetWidth();
				if (!DrawTileAtPosition(m_memdc, x, y))
				{
					m_redraw_list.insert(i);
				}
			}
		m_redraw_all = false;
	}
	else
	{
		auto it = m_redraw_list.begin();
		while (it != m_redraw_list.end())
		{
			if ((*it >= 0) && (*it < (m_map->GetWidth() * m_map->GetHeight())))
			{
				int x = *it % m_map->GetWidth();
				int y = *it / m_map->GetWidth();
				if (DrawTileAtPosition(m_memdc, x, y))
				{
					m_redraw_list.erase(it++);
				}
				else
				{
					++it;
				}
			}
		}
	}

	DrawSelectionBorders(m_memdc);
	PaintBitmap(dc);
	m_memdc.SelectObject(wxNullBitmap);
}

void Map2DEditor::OnPaint(wxPaintEvent& evt)
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
		m_hoveredtile = sel;
		FireEvent(EVT_MAP_HOVER, std::to_string(m_hoveredtile));
		Refresh();
	}
	evt.Skip();
}

void Map2DEditor::OnMouseLeave(wxMouseEvent& evt)
{
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		m_redraw_list.insert(m_hoveredtile);
		m_hoveredtile = -1;
		FireEvent(EVT_MAP_HOVER, std::to_string(m_hoveredtile));
		Refresh();
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
	if (row > m_map->GetHeight())
	{
		return false;
	}
	auto pos = GetSelection();
	m_map->InsertRow(row, GetSelectedTile());
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

bool Map2DEditor::DeleteRow(int row)
{
	if (row >= m_map->GetHeight())
	{
		return false;
	}
	auto pos = GetSelection();
	m_map->DeleteRow(row);
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

bool Map2DEditor::InsertColumn(int column)
{
	if (column > m_map->GetWidth())
	{
		return false;
	}
	auto pos = GetSelection();
	m_map->InsertColumn(column, GetSelectedTile());
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

bool Map2DEditor::DeleteColumn(int column)
{
	if (column >= m_map->GetWidth())
	{
		return false;
	}
	auto pos = GetSelection();
	m_map->DeleteColumn(column);
	SetRowColumnCount(m_map->GetHeight(), m_map->GetWidth());
	SetSelection(pos);
	RedrawTiles();
	return true;
}

