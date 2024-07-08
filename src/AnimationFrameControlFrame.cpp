#include "AnimationFrameControlFrame.h"

#include <SpriteEditorFrame.h>

AnimationFrameControlFrame::AnimationFrameControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

AnimationFrameControlFrame::~AnimationFrameControlFrame()
{
}

void AnimationFrameControlFrame::SetAnimationFrames(const std::vector<std::string>& frames)
{
}

void AnimationFrameControlFrame::ResetAnimationFrames()
{
}

int AnimationFrameControlFrame::GetMaxSelection() const
{
	return 0;
}

void AnimationFrameControlFrame::SetGameData(std::shared_ptr<GameData> gd)
{
}

void AnimationFrameControlFrame::ClearGameData()
{
}

void AnimationFrameControlFrame::Select()
{
}

void AnimationFrameControlFrame::Add()
{
}

void AnimationFrameControlFrame::Delete()
{
}

void AnimationFrameControlFrame::MoveUp()
{
}

void AnimationFrameControlFrame::MoveDown()
{
}

void AnimationFrameControlFrame::OpenElement()
{
}

std::string AnimationFrameControlFrame::MakeLabel(int index) const
{
	return std::string();
}

bool AnimationFrameControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return false;
}
