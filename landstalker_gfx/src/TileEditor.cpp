#include "TileEditor.h"
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(TileEditor, wxHVScrolledWindow)
EVT_PAINT(TileEditor::OnPaint)
EVT_SIZE(TileEditor::OnSize)
EVT_LEFT_DOWN(TileEditor::OnMouseUpDown)
EVT_RIGHT_DOWN(TileEditor::OnMouseUpDown)
EVT_LEFT_UP(TileEditor::OnMouseUpDown)
EVT_RIGHT_UP(TileEditor::OnMouseUpDown)
EVT_MOTION(TileEditor::OnMouseMove)
EVT_LEAVE_WINDOW(TileEditor::OnMouseLeave)
EVT_ENTER_WINDOW(TileEditor::OnMouseEnter)
EVT_SET_FOCUS(TileEditor::OnTilesetFocus)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(EVT_TILE_CHANGE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILE_ACTIVATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILE_PIXEL_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILE_PIXEL_HOVER, wxCommandEvent);

TileEditor::TileEditor(wxWindow* parent)
	: wxHVScrolledWindow(parent, wxID_ANY),
	  m_tileset(nullptr),
	  m_tile(0),
	  m_pixelsize(16),
	  m_primary_colour(1),
	  m_secondary_colour(0),
	  m_selectedpixel{ -1, -1 },
	  m_hoveredpixel{ -1, -1 },
	  m_enableborders(true),
	  m_enableedit(true),
	  m_enablealpha(true),
	  m_secondary_active(false),
	  m_drawing(false),
	  m_gd(nullptr)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	SetRowColumnCount(0, 0);

	InitialiseBrushesAndPens();
}

TileEditor::~TileEditor()
{
	delete m_alpha_brush;
	delete m_border_pen;
	delete m_selected_border_pen;
	delete m_highlighted_border_pen;
	delete m_highlighted_brush;
}

void TileEditor::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	if (m_gd == nullptr)
	{
		m_tileset = nullptr;
		m_tile.SetTileValue(0);
		m_selected_palette = nullptr;
		m_selected_palette_entry = nullptr;
		m_selected_palette_name = "";
	}
}

void TileEditor::SetTileset(std::shared_ptr<Tileset> tileset)
{
	m_tileset = tileset;
	m_pixels = m_tileset->GetTilePixels(m_tile.GetIndex());
	SetRowColumnCount(m_tileset->GetTileHeight(), m_tileset->GetTileWidth());
	AutoSize();
	wxVarHScrollHelper::RefreshAll();
	wxVarVScrollHelper::RefreshAll();
	Redraw();
}

void TileEditor::SetTile(const Tile& tile)
{
	m_tile = tile;
	if (m_tileset != nullptr)
	{
		m_pixels = m_tileset->GetTilePixels(tile.GetIndex());
	}
	else
	{
		m_pixels.clear();
	}
	Redraw();
}

void TileEditor::Redraw()
{
	Refresh(true);
}

int TileEditor::GetPixelSize() const
{
	return m_pixelsize;
}

void TileEditor::SetActivePalette(const std::string& name)
{
	if (m_selected_palette_name != name)
	{
		m_selected_palette_name = name;
		m_selected_palette_entry = m_gd->GetPalette(m_selected_palette_name);
		m_selected_palette = m_selected_palette_entry->GetData();
		ForceRedraw();
	}
}

std::string TileEditor::GetActivePalette() const
{
	return m_selected_palette_name;
}

const Tile& TileEditor::GetTile() const
{
	return m_tile;
}

void TileEditor::SetPrimaryColour(uint8_t colour)
{
	auto result = ValidateColour(colour);
	if (result >= 0)
	{
		m_primary_colour = result;
	}
}

uint8_t TileEditor::GetPrimaryColour() const
{
	return m_primary_colour;
}

