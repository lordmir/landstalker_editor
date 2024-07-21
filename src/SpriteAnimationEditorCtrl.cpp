#include "SpriteAnimationEditorCtrl.h"

SpriteAnimationEditorCtrl::SpriteAnimationEditorCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent),
	  m_sprite_id(0),
	  m_animation(0),
	  m_frame(0)
{
}

SpriteAnimationEditorCtrl::~SpriteAnimationEditorCtrl()
{
}

bool SpriteAnimationEditorCtrl::Open(uint8_t sprite_id, uint8_t anim_id, uint8_t frame_id, std::shared_ptr<Palette> pal)
{
	if (m_gd == nullptr)
	{
		return false;
	}
	m_pal = pal;
	m_sprite_id = sprite_id;
	m_animation = anim_id;
	m_frame = frame_id;
	m_animations.clear();
	m_frames.clear();
	for (const auto& a : m_gd->GetSpriteData()->GetSpriteAnimations(sprite_id))
	{
		m_animations.push_back(m_gd->GetSpriteData()->GetSpriteAnimationFrames(a));
	}
	for (const auto& f : m_gd->GetSpriteData()->GetSpriteFrames(sprite_id))
	{
		m_frames[f] = m_gd->GetSpriteData()->GetSpriteFrame(f);
	}
	return true;
}

void SpriteAnimationEditorCtrl::RedrawTiles(int index)
{
}

void SpriteAnimationEditorCtrl::SetPixelSize(int n)
{
}

int SpriteAnimationEditorCtrl::GetPixelSize() const
{
	return 0;
}

void SpriteAnimationEditorCtrl::SetActivePalette(std::shared_ptr<Palette> pal)
{
}

int SpriteAnimationEditorCtrl::GetAnimSpeed() const
{
	return 0;
}

void SpriteAnimationEditorCtrl::SetAnimSpeed(int speed)
{
}

bool SpriteAnimationEditorCtrl::IsPlaying() const
{
	return false;
}

void SpriteAnimationEditorCtrl::Play()
{
}

void SpriteAnimationEditorCtrl::Pause()
{
}

void SpriteAnimationEditorCtrl::SetAnimation(int animation)
{
	m_animation = animation;
	m_frame = 0;
}

int SpriteAnimationEditorCtrl::GetAnimation() const
{
	return m_animation;
}

void SpriteAnimationEditorCtrl::SetCurrentFrame(int frame)
{
	if (frame >= 0 && frame < static_cast<int>(m_animations.at(m_animation).size()))
	{
		m_frame = frame;
	}
}

int SpriteAnimationEditorCtrl::GetCurrentFrame() const
{
	return m_frame;
}

bool SpriteAnimationEditorCtrl::IsSelectionValid() const
{
	return false;
}

std::shared_ptr<SpriteFrame> SpriteAnimationEditorCtrl::GetSelectedFrame() const
{
	return std::shared_ptr<SpriteFrame>();
}

void SpriteAnimationEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
	RedrawTiles();
}

void SpriteAnimationEditorCtrl::ClearGameData()
{
	m_gd = nullptr;
	m_sprite_id = 0;
	m_animation = 0;
	m_frame = 0;
	m_animations.clear();
	m_frames.clear();
	RedrawTiles();
}

wxCoord SpriteAnimationEditorCtrl::OnGetRowHeight(size_t row) const
{
	return wxCoord();
}

wxCoord SpriteAnimationEditorCtrl::OnGetColumnWidth(size_t column) const
{
	return wxCoord();
}

void SpriteAnimationEditorCtrl::OnDraw(wxDC& dc)
{
}

void SpriteAnimationEditorCtrl::OnPaint(wxPaintEvent& evt)
{
}

void SpriteAnimationEditorCtrl::OnSize(wxSizeEvent& evt)
{
}
