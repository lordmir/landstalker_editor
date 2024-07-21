#include <user_interface/sprites/include/EntityViewerCtrl.h>
#include <wx/dcbuffer.h>

wxBEGIN_EVENT_TABLE(EntityViewerCtrl, wxHVScrolledWindow)
EVT_PAINT(EntityViewerCtrl::OnPaint)
EVT_SIZE(EntityViewerCtrl::OnSize)
wxEND_EVENT_TABLE()

EntityViewerCtrl::EntityViewerCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent),
	  m_rows(36),
	  m_columns(36),
	  m_timer(new wxTimer(this))
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetRowColumnCount(m_rows, m_columns);
	this->Bind(wxEVT_TIMER, &EntityViewerCtrl::OnTimer, this);
	m_timer->Start(1000 * m_speed / 30);
}

EntityViewerCtrl::~EntityViewerCtrl()
{
	if (m_timer->IsRunning())
	{
		m_timer->Stop();
	}
	this->Unbind(wxEVT_TIMER, &EntityViewerCtrl::OnTimer, this);
}

void EntityViewerCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	Refresh(true);
}

void EntityViewerCtrl::ClearGameData()
{
	Pause();
	m_gd.reset();
	m_entity_id = -1;
	m_sprite.reset();
	m_palette.reset();
	Refresh(true);
}

void EntityViewerCtrl::SetActivePalette(const std::shared_ptr<Palette> pal)
{
	m_palette = pal;
	Refresh(true);
}

bool EntityViewerCtrl::IsPlaying() const
{
	return m_playing;
}

void EntityViewerCtrl::Play()
{
	m_timer->Start(1000 * m_speed / 30);
	m_playing = true;
}

void EntityViewerCtrl::Pause()
{
	if (m_timer->IsRunning())
	{
		m_timer->Stop();
	}
	m_playing = false;
}

int EntityViewerCtrl::GetAnimSpeed() const
{
	return m_speed;
}

void EntityViewerCtrl::SetAnimSpeed(int speed)
{
	m_speed = speed;
	if (m_timer->IsRunning())
	{
		m_timer->Stop();
		m_timer->Start(1000 * m_speed / 30);
	}
}

int EntityViewerCtrl::GetAnimation() const
{
	return m_animation;
}

void EntityViewerCtrl::SetAnimation(int anim)
{
	m_animation = anim;
	m_frame = 0;
	UpdateSprite();
}

int EntityViewerCtrl::GetAnimationFrame() const
{
	return m_frame;
}

void EntityViewerCtrl::SetAnimationFrame(int frame)
{
	m_frame = frame;
	UpdateSprite();
}


int EntityViewerCtrl::GetPixelSize() const
{
	return m_pixelsize;
}

void EntityViewerCtrl::SetPixelSize(int size)
{
	m_pixelsize = size;

	Refresh(true);
}

void EntityViewerCtrl::Open(uint8_t entity, uint8_t animation, std::shared_ptr<Palette> pal)
{
	m_entity_id = entity;
	m_sprite_id = m_gd->GetSpriteData()->GetSpriteFromEntity(entity);
	m_animation = animation;
	m_frame = 0;
	m_palette = pal;
	m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite_id, m_animation, m_frame)->GetData();
	bool freeze = m_gd->GetSpriteData()->IsEntityItem(entity);
	m_cellwidth = m_pixelsize * m_sprite->GetTileWidth();
	m_cellheight = m_pixelsize * m_sprite->GetTileHeight();
	if (freeze && m_playing)
	{
		Pause();
	}
	else if(!freeze && !m_playing)
	{
		Play();
	}
	Refresh(true);
}

void EntityViewerCtrl::Open(uint8_t entity, std::shared_ptr<Palette> pal)
{
	m_entity_id = entity;
	m_sprite_id = m_gd->GetSpriteData()->GetSpriteFromEntity(entity);
	m_animation = m_gd->GetSpriteData()->GetDefaultEntityAnimationId(entity);
	m_frame = m_gd->GetSpriteData()->GetDefaultEntityFrameId(entity);
	m_palette = pal;
	bool freeze = true;
	auto sd = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite_id, m_animation, m_frame);
	if (sd)
	{
		m_sprite = sd->GetData();
		freeze = m_gd->GetSpriteData()->IsEntityItem(entity);
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
	if (freeze && m_playing)
	{
		Pause();
	}
	Refresh(true);
}

void EntityViewerCtrl::Open(std::shared_ptr<SpriteFrame> frame, std::shared_ptr<Palette> pal)
{
	m_entity_id = 0;
	m_animation = 0;
	m_frame = 0;
	bool freeze = true;
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
	if (freeze && m_playing)
	{
		Pause();
	}
	Refresh(true);
}

wxCoord EntityViewerCtrl::OnGetRowHeight(size_t /*row*/) const
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

wxCoord EntityViewerCtrl::OnGetColumnWidth(size_t /*column*/) const
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
	dc.SetBackground(wxBrush(wxColor(32, 32, 32)));
	dc.Clear();

	if (m_sprite && m_palette)
	{
		for (int i = 0; i < static_cast<int>(m_sprite->GetTileCount()); ++i)
		{
			DrawTileAtPosition(dc, i);
		}
	}
}

void EntityViewerCtrl::OnPaint(wxPaintEvent& /*evt*/)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void EntityViewerCtrl::OnSize(wxSizeEvent& evt)
{
	wxVarHScrollHelper::HandleOnSize(evt);
	wxVarVScrollHelper::HandleOnSize(evt);
	ScrollToRow(std::max<int>(0U, (m_rows - GetVisibleRowsEnd() + GetVisibleRowsBegin()) / 2 - 4));
	ScrollToColumn(1 + std::max<int>(0U, (m_columns - GetVisibleColumnsEnd() + GetVisibleColumnsBegin()) / 2 - 2));
	Refresh(false);
}

void EntityViewerCtrl::OnTimer(wxTimerEvent& /*evt*/)
{
	if (m_gd && m_sprite && m_playing)
	{
		m_frame++;
		if (m_frame >= static_cast<int>(m_gd->GetSpriteData()->GetSpriteAnimationFrameCount(m_sprite_id, m_animation)))
		{
			m_frame = 0;
		}
		UpdateSprite();
	}
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
	std::array<uint32_t, 64> tile_pixels = {0};
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

void EntityViewerCtrl::UpdateSprite()
{
	if (m_gd)
	{
		m_sprite = m_gd->GetSpriteData()->GetSpriteFrame(m_sprite_id, m_animation, m_frame)->GetData();
		Refresh(true);
	}
}