void TileEditor::SetSecondaryColour(uint8_t colour)
{
	auto result = ValidateColour(colour);
	if(result >= 0)
	{
		m_secondary_colour = result;
	}
}

uint8_t TileEditor::GetSecondaryColour() const
{
	return m_secondary_colour;
}

bool TileEditor::GetEditEnabled() const
{
	return m_enableedit;
}

void TileEditor::SetEditEnabled(bool enabled)
{
	if (m_enableedit != enabled)
	{
		if (enabled == false)
		{
			m_selectedpixel = { -1, -1 };
			m_hoveredpixel = { -1, -1 };
		}
		m_enableedit = enabled;
		Redraw();
	}
}

bool TileEditor::GetAlphaEnabled() const
{
	return m_enablealpha;
}

void TileEditor::SetAlphaEnabled(bool enabled)
{
	if (m_enablealpha != enabled)
	{
		m_enablealpha = enabled;
		Redraw();
	}
}

bool TileEditor::GetBordersEnabled() const
{
	return m_enableborders;
}

void TileEditor::SetBordersEnabled(bool enabled)
{
	if (m_enableborders != enabled)
	{
		m_enableborders = enabled;
		Redraw();
	}
}

bool TileEditor::IsPointValid(const Point& point) const
{
	if (!m_tileset) return false;
	return ((point.x >= 0) && (point.x < m_tileset->GetTileWidth()) &&
	        (point.y >= 0) && (point.y < m_tileset->GetTileHeight()));
}

bool TileEditor::IsSelectionValid() const
{
	return IsPointValid(m_selectedpixel);
}

TileEditor::Point TileEditor::GetSelectedPixel() const
{
	return m_selectedpixel;
}

bool TileEditor::IsHoverValid() const
{
	return IsPointValid(m_hoveredpixel);
}

TileEditor::Point TileEditor::GetHoveredPixel() const
{
	return m_hoveredpixel;
}

int TileEditor::GetColourAtPixel(const Point& point) const
{
	if (m_pixels.empty()) throw std::runtime_error("m_pixels is empty");
	return m_pixels[point.x + point.y * m_tileset->GetTileWidth()];
}

bool TileEditor::SetColourAtPixel(const Point& point, int colour)
{
	bool retval = SetColour(point, colour);
	if (retval)
	{
		FireEvent(EVT_TILE_CHANGE);
	}
	return retval;
}

void TileEditor::Clear()
{
	if (!m_pixels.empty())
	{
		std::fill(m_pixels.begin(), m_pixels.end(), m_secondary_colour);
	}
}

wxCoord TileEditor::OnGetRowHeight(size_t row) const
{
	return wxCoord(m_pixelsize);
}

wxCoord TileEditor::OnGetColumnWidth(size_t column) const
{
	return wxCoord(m_pixelsize);
}

void TileEditor::OnDraw(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();

	wxPosition s = GetVisibleBegin();
	wxPosition e = GetVisibleEnd();

	if(!m_pixels.empty())
	{
		if (m_pixelsize > 3)
		{
			m_border_pen->SetStyle(wxPENSTYLE_SOLID);
		}
		else
		{
			m_border_pen->SetStyle(wxPENSTYLE_TRANSPARENT);
		}
		for (int i = 0; i < m_pixels.size(); ++i)
		{
			Point p = ConvertPixelToXY(i);
			if ((p.x >= s.GetCol()) && (p.x < e.GetCol()) &&
				(p.y >= s.GetRow()) && (p.y < e.GetRow()))
			{
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.SetBrush(GetBrush(m_pixels.at(i)));
				dc.DrawRectangle(p.x * m_pixelsize, p.y * m_pixelsize, m_pixelsize, m_pixelsize);
				if (m_enableborders)
				{
					dc.SetPen(*m_border_pen);
					dc.SetBrush(*wxTRANSPARENT_BRUSH);
					dc.DrawRectangle({ p.x * m_pixelsize, p.y * m_pixelsize, m_pixelsize + 1, m_pixelsize + 1 });
				}
			}
		}
		if (IsHoverValid())
		{
			auto p = GetHoveredPixel();
			wxBrush sel_brush = GetBrush(m_secondary_active ? m_secondary_colour : m_primary_colour);
			wxPen cursor(sel_brush.GetColour());
			cursor.SetWidth(std::min((m_pixelsize + 1)/2, 3));
			dc.SetPen(cursor);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle({ p.x * m_pixelsize, p.y * m_pixelsize, m_pixelsize + 1, m_pixelsize + 1 });
		}
	}
}

