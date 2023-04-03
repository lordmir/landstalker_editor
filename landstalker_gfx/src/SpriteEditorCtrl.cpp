#include "SpriteEditorCtrl.h"

#include <wx/wx.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>

#include <fstream>
#include "LZ77.h"

wxBEGIN_EVENT_TABLE(SpriteEditorCtrl, wxHVScrolledWindow)
EVT_PAINT(SpriteEditorCtrl::OnPaint)
EVT_SIZE(SpriteEditorCtrl::OnSize)
EVT_LEFT_DOWN(SpriteEditorCtrl::OnMouseDown)
EVT_LEFT_DCLICK(SpriteEditorCtrl::OnDoubleClick)
EVT_MOTION(SpriteEditorCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(SpriteEditorCtrl::OnMouseLeave)
EVT_SET_FOCUS(SpriteEditorCtrl::OnTilesetFocus)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_SPRITE_FRAME_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_HOVER, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_EDIT_REQUEST, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SPRITE_FRAME_ACTIVATE, wxCommandEvent);

static const uint8_t UNKNOWN_TILE[32] = {
	0x11, 0xFF, 0xFF, 0x11, 0x1F, 0xF1, 0x1F, 0xF1, 0x11, 0x11, 0x1F, 0xF1, 0x11, 0x11, 0xFF, 0x11,
	0x11, 0x1F, 0xF1, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F, 0xF1, 0x11, 0x11, 0x11, 0x11, 0x11 };

SpriteEditorCtrl::SpriteEditorCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent, wxID_ANY),
	m_pixelsize(8),
	m_selectable(false),
	m_selectedtile(-1),
	m_hoveredtile(-1),
	m_pendingswap(-1),
	m_tilebase(0),
	m_columns(36),
	m_rows(36),
	m_redraw_all(true),
	m_enablealpha(true),
	m_enableborders(true),
	m_enabletilenumbers(false),
	m_enableselection(true),
	m_enablehover(true)
{

	SetRowColumnCount(m_rows, m_columns);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	InitialiseBrushesAndPens();
}

SpriteEditorCtrl::~SpriteEditorCtrl()
{
	delete m_alpha_brush;
	delete m_border_pen;
	delete m_selected_border_pen;
	delete m_highlighted_border_pen;
	delete m_highlighted_brush;
}

void SpriteEditorCtrl::SetColour(int c)
{
}

bool SpriteEditorCtrl::Save(wxString filename, bool compressed)
{
	return m_sprite->Save(filename.ToStdString(), compressed);
}

bool SpriteEditorCtrl::Open(wxString filename, int tile_width, int tile_height, int tile_bitdepth)
{
	return m_sprite->Open(filename.ToStdString());
}

bool SpriteEditorCtrl::Open(std::vector<uint8_t>& pixels, bool uses_compression, int tile_width, int tile_height, int tile_bitdepth)
{
	m_sprite->SetBits(pixels);
	return true;
}

bool SpriteEditorCtrl::Open(std::shared_ptr<SpriteFrame> frame, std::shared_ptr<Palette> pal)
{
	m_sprite = frame;
	m_pal = pal;
	ForceRedraw();
	return false;
}

bool SpriteEditorCtrl::New(int r, int c)
{
	return false;
}

void SpriteEditorCtrl::RedrawTiles(int index)
{
	if ((index < 0) || (index >= m_sprite->GetTileCount()))
	{
		ForceRedraw();
	}
	else
	{
		m_redraw_list.insert(index);
		Refresh(false);
	}
}

wxCoord SpriteEditorCtrl::OnGetRowHeight(size_t row) const
{
	if (m_sprite)
	{
		return wxCoord(m_pixelsize * m_sprite->GetTileHeight());
	}
	else
	{
		return 0;
	}
}

wxCoord SpriteEditorCtrl::OnGetColumnWidth(size_t column) const
{
	if (m_sprite)
	{
		return wxCoord(m_pixelsize * m_sprite->GetTileWidth());
	}
	else
	{
		return 0;
	}
}

