#include "SubspriteControlFrame.h"

#include <SpriteEditorFrame.h>

wxDEFINE_EVENT(EVT_SUBSPRITE_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_SUBSPRITE_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_SUBSPRITE_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_SUBSPRITE_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_SUBSPRITE_MOVE_DOWN, wxCommandEvent);

SubspriteControlFrame::SubspriteControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

SubspriteControlFrame::~SubspriteControlFrame()
{
}

void SubspriteControlFrame::SetSubsprites(const std::vector<SpriteFrame::SubSprite>& subsprites)
{
	m_subsprites = subsprites;
	UpdateUI();
}

void SubspriteControlFrame::ResetSubsprites()
{
	m_subsprites.clear();
	UpdateUI();
}

int SubspriteControlFrame::GetMaxSelection() const
{
	return m_subsprites.size();
}

void SubspriteControlFrame::Select()
{
	FireEvent(EVT_SUBSPRITE_SELECT);
}

void SubspriteControlFrame::Add()
{
	FireEvent(EVT_SUBSPRITE_ADD);
}

void SubspriteControlFrame::Delete()
{
	FireEvent(EVT_SUBSPRITE_DELETE);
}

void SubspriteControlFrame::MoveUp()
{
	FireEvent(EVT_SUBSPRITE_MOVE_UP);
}

void SubspriteControlFrame::MoveDown()
{
	FireEvent(EVT_SUBSPRITE_MOVE_DOWN);
}

void SubspriteControlFrame::OpenElement()
{
}

std::string SubspriteControlFrame::MakeLabel(int index) const
{
	return StrPrintf("%02d: (%02d, %02d) %01dx%01d @%02d", index + 1, m_subsprites[index].x,
		m_subsprites[index].y, m_subsprites[index].w, m_subsprites[index].h, m_subsprites[index].tile_idx);
}

bool SubspriteControlFrame::HandleKeyPress(unsigned int /*key*/, unsigned int /*modifiers*/)
{
	return false;
}
