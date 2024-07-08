#include "SubspriteControlFrame.h"

#include <SpriteEditorFrame.h>

SubspriteControlFrame::SubspriteControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

SubspriteControlFrame::~SubspriteControlFrame()
{
}

void SubspriteControlFrame::SetSubsprites(const std::vector<SpriteFrame::SubSprite>& subsprites)
{
}

void SubspriteControlFrame::ResetSubsprites()
{
}

int SubspriteControlFrame::GetMaxSelection() const
{
	return 0;
}

void SubspriteControlFrame::Select()
{
}

void SubspriteControlFrame::Add()
{
}

void SubspriteControlFrame::Delete()
{
}

void SubspriteControlFrame::MoveUp()
{
}

void SubspriteControlFrame::MoveDown()
{
}

void SubspriteControlFrame::OpenElement()
{
}

std::string SubspriteControlFrame::MakeLabel(int index) const
{
	return std::string();
}

bool SubspriteControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return false;
}
