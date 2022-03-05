#include "TilesetEditor.h"

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>

#include <fstream>
#include "LZ77.h"
#include "Utils.h"

wxBEGIN_EVENT_TABLE(TilesetEditor, wxHVScrolledWindow)
EVT_PAINT(TilesetEditor::OnPaint)
EVT_SIZE(TilesetEditor::OnSize)
EVT_LEFT_DOWN(TilesetEditor::OnMouseDown)
EVT_LEFT_DCLICK(TilesetEditor::OnDoubleClick)
EVT_MOTION(TilesetEditor::OnMouseMove)
EVT_LEAVE_WINDOW(TilesetEditor::OnMouseLeave)
EVT_SET_FOCUS(TilesetEditor::OnTilesetFocus)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_TILESET_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESET_ACTIVATE, wxCommandEvent);

TilesetEditor::TilesetEditor(wxWindow* parent)
	: wxVScrolledWindow(parent, wxID_ANY),
	m_pixelsize(8),
	m_selectable(false),
	m_selectedtile(-1),
	m_hoveredtile(-1),
	m_pendingswap(-1),
	m_tilebase(0),
	m_tilewidth(0),
	m_tileheight(0),
	m_columns(0),
	m_rows(0),
	m_redraw_all(true),
	m_default_palette(),
	m_enablealpha(true),
	m_enableborders(true),
	m_enabletilenumbers(false),
	m_enableselection(true),
	m_enablehover(true)
{
	SetRowCount(m_rows);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	InitialiseBrushesAndPens();
}

TilesetEditor::TilesetEditor(wxWindow* parent, std::shared_ptr<Tileset> tileset, std::shared_ptr<std::map<std::string, Palette>> palettes)
	: TilesetEditor(parent)
{
	m_palettes = palettes;
	m_tileset = tileset;
}

TilesetEditor::~TilesetEditor()
{
	delete m_alpha_brush;
	delete m_border_pen;
	delete m_selected_border_pen;
	delete m_highlighted_border_pen;
	delete m_highlighted_brush;
}

void TilesetEditor::SetColour(int c)
{
}

void TilesetEditor::SetPalettes(std::shared_ptr<std::map<std::string, Palette>> palettes)
{
	m_palettes = palettes;
}

std::shared_ptr<Tileset> TilesetEditor::GetTileset()
{
	return m_tileset;
}

bool TilesetEditor::Save(wxString filename, bool compressed)
{
	return m_tileset->Save(filename.ToStdString(), compressed);
}

bool TilesetEditor::Open(wxString filename, int tile_width, int tile_height, int tile_bitdepth)
{
	return false;
}

bool TilesetEditor::Open(std::vector<uint8_t>& pixels, bool uses_compression, int tile_width, int tile_height, int tile_bitdepth)
{
	bool retval = false;
	try
	{
		m_tileset = std::make_shared<Tileset>(pixels, uses_compression, tile_width, tile_height, tile_bitdepth);

		UpdateRowCount();
		ForceRedraw();
		retval = true;
	}
	catch (std::exception& e)
	{
		Debug(e.what());
	}
	return retval;
}

bool TilesetEditor::New(int r, int c)
{
	return false;
}

void TilesetEditor::RedrawTiles(int index)
{
	if ((index < 0) || (index >= m_tileset->GetTileCount()))
	{
		ForceRedraw();
	}
	else
	{
		m_redraw_list.insert(index);
		Refresh(true);
	}
}

wxCoord TilesetEditor::OnGetRowHeight(size_t row) const
{
	return wxCoord(m_pixelsize * m_tileset->GetTileHeight());
}