void TileEditor::OnPaint(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void TileEditor::SetPixelSize(int n)
{
	m_pixelsize = n;
	wxVarHScrollHelper::RefreshAll();
	wxVarVScrollHelper::RefreshAll();
	Refresh();
}

void TileEditor::OnSize(wxSizeEvent& evt)
{
	AutoSize();
	wxVarHScrollHelper::HandleOnSize(evt);
	wxVarVScrollHelper::HandleOnSize(evt);
	evt.Skip();
}

void TileEditor::OnMouseUpDown(wxMouseEvent& evt)
{
	if (evt.LeftDown())
	{
		m_drawing = true;
		m_secondary_active = false;
	}
	if (evt.RightDown())
	{
		m_drawing = true;
		m_secondary_active = true;
	}
	if (evt.LeftUp())
	{
		if (evt.RightIsDown())
		{
			m_secondary_active = true;
		}
		else
		{
			m_drawing = false;
			m_secondary_active = false;
		}
	}
	if (evt.RightUp())
	{
		m_secondary_active = false;
		if (!evt.LeftIsDown())
		{
			m_drawing = false;
		}
	}
	int px = ConvertMouseXYToPixel(evt.GetPosition());
	Point pt{ -1, -1 };
	if (px >= 0)
	{
		pt = ConvertPixelToXY(px);
	}
	MouseDraw(pt);
	evt.Skip();
}

void TileEditor::OnMouseMove(wxMouseEvent& evt)
{
	int px = ConvertMouseXYToPixel(evt.GetPosition());
	Point pt{ -1, -1 };
	if (px >= 0)
	{
		pt = ConvertPixelToXY(px);
	}
	MouseDraw(pt);
	evt.Skip();
}

void TileEditor::OnMouseEnter(wxMouseEvent& evt)
{
	if (evt.LeftIsDown() && m_drawing)
	{
		m_secondary_active = false;
	}
	if (evt.RightIsDown() && m_drawing)
	{
		m_secondary_active = true;
	}
	if (!evt.LeftIsDown() && !evt.RightIsDown())
	{
		m_drawing = false;
		m_secondary_active = false;
	}
	evt.Skip();
}

void TileEditor::OnMouseLeave(wxMouseEvent& evt)
{
	Point pt = { -1, -1 };
	if (IsHoverValid())
	{
		m_hoveredpixel = pt;
		Refresh();
	}
	evt.Skip();
}

void TileEditor::MouseDraw(const Point& point)
{
	bool refresh = false;
	int px = ConvertXYToPixel(point);
	if (point != m_hoveredpixel)
	{
		m_hoveredpixel = point;
		refresh = true;
		FireEvent(EVT_TILE_PIXEL_HOVER);
	}
	if ((px >= 0) && (m_drawing))
	{
		int colour = m_secondary_active ? m_secondary_colour : m_primary_colour;
		if (!m_pixels.empty() && m_pixels.at(px) != colour)
		{
			m_pixels.at(px) = colour;
			m_tileset->GetTilePixels(m_tile.GetIndex()) = m_pixels;
			refresh = true;
			FireEvent(EVT_TILE_CHANGE);
		}
	}
	if (refresh)
	{
		Refresh();
	}
}

void TileEditor::OnTilesetFocus(wxFocusEvent& evt)
{
	FireEvent(EVT_TILE_ACTIVATE);
	evt.Skip();
}

int TileEditor::ConvertMouseXYToPixel(const wxPoint& point)
{
	wxPosition s = GetVisibleBegin();
	Point pt = { point.x / m_pixelsize + s.GetCol(), point.y / m_pixelsize + s.GetRow()};

	if (IsPointValid(pt))
	{
		return pt.x + pt.y * m_tileset->GetTileWidth();
	}
	return -1;
}

void TileEditor::ForceRedraw()
{
	Update();
	Refresh(true);
}

void TileEditor::AutoSize()
{
	if (m_tileset != nullptr)
	{
		this->GetClientSize(&m_ctrlwidth, &m_ctrlheight);
		int pixwidth = m_ctrlwidth / m_tileset->GetTileWidth();
		int pixheight = m_ctrlheight / m_tileset->GetTileHeight();
		int new_pixelsize = std::min(pixwidth, pixheight);
		if (new_pixelsize <= 0)
		{
			new_pixelsize = 1;
		}
		if (new_pixelsize != m_pixelsize)
		{
			SetPixelSize(new_pixelsize);
		}
	}
	Refresh(false);
}

int TileEditor::ConvertXYToPixel(const Point& point)
{
	if (IsPointValid(point))
	{
		return point.x + point.y * m_tileset->GetTileWidth();
	}
	return -1;
}

TileEditor::Point TileEditor::ConvertPixelToXY(int pixel)
{
	if (m_pixels.empty() || (pixel < 0) || (pixel > m_pixels.size()))
	{
		return { -1, -1 };
	}
	return Point{ pixel % static_cast<int>(m_tileset->GetTileWidth()),
	              pixel / static_cast<int>(m_tileset->GetTileWidth()) };
}

bool TileEditor::SetColour(const Point& point, int colour)
{
	bool retval = false;
	int pixel = ConvertXYToPixel(point);
	if (!m_pixels.empty() && pixel != -1)
	{
		if (m_pixels[pixel] != colour)
		{
			m_pixels[pixel] = colour;
			m_tileset->GetTilePixels(m_tile.GetIndex()) = m_pixels;
			Refresh(true);
			retval = true;
		}
	}
	return retval;
}

int TileEditor::ValidateColour(int colour) const
{
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
		auto result = std::find(cmap.begin(), cmap.end(), colour) - cmap.begin();
		if (result < (1 << m_tileset->GetTileBitDepth()))
		{
			return result;
		}
	}
	return -1;
}

