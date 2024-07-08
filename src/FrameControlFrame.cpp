#include "FrameControlFrame.h"

#include <SpriteEditorFrame.h>

FrameControlFrame::FrameControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

FrameControlFrame::~FrameControlFrame()
{
}

void FrameControlFrame::SetFrames(const std::vector<std::string>& frames)
{
}

void FrameControlFrame::ResetFrames()
{
}

int FrameControlFrame::GetMaxSelection() const
{
	return 0;
}

void FrameControlFrame::SetGameData(std::shared_ptr<GameData> gd)
{
}

void FrameControlFrame::ClearGameData()
{
}

void FrameControlFrame::Select()
{
}

void FrameControlFrame::Add()
{
}

void FrameControlFrame::Delete()
{
}

void FrameControlFrame::MoveUp()
{
}

void FrameControlFrame::MoveDown()
{
}

void FrameControlFrame::OpenElement()
{
}

std::string FrameControlFrame::MakeLabel(int index) const
{
	return std::string();
}

bool FrameControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return false;
}