void TilesetEditor::OnDraw(wxDC& dc)
{
	if (m_redraw_all == true)
	{
		m_bg_bmp.Create(m_cellwidth * m_columns + 1, m_cellheight * m_rows + 1);
		m_bmp.Create(m_cellwidth * m_columns + 1, m_cellheight * m_rows + 1);
		m_buf.Resize(m_tilewidth * m_columns, m_tileheight * m_rows);
	}
	wxMemoryDC background(m_bg_bmp);
	wxMemoryDC memdc(m_bmp);
	if (m_redraw_all == true)
	{
		background.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
		background.Clear();
	}
	if (m_tileset != nullptr)
	{

		if (m_redraw_all)
		{
			DrawAllTiles(background);
		}
		else if (!m_redraw_list.empty())
		{
			DrawTileList(background);
		}
		PaintBitmap(background, memdc);
		DrawGrid(memdc);
		DrawSelectionBorders(memdc);
	}
	else
	{
		memdc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
		memdc.Clear();
	}
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();
	PaintBitmap(memdc, dc);
	background.SelectObject(wxNullBitmap);
	memdc.SelectObject(wxNullBitmap);
}

void TilesetEditor::OnPaint(wxPaintEvent& evt)
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

void TilesetEditor::OnMouseDown(wxMouseEvent& evt)
{
	if (!m_enableselection) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	SelectTile(sel);
	evt.Skip();
}

void TilesetEditor::OnDoubleClick(wxMouseEvent& evt)
{
	int sel = ConvertXYToTile(evt.GetPosition());
	if (sel != -1)
	{
		EditTile(sel);
	}
	evt.Skip();
}

void TilesetEditor::OnMouseMove(wxMouseEvent& evt)
{
	if (!m_enablehover) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	if (sel != m_hoveredtile)
	{
		m_hoveredtile = sel;
		FireEvent(EVT_TILESET_HOVER, std::to_string(m_hoveredtile));
		Refresh();
	}
	evt.Skip();
}

void TilesetEditor::OnMouseLeave(wxMouseEvent& evt)
{
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		m_hoveredtile = -1;
		FireEvent(EVT_TILESET_HOVER, std::to_string(m_hoveredtile));
		Refresh();
	}
	evt.Skip();
}

void TilesetEditor::OnTilesetFocus(wxFocusEvent& evt)
{
	FireEvent(EVT_TILESET_ACTIVATE, "");
	evt.Skip();
}

int TilesetEditor::ConvertXYToTile(const wxPoint& point)
{
	int s = GetVisibleRowsBegin();
	int x = point.x / (m_pixelsize * m_tileset->GetTileWidth());
	int y = s + point.y / (m_pixelsize * m_tileset->GetTileHeight());
	int sel = x + y * m_columns;
	if ((sel >= m_tileset->GetTileCount()) || (x < 0) || (y < 0) || (x >= m_columns))
	{
		sel = -1;
	}
	return sel;
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

void TilesetEditor::DrawAllTiles(wxDC& dest)
{
	int x = 0;
	int y = 0;
	dest.SetBrush(*wxTRANSPARENT_BRUSH);
	dest.SetPen(*wxTRANSPARENT_PEN);
	dest.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
	for (int i = 0; i < m_tileset->GetTileCount(); ++i)
	{
		dest.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight });
		m_buf.InsertTile(x * m_tilewidth, y * m_tileheight, 0, i, *m_tileset);
		x++;
		if (x >= m_columns)
		{
			x = 0;
			y++;
		}
	}
	auto img = m_buf.MakeImage({ m_palettes->find(m_selected_palette)->second }, true);
	if (m_tiles_bmp)
	{
		delete m_tiles_bmp;
	}
	m_tiles_bmp = new wxBitmap(img);
	wxMemoryDC tiles(*m_tiles_bmp);
	dest.StretchBlit({ 0,0 },
		{ img.GetWidth() * m_pixelsize, img.GetHeight() * m_pixelsize },
		&tiles, { 0,0 }, { img.GetWidth(), img.GetHeight() }, wxCOPY, true, { 0,0 });
	m_redraw_all = false;
	m_redraw_list.clear();
}