int TileEditor::GetColour(int index) const
{
	auto cmap = m_tileset->GetColourIndicies();
	if (index >= cmap.size())
	{
		if (index < (1 << m_tileset->GetTileBitDepth()))
		{
			return index;
		}
	}
	else
	{
		return cmap[index];
	}
	return -1;
}

void TileEditor::InitialiseBrushesAndPens()
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

wxBrush TileEditor::GetBrush(int index)
{
	if ((index < 0) || (index > (1 << m_tileset->GetTileBitDepth()))) return *wxGREY_BRUSH;
	uint32_t colour;
	if (!m_tileset->GetColourIndicies().empty())
	{
		colour = m_selected_palette->getBGRA(m_tileset->GetColourIndicies()[index]);
	}
	else
	{
		colour = m_selected_palette->getBGRA(index);
	}
	if (m_enablealpha && ((colour & 0xFF000000) == 0))
	{
		return *m_alpha_brush;
	}
	else
	{
		return wxBrush(wxColour(colour & 0xFFFFFF));
	}
}

void TileEditor::FireEvent(const wxEventType& e)
{
	wxCommandEvent evt(e);
	evt.SetString(std::to_string(m_tile.GetIndex()));
	evt.SetClientData(&m_tileset);
	wxPostEvent(this->GetParent(), evt);
}
