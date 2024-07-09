#include "FrameControlFrame.h"

#include <SpriteEditorFrame.h>

wxDEFINE_EVENT(EVT_FRAME_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_FRAME_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_FRAME_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_FRAME_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_FRAME_MOVE_DOWN, wxCommandEvent);

FrameControlFrame::FrameControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
}

FrameControlFrame::~FrameControlFrame()
{
}

void FrameControlFrame::SetSprite(uint8_t sprite_id)
{
	m_frames.clear();
	if (m_gd)
	{
		m_frames = m_gd->GetSpriteData()->GetSpriteFrames(sprite_id);
	}
	UpdateUI();
}

void FrameControlFrame::ResetSprite()
{
	m_frames.clear();
	UpdateUI();
}

int FrameControlFrame::GetMaxSelection() const
{
	return m_frames.size();
}

void FrameControlFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
}

void FrameControlFrame::ClearGameData()
{
	m_gd = nullptr;
}

void FrameControlFrame::Select()
{
	FireEvent(EVT_FRAME_SELECT);
}

void FrameControlFrame::Add()
{
	FireEvent(EVT_FRAME_ADD);
}

void FrameControlFrame::Delete()
{
	FireEvent(EVT_FRAME_DELETE);
}

void FrameControlFrame::MoveUp()
{
	FireEvent(EVT_FRAME_MOVE_UP);
}

void FrameControlFrame::MoveDown()
{
	FireEvent(EVT_FRAME_MOVE_DOWN);
}

void FrameControlFrame::OpenElement()
{
}

std::string FrameControlFrame::MakeLabel(int index) const
{
	return m_frames[index];
}

bool FrameControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return true;
}
