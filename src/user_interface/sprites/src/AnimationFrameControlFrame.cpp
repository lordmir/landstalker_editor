#include <user_interface/sprites/include/AnimationFrameControlFrame.h>

#include <user_interface/sprites/include/SpriteEditorFrame.h>


wxDEFINE_EVENT(EVT_ANIMATION_FRAME_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_FRAME_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_FRAME_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_FRAME_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_FRAME_MOVE_DOWN, wxCommandEvent);
wxDEFINE_EVENT(EVT_ANIMATION_FRAME_CHANGE, wxCommandEvent);

AnimationFrameControlFrame::AnimationFrameControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

AnimationFrameControlFrame::~AnimationFrameControlFrame()
{
}

void AnimationFrameControlFrame::SetAnimation(uint8_t sprite_id, uint8_t anim_id)
{
	m_frames.clear();
	if (m_gd)
	{
		const auto& namelist = m_gd->GetSpriteData()->GetSpriteAnimationFrames(sprite_id, anim_id);
		m_frames = std::vector<wxString>(namelist.cbegin(), namelist.cend());
	}
	UpdateUI();
}

void AnimationFrameControlFrame::ResetAnimation()
{
	m_frames.clear();
	UpdateUI();
}

int AnimationFrameControlFrame::GetMaxSelection() const
{
	return m_frames.size();
}

void AnimationFrameControlFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
}

void AnimationFrameControlFrame::ClearGameData()
{
	m_gd.reset();
}

void AnimationFrameControlFrame::Select()
{
	FireEvent(EVT_ANIMATION_FRAME_SELECT);
}

void AnimationFrameControlFrame::Add()
{
	FireEvent(EVT_ANIMATION_FRAME_ADD);
}

void AnimationFrameControlFrame::Delete()
{
	FireEvent(EVT_ANIMATION_FRAME_DELETE);
}

void AnimationFrameControlFrame::MoveUp()
{
	FireEvent(EVT_ANIMATION_FRAME_MOVE_UP);
}

void AnimationFrameControlFrame::MoveDown()
{
	FireEvent(EVT_ANIMATION_FRAME_MOVE_DOWN);
}

void AnimationFrameControlFrame::OpenElement()
{
	FireEvent(EVT_ANIMATION_FRAME_CHANGE);
}

std::wstring AnimationFrameControlFrame::MakeLabel(int index) const
{
	return m_frames.at(index).ToStdWstring();
}

bool AnimationFrameControlFrame::HandleKeyPress(unsigned int /*key*/, unsigned int /*modifiers*/)
{
	return true;
}