void SpriteEditorCtrl::OnDraw(wxDC& dc)
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
	if (m_sprite != nullptr)
	{
		if (m_redraw_all == true)
		{
			m_redraw_list.clear();
			for (int i = 0; i < m_sprite->GetTileCount(); ++i)
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
			while (it != m_redraw_list.end())
			{
				if ((*it >= 0) && (*it < m_sprite->GetTileCount()))
				{
					if (DrawTileAtPosition(m_memdc, *it))
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

		m_memdc.SetBrush(*wxTRANSPARENT_BRUSH);
		m_memdc.SetPen(*wxGREEN_PEN);
		m_memdc.DrawLine(SpriteToScreenXY({ -10, 0 }), SpriteToScreenXY({ 10, 0 }));
		m_memdc.DrawLine(SpriteToScreenXY({ 0, -10 }), SpriteToScreenXY({ 0, 10 }));
		m_memdc.SetPen(*wxRED_PEN);
		for (std::size_t i = 0; i < m_sprite->GetSubSpriteCount(); ++i)
		{
			const auto& s = m_sprite->GetSubSprite(i);
			m_memdc.DrawRectangle(SpriteToScreenXY({ s.x, s.y }), { static_cast<int>(s.w * m_sprite->GetTileWidth() * m_pixelsize), static_cast<int>(s.h * m_sprite->GetTileHeight() * m_pixelsize) });
		}
	}

	PaintBitmap(dc);
	m_memdc.SelectObject(wxNullBitmap);
}

void SpriteEditorCtrl::OnPaint(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void SpriteEditorCtrl::OnSize(wxSizeEvent& evt)
{
	this->GetClientSize(&m_ctrlwidth, &m_ctrlheight);
	if (UpdateRowCount())
	{
		ForceRedraw();
	}
	wxVarHScrollHelper::HandleOnSize(evt);
	wxVarVScrollHelper::HandleOnSize(evt);
	ScrollToRow(std::max<int>(0U, (m_rows - GetVisibleRowsEnd() + GetVisibleRowsBegin()) / 2 - 4));
	ScrollToColumn(1 + std::max<int>(0U, (m_columns - GetVisibleColumnsEnd() + GetVisibleColumnsBegin()) / 2 - 2));
	Refresh(false);
}

void SpriteEditorCtrl::OnMouseDown(wxMouseEvent& evt)
{
	if (!m_enableselection) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	SelectTile(sel);
	evt.Skip();
}

void SpriteEditorCtrl::OnDoubleClick(wxMouseEvent& evt)
{
	int sel = ConvertXYToTile(evt.GetPosition());
	if (sel != -1)
	{
		EditTile(sel);
	}
	evt.Skip();
}

void SpriteEditorCtrl::OnMouseMove(wxMouseEvent& evt)
{
	if (!m_enablehover) return;
	if (m_sprite == nullptr) return;
	int sel = ConvertXYToTile(evt.GetPosition());
	if ((m_hoveredtile != -1) && (sel != m_hoveredtile))
	{
		m_redraw_list.insert(m_hoveredtile);
	}
	if (sel != m_hoveredtile)
	{
		m_hoveredtile = sel;
		FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		Refresh();
	}
	evt.Skip();
}

void SpriteEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
	if (!m_enablehover) return;
	if (m_hoveredtile != -1)
	{
		m_redraw_list.insert(m_hoveredtile);
		m_hoveredtile = -1;
		FireEvent(EVT_SPRITE_FRAME_HOVER, std::to_string(m_hoveredtile));
		Refresh();
	}
	evt.Skip();
}

void SpriteEditorCtrl::OnTilesetFocus(wxFocusEvent& evt)
{
	FireEvent(EVT_SPRITE_FRAME_ACTIVATE, "");
	evt.Skip();
}

int SpriteEditorCtrl::ConvertXYToTile(const wxPoint& point)
{
	int s = GetVisibleRowsBegin();
	int x = point.x / (m_pixelsize * m_sprite->GetTileWidth());
	int y = s + point.y / (m_pixelsize * m_sprite->GetTileHeight());
	int sel = x + y * m_columns;
	if ((sel >= m_sprite->GetTileCount()) || (x < 0) || (y < 0) || (x >= m_columns))
	{
		sel = -1;
	}
	return sel;
}

wxPoint SpriteEditorCtrl::SpriteToScreenXY(wxPoint sprite)
{
	return wxPoint((sprite.x + 0x80) * m_pixelsize, (sprite.y + 0x80) * m_pixelsize);
}

wxPoint SpriteEditorCtrl::ScreenToSpriteXY(wxPoint screen)
{
	return wxPoint((-screen.x - 0x80) / m_pixelsize, (-screen.y - 0x80) / m_pixelsize);
}

bool SpriteEditorCtrl::UpdateRowCount()
{
	if (m_sprite == nullptr)
	{
		return false;
	}
	m_cellwidth = m_pixelsize * m_sprite->GetTileWidth();
	m_cellheight = m_pixelsize * m_sprite->GetTileHeight();
	return false;
}

void SpriteEditorCtrl::DrawTile(wxDC& dc, int x, int y, int tile)
{
	wxPen pen = dc.GetPen();
	wxBrush brush = dc.GetBrush();

	pen.SetStyle(wxPENSTYLE_TRANSPARENT);
	brush.SetStyle(wxBRUSHSTYLE_SOLID);
	dc.SetPen(pen);

	auto tile_bytes = m_sprite->GetTile(tile);
	const auto& pal = GetSelectedPalette();
	std::vector<uint32_t> tile_pixels;
	for (const auto& b : tile_bytes)
	{
		tile_pixels.push_back(pal.getBGRA(b));
	}

	for (int i = 0; i < tile_pixels.size(); ++i)
	{
		int xx = x + (i % m_sprite->GetTileWidth()) * m_pixelsize;
		int yy = y + (i / m_sprite->GetTileWidth()) * m_pixelsize;
		brush.SetColour(wxColour(tile_pixels[i]));
		dc.SetBrush(brush);
		// Has alpha
		if ((tile_pixels[i] & 0xFF000000) > 0)
		{
			dc.DrawRectangle({ xx, yy, m_pixelsize, m_pixelsize });
		}
	}

}

bool SpriteEditorCtrl::DrawTileAtPosition(wxDC& dc, int pos)
{
	bool retval = false;
	int s = GetVisibleRowsBegin();
	int e = GetVisibleRowsEnd();

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	auto p = m_sprite->GetTilePosition(pos);
	auto ss = SpriteToScreenXY({ p.first, p.second });
	auto x = ss.x / m_cellwidth;
	auto y = ss.y / m_cellheight;
	if ((y >= s) && (y < e))
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(m_enablealpha ? *m_alpha_brush : *wxBLACK_BRUSH);
		dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight });
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

void SpriteEditorCtrl::DrawSelectionBorders(wxDC& dc)
{
	if (m_hoveredtile != -1)
	{
		auto x = m_hoveredtile % m_columns;
		auto y = m_hoveredtile / m_columns;
		dc.SetBrush(*m_highlighted_brush);
		dc.SetPen(*m_highlighted_border_pen);
		if (m_hoveredtile == m_selectedtile)
		{
			dc.DrawRectangle({ x * m_cellwidth + 1, y * m_cellheight + 1, m_cellwidth - 2, m_cellheight - 2 });
		}
		else
		{
			dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight });
		}
	}
	if (m_selectedtile != -1)
	{
		auto x = m_selectedtile % m_columns;
		auto y = m_selectedtile / m_columns;
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*m_selected_border_pen);
		dc.DrawRectangle({ x * m_cellwidth, y * m_cellheight, m_cellwidth, m_cellheight });
	}
}

