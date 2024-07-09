#include "AnimationControlFrame.h"

#include <SpriteEditorFrame.h>

wxDEFINE_EVENT(EVT_ANIMATION_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_MOVE_DOWN, wxCommandEvent);

AnimationControlFrame::AnimationControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

AnimationControlFrame::~AnimationControlFrame()
{
}

void AnimationControlFrame::SetSprite(uint8_t sprite_id)
{
	m_anims.clear();
	if (m_gd)
	{
		m_anims = m_gd->GetSpriteData()->GetSpriteAnimations(sprite_id);
	}
	UpdateUI();
}

void AnimationControlFrame::ResetSprite()
{
	m_anims.clear();
}

int AnimationControlFrame::GetMaxSelection() const
{
	return m_anims.size();
}

void AnimationControlFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
}

void AnimationControlFrame::ClearGameData()
{
	m_gd.reset();
}

void AnimationControlFrame::Select()
{
	FireEvent(EVT_ANIMATION_SELECT);
}

void AnimationControlFrame::Add()
{
	FireEvent(EVT_ANIMATION_ADD);
}

void AnimationControlFrame::Delete()
{
	FireEvent(EVT_ANIMATION_DELETE);
}

void AnimationControlFrame::MoveUp()
{
	FireEvent(EVT_ANIMATION_MOVE_UP);
}

void AnimationControlFrame::MoveDown()
{
	FireEvent(EVT_ANIMATION_MOVE_DOWN);
}

void AnimationControlFrame::OpenElement()
{
}

std::string AnimationControlFrame::MakeLabel(int index) const
{
	return m_anims.at(index);
}

bool AnimationControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return true;
}
