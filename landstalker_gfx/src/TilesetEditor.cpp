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
		Refresh(false);
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
		m_bmp.Create(m_cellwidth * m_columns + 1, m_cellheight * m_rows + 1);
	}
	m_memdc.SelectObject(m_bmp);
	if (m_redraw_all == true)
	{
		m_memdc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
		m_memdc.Clear();
	}
	if (!m_tileset)
	{
		return;
	}
	m_buf.Resize(m_tileset->GetTileWidth() * m_columns, m_tileset->GetTileHeight() * m_rows);
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
		for (int i = 0; i < m_tileset->GetTileCount(); ++i)
		{
			if (!DrawTileAtPosition(m_memdc, i))
			{
				m_redraw_list.insert(i);
			}
		}
		m_redraw_all = false;
	}
	else
	{
		auto it = m_redraw_list.begin();
		while(it != m_redraw_list.end())
		{
			if ((*it >= 0) && (*it < m_tileset->GetTileCount()))
			{
				if (DrawTileAtPosition(m_memdc, *it))
				{
					it = m_redraw_list.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}
	}

	DrawSelectionBorders(m_memdc);
	PaintBitmap(dc);
	m_memdc.SelectObject(wxNullBitmap);
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
	if ((m_hoveredtile != -1) && (sel != m_hoveredtile))
	{
		m_redraw_list.insert(m_hoveredtile);
	}
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
		m_redraw_list.insert(m_hoveredtile);
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
			SetRowCount(m_rows);
			return true;
		}
		return false;
	}
	m_cellwidth = m_pixelsize * m_tileset->GetTileWidth();
	m_cellheight = m_pixelsize * m_tileset->GetTileHeight();
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

void TilesetEditor::DrawTile(wxDC& dc, int x, int y, int tile)
{
	wxPen pen = dc.GetPen();
	wxBrush brush = dc.GetBrush();

	pen.SetStyle(wxPENSTYLE_TRANSPARENT);
	brush.SetStyle(wxBRUSHSTYLE_SOLID);
	dc.SetPen(pen);

	auto tile_pixels = m_tileset->GetTileRGBA(tile, GetSelectedPalette());

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

bool TilesetEditor::DrawTileAtPosition(wxDC& dc, int pos)
{
	bool retval = false;
	int s = GetVisibleRowsBegin();
	int e = GetVisibleRowsEnd();


	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	auto x = pos % m_columns;
	auto y = pos / m_columns;
	m_buf.InsertTile(x * m_tileset->GetTileWidth(), y * m_tileset->GetTileHeight(), 0, pos, *m_tileset);
	if ((y >= s) && (y < e))
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
		dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight});
		DrawTile(dc, x * m_cellwidth, y * m_cellheight, pos);
		if (m_enableborders)
		{
			dc.SetPen(*m_border_pen);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth + 1, m_cellheight + 1 });
		}
		if (m_enabletilenumbers)
		{
			auto label = (wxString::Format("%03d", pos));
			auto extent = dc.GetTextExtent(label);
			if ((extent.GetWidth() < m_cellwidth - 2) && (extent.GetHeight() < m_cellheight - 2))
			{
				dc.DrawText(label, { x * m_cellwidth + 2, y * m_cellheight + 2 });
			}
		}
		retval = true;
	}
	return retval;
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

void TilesetEditor::PaintBitmap(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();
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
		dc.Blit(vX, vY + sy, vW, vH, &m_memdc, vX, vY + sy);
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
		ForceRedraw();
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
		m_redraw_list.insert(m_selectedtile);
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
		m_redraw_list.insert(m_selectedtile);
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
		ForceRedraw();
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
			m_tileset->SwapTile(m_pendingswap, tile);
			m_pendingswap = -1;
		}
		m_redraw_list.insert(m_selectedtile);
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

void TilesetEditor::InsertTileAtEnd()
{
	m_tileset->InsertTilesBefore(m_tileset->GetTileCount());
	m_redraw_list.insert(m_tileset->GetTileCount() - 1);
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