void TilesetEditor::DrawTileList(wxDC& dest)
{
	dest.SetBrush(*wxTRANSPARENT_BRUSH);
	dest.SetPen(*wxTRANSPARENT_PEN);
	dest.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
	int s = GetVisibleRowsBegin();
	int e = GetVisibleRowsEnd();
	auto it = m_redraw_list.begin();
	while (it != m_redraw_list.end())
	{
		if ((*it >= 0) && (*it < m_tileset->GetTileCount()))
		{
			const int x = *it % m_columns;
			const int y = *it / m_columns;
			if ((y >= s) && (y <= e))
			{
				m_buf.InsertTile(x * m_tilewidth, y * m_tileheight, 0, *it, *m_tileset, false);
			}
		}
		it++;
	}
	auto img = m_buf.MakeImage({ m_palettes->find(m_selected_palette)->second }, true);
	if (m_tiles_bmp)
	{
		delete m_tiles_bmp;
	}
	m_tiles_bmp = new wxBitmap(img);
	wxMemoryDC tiles(*m_tiles_bmp);

	it = m_redraw_list.begin();
	while (it != m_redraw_list.end())
	{
		if ((*it >= 0) && (*it < m_tileset->GetTileCount()))
		{
			const int x = *it % m_columns;
			const int y = *it / m_columns;
			if ((y >= s) && (y <= e))
			{
				dest.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight });
				dest.StretchBlit({ x * m_cellwidth, y * m_cellheight },
					{ m_cellwidth, m_cellheight },
					&tiles,
					{ x * m_tilewidth, y * m_tileheight },
					{ m_tilewidth, m_tileheight },
					wxCOPY, true, { x * m_tilewidth, y * m_tileheight });
				it = m_redraw_list.erase(it);
			}
			else
			{
				it++;
			}
		}
		else
		{
			it = m_redraw_list.erase(it);
		}
	}
}

void TilesetEditor::DrawGrid(wxDC& dest)
{
	int x = 0;
	int y = 0;
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

	for (int i = 0; i < m_tileset->GetTileCount(); ++i)
	{
		int s = GetVisibleRowsBegin();
		int e = GetVisibleRowsEnd();
		if (y >= s && y <= e)
		{
			if (m_enableborders)
			{
				dest.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth + 1, m_cellheight + 1 });
			}
			if (m_enabletilenumbers)
			{
				auto label = (wxString::Format("%03d", i));
				auto extent = dest.GetTextExtent(label);
				if ((extent.GetWidth() < m_cellwidth - 2) && (extent.GetHeight() < m_cellheight - 2))
				{
					dest.DrawText(label, { x * m_cellwidth + 2, y * m_cellheight + 2 });
				}
			}
		}
		x++;
		if (x >= m_columns)
		{
			x = 0;
			y++;
		}
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

void TilesetEditor::PaintBitmap(wxDC& src, wxDC& dst)
{
	int sy = GetVisibleRowsBegin() * m_cellheight;
	
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
		dst.Blit(vX, vY + sy, vW, vH, &src, vX, vY + sy);
		upd++;
	}
}

void TilesetEditor::InitialiseBrushesAndPens()
{
	m_alpha_brush = new wxBrush(*wxBLACK);
	wxBitmap* stipple = new wxBitmap(6, 6);
	wxMemoryDC* imagememDC = new wxMemoryDC();
	imagememDC->SelectObject(*stipple);
	imagememDC->SetBackground(*wxGREY_BRUSH);
	imagememDC->Clear();
	imagememDC->SetBrush(*wxLIGHT_GREY_BRUSH);
	imagememDC->SetPen(*wxTRANSPARENT_PEN);
	imagememDC->DrawRectangle(0, 0, 3, 3);
	imagememDC->DrawRectangle(3, 3, 5, 5);
	imagememDC->SelectObject(wxNullBitmap);
	m_alpha_brush->SetStyle(wxBRUSHSTYLE_STIPPLE_MASK);
	m_alpha_brush->SetStipple(*stipple);
	delete stipple;
	delete imagememDC;
	m_border_pen = new wxPen(*wxMEDIUM_GREY_PEN);
	m_selected_border_pen = new wxPen(*wxRED_PEN);
	m_highlighted_border_pen = new wxPen(*wxBLUE_PEN);
	m_highlighted_brush = new wxBrush(*wxTRANSPARENT_BRUSH);
}