void SpriteEditorCtrl::PaintBitmap(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();
	int sx = GetVisibleColumnsBegin() * m_sprite->GetTileWidth() * m_pixelsize;
	int sy = GetVisibleRowsBegin() * m_sprite->GetTileHeight() * m_pixelsize;

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

void SpriteEditorCtrl::InitialiseBrushesAndPens()
{
	m_alpha_brush = new wxBrush();
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

void SpriteEditorCtrl::ForceRedraw()
{
	m_redraw_all = true;
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

const Palette& SpriteEditorCtrl::GetSelectedPalette()
{
	return *m_pal;
}

void SpriteEditorCtrl::SetPixelSize(int n)
{
	m_pixelsize = n;
	UpdateRowCount();
	ForceRedraw();
}

int SpriteEditorCtrl::GetPixelSize() const
{
	return m_pixelsize;
}

int SpriteEditorCtrl::GetTilemapSize() const
{
	return m_sprite->GetTileCount();
}

bool SpriteEditorCtrl::GetCompressed() const
{
	return m_sprite->GetCompressed();
}

void SpriteEditorCtrl::SetActivePalette(const std::shared_ptr<Palette> pal)
{
	m_pal = pal;
	ForceRedraw();
}

bool SpriteEditorCtrl::GetTileNumbersEnabled() const
{
	return m_enabletilenumbers;
}

void SpriteEditorCtrl::SetTileNumbersEnabled(bool enabled)
{
	if (enabled != m_enabletilenumbers)
	{
		m_enabletilenumbers = enabled;
		ForceRedraw();
	}
}

bool SpriteEditorCtrl::GetSelectionEnabled() const
{
	return m_enableselection;
}

void SpriteEditorCtrl::SetSelectionEnabled(bool enabled)
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

bool SpriteEditorCtrl::GetHoverEnabled() const
{
	return m_enablehover;
}

void SpriteEditorCtrl::SetHoverEnabled(bool enabled)
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

bool SpriteEditorCtrl::GetAlphaEnabled() const
{
	return m_enablealpha;
}

void SpriteEditorCtrl::SetAlphaEnabled(bool enabled)
{
	if (m_enablealpha != enabled)
	{
		m_enablealpha = enabled;
		ForceRedraw();
	}
}

bool SpriteEditorCtrl::GetBordersEnabled() const
{
	return m_enableborders;
}

void SpriteEditorCtrl::SetBordersEnabled(bool enabled)
{
	if (m_enableborders != enabled)
	{
		m_enableborders = enabled;
		ForceRedraw();
	}
}

bool SpriteEditorCtrl::IsSelectionValid() const
{
	return ((m_selectedtile != -1) && (m_selectedtile < m_sprite->GetTileCount()));
}

Tile SpriteEditorCtrl::GetSelectedTile() const
{
	return Tile(m_selectedtile);
}

bool SpriteEditorCtrl::IsHoverValid() const
{
	return m_hoveredtile != -1;
}

Tile SpriteEditorCtrl::GetHoveredTile() const
{
	return Tile(m_hoveredtile);
}

void SpriteEditorCtrl::SelectTile(int tile)
{
	if ((m_selectedtile != -1) && (tile != m_selectedtile))
	{
		if (m_pendingswap)
		{
			m_redraw_list.insert(tile);
			m_sprite->GetTileset()->SwapTile(m_pendingswap, tile);
			m_pendingswap = -1;
		}
		m_redraw_list.insert(m_selectedtile);
	}
	if (tile != m_selectedtile)
	{
		FireEvent(EVT_SPRITE_FRAME_SELECT, std::to_string(m_selectedtile));
		m_selectedtile = tile;
		Refresh();
	}
}

void SpriteEditorCtrl::InsertTileBefore(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_sprite->GetTileset()->InsertTilesBefore(tile.GetIndex());
		for (int i = tile.GetIndex(); i < m_sprite->GetTileCount(); ++i)
		{
			m_redraw_list.insert(i);
		}
		SelectTile(m_selectedtile + 1);
		UpdateRowCount();
		Refresh();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteEditorCtrl::InsertTileAfter(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_sprite->GetTileset()->InsertTilesBefore(tile.GetIndex() + 1);
		for (int i = tile.GetIndex(); i < m_sprite->GetTileCount(); ++i)
		{
			m_redraw_list.insert(i);
		}
		UpdateRowCount();
		Refresh();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteEditorCtrl::InsertTileAtEnd()
{
	m_sprite->GetTileset()->InsertTilesBefore(m_sprite->GetTileCount());
	m_redraw_list.insert(m_sprite->GetTileCount() - 1);
	UpdateRowCount();
	Refresh();
	FireEvent(EVT_SPRITE_FRAME_CHANGE, "");
}

void SpriteEditorCtrl::DeleteTileAt(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_sprite->GetTileset()->DeleteTile(tile.GetIndex());
		if (!IsSelectionValid())
		{
			SelectTile(m_sprite->GetTileCount() - 1);
		}
		ForceRedraw();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteEditorCtrl::CutTile(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_clipboard = m_sprite->GetTile(tile);
		DeleteTileAt(tile);
	}
}

void SpriteEditorCtrl::CopyTile(const Tile& tile) const
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_clipboard = m_sprite->GetTile(tile);
	}
}

void SpriteEditorCtrl::PasteTile(const Tile& tile)
{
	if ((tile.GetIndex() <= m_sprite->GetTileCount()) && !IsClipboardEmpty())
	{
		m_sprite->GetTilePixels(tile.GetTileValue()) = m_clipboard;
		m_redraw_list.insert(tile.GetTileValue());
		Refresh();
		FireEvent(EVT_SPRITE_FRAME_CHANGE, std::to_string(tile.GetTileValue()));
	}
}

void SpriteEditorCtrl::SwapTile(const Tile& tile)
{
	if (tile.GetIndex() <= m_sprite->GetTileCount())
	{
		m_pendingswap = tile.GetIndex();
	}
}

void SpriteEditorCtrl::EditTile(const Tile& tile)
{
	if (tile.GetIndex() < m_sprite->GetTileCount())
	{
		FireEvent(EVT_SPRITE_FRAME_EDIT_REQUEST, std::to_string(tile.GetIndex()));
	}
}

bool SpriteEditorCtrl::IsClipboardEmpty() const
{
	return m_clipboard.empty();
}


void SpriteEditorCtrl::FireEvent(const wxEventType& e, const std::string& data)
{
	wxCommandEvent evt(e);
	evt.SetString(data);
	evt.SetClientData(&m_sprite);
	wxPostEvent(this->GetParent(), evt);
}
