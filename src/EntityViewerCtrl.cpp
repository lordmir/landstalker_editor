#include <EntityViewerCtrl.h>
#include <wx/dcbuffer.h>

wxBEGIN_EVENT_TABLE(EntityViewerCtrl, wxHVScrolledWindow)
EVT_PAINT(EntityViewerCtrl::OnPaint)
EVT_SIZE(EntityViewerCtrl::OnSize)
wxEND_EVENT_TABLE()

EntityViewerCtrl::EntityViewerCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetRowColumnCount(36, 36);
}

EntityViewerCtrl::~EntityViewerCtrl()
{
}

void EntityViewerCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	Refresh(true);
}

void EntityViewerCtrl::ClearGameData()
{
	m_gd.reset();
	m_entity_id = -1;
	m_sprite.reset();
	m_palette.reset();
	Refresh(true);
}

void EntityViewerCtrl::Open(std::shared_ptr<SpriteFrame> frame, std::shared_ptr<Palette> pal)
{
	m_entity_id = 0;
	m_animation = 0;
	m_frame = 0;
	if (frame && pal)
	{
		m_sprite = frame;
		m_palette = pal;
		m_cellwidth = m_pixelsize * m_sprite->GetTileWidth();
		m_cellheight = m_pixelsize * m_sprite->GetTileHeight();
	}
	else
	{
		m_sprite = nullptr;
		m_palette = nullptr;
		m_cellwidth = m_pixelsize;
		m_cellheight = m_pixelsize;
	}
	Refresh(true);
}

wxCoord EntityViewerCtrl::OnGetRowHeight(size_t row) const
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

wxCoord EntityViewerCtrl::OnGetColumnWidth(size_t column) const
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

void EntityViewerCtrl::OnDraw(wxDC& dc)
{
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
	dc.Clear();

	wxPosition s = GetVisibleBegin();
	wxPosition e = GetVisibleEnd();

	if (m_sprite && m_palette)
	{
		for (int i = 0; i < static_cast<int>(m_sprite->GetTileCount()); ++i)
		{
			DrawTileAtPosition(dc, i);
		}
	}
}

void EntityViewerCtrl::OnPaint(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void EntityViewerCtrl::OnSize(wxSizeEvent& evt)
{
	wxVarHScrollHelper::HandleOnSize(evt);
	wxVarVScrollHelper::HandleOnSize(evt);
	evt.Skip();
}

bool EntityViewerCtrl::DrawTileAtPosition(wxDC& dc, int pos)
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
		DrawTile(dc, x * m_cellwidth, y * m_cellheight, pos);
		retval = true;
	}
	return retval;
}

void EntityViewerCtrl::DrawTile(wxDC& dc, int x, int y, int tile)
{
	wxPen pen = dc.GetPen();
	wxBrush brush = dc.GetBrush();

	pen.SetStyle(wxPENSTYLE_TRANSPARENT);
	brush.SetStyle(wxBRUSHSTYLE_SOLID);
	dc.SetPen(pen);

	auto tile_bytes = m_sprite->GetTile(tile);
	std::array<uint32_t, 64> tile_pixels;
	auto it = tile_pixels.begin();
	for (const auto& b : tile_bytes)
	{
		*it++ = m_palette->getBGRA(b);
	}

	for (int i = 0; i < static_cast<int>(tile_pixels.size()); ++i)
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

wxPoint EntityViewerCtrl::SpriteToScreenXY(const wxPoint& point)
{
	return wxPoint((point.x + 0x80) * m_pixelsize, (point.y + 0x80) * m_pixelsize);
}
