#include "AnimationControlFrame.h"

#include <SpriteEditorFrame.h>

AnimationControlFrame::AnimationControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

AnimationControlFrame::~AnimationControlFrame()
{
}

void AnimationControlFrame::SetAnimations(const std::vector<std::string>& frames)
{
}

void AnimationControlFrame::ResetAnimations()
{
}

int AnimationControlFrame::GetMaxSelection() const
{
	return 0;
}

void AnimationControlFrame::SetGameData(std::shared_ptr<GameData> gd)
{
}

void AnimationControlFrame::ClearGameData()
{
}

void AnimationControlFrame::Select()
{
}

void AnimationControlFrame::Add()
{
}

void AnimationControlFrame::Delete()
{
}

void AnimationControlFrame::MoveUp()
{
}

void AnimationControlFrame::MoveDown()
{
}

void AnimationControlFrame::OpenElement()
{
}

std::string AnimationControlFrame::MakeLabel(int index) const
{
	return std::string();
}

bool AnimationControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return false;
}
