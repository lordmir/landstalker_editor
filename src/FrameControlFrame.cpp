#include "FrameControlFrame.h"

#include <SpriteEditorFrame.h>

wxDEFINE_EVENT(EVT_FRAME_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_FRAME_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_FRAME_DELETE, wxCommandEvent);

FrameControlFrame::FrameControlFrame(SpriteEditorFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst)
{
	m_ctrl_move_up->Hide();
	m_ctrl_move_down->Hide();
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
}

void FrameControlFrame::MoveDown()
{
}

void FrameControlFrame::OpenElement()
{
}

std::string FrameControlFrame::MakeLabel(int index) const
{
	return m_frames[index];
}

bool FrameControlFrame::HandleKeyPress(unsigned int /*key*/, unsigned int /*modifiers*/)
{
	return true;
}