void TilesetEditor::ForceRedraw()
{
	m_redraw_all = true;
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

const Palette& TilesetEditor::GetSelectedPalette()
{
	if (m_palettes)
	{
		auto it = m_palettes->find(m_selected_palette);
		if (it != m_palettes->end())
		{
			return it->second;
		}
	}
	return m_default_palette;
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
	if (m_selected_palette != name)
	{
		m_selected_palette = name;
		ForceRedraw();
	}
}

std::string TilesetEditor::GetActivePalette() const
{
	return m_selected_palette;
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

bool TilesetEditor::IsSelectionValid() const
{
	return ((m_selectedtile != -1) && (m_selectedtile < m_tileset->GetTileCount()));
}

Tile TilesetEditor::GetSelectedTile() const
{
	return Tile(m_selectedtile);
}

bool TilesetEditor::IsHoverValid() const
{
	return m_hoveredtile != -1;
}

Tile TilesetEditor::GetHoveredTile() const
{
	return Tile(m_hoveredtile);
}

void TilesetEditor::SelectTile(int tile)
{
	if ((m_selectedtile != -1) && (tile != m_selectedtile))
	{
		if (m_pendingswap)
		{
			m_redraw_list.insert(tile);
			m_redraw_list.insert(m_selectedtile);
			m_tileset->SwapTile(m_pendingswap, tile);
			m_pendingswap = -1;
			Refresh();
		}
	}
	if (tile != m_selectedtile)
	{
		m_selectedtile = tile;
		Refresh();
	}
	FireEvent(EVT_TILESET_SELECT, std::to_string(m_selectedtile));
}

void TilesetEditor::InsertTileBefore(const Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_tileset->InsertTilesBefore(tile.GetIndex());
		for (int i = tile.GetIndex(); i < m_tileset->GetTileCount(); ++i)
		{
			m_redraw_list.insert(i);
		}
		SelectTile(m_selectedtile + 1);
		UpdateRowCount();
		Refresh();
		FireEvent(EVT_TILESET_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void TilesetEditor::InsertTileAfter(const Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_tileset->InsertTilesBefore(tile.GetIndex() + 1);
		for (int i = tile.GetIndex(); i < m_tileset->GetTileCount(); ++i)
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
	m_tileset->InsertTilesBefore(m_tileset->GetTileCount(), count);
	UpdateRowCount();
	Refresh();
	FireEvent(EVT_TILESET_CHANGE, "");
}

void TilesetEditor::DeleteTileAt(const Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_tileset->DeleteTile(tile.GetIndex());
		if (!IsSelectionValid())
		{
			SelectTile(m_tileset->GetTileCount() - 1);
		}
		ForceRedraw();
		FireEvent(EVT_TILESET_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void TilesetEditor::CutTile(const Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_clipboard = m_tileset->GetTile(tile);
		DeleteTileAt(tile);
	}
}

void TilesetEditor::CopyTile(const Tile& tile) const
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_clipboard = m_tileset->GetTile(tile);
	}
}

void TilesetEditor::PasteTile(const Tile& tile)
{
	if ((tile.GetIndex() <= m_tileset->GetTileCount()) && !IsClipboardEmpty())
	{
		m_tileset->GetTilePixels(tile.GetTileValue()) = m_clipboard;
		m_redraw_list.insert(tile.GetTileValue());
		Refresh();
		FireEvent(EVT_TILESET_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void TilesetEditor::SwapTile(const Tile& tile)
{
	if (tile.GetIndex() <= m_tileset->GetTileCount())
	{
		m_pendingswap = tile.GetIndex();
	}
}

void TilesetEditor::EditTile(const Tile& tile)
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
