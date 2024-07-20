#include "SpriteAnimationEditorCtrl.h"

SpriteAnimationEditorCtrl::SpriteAnimationEditorCtrl(wxWindow* parent)
	: wxHVScrolledWindow(parent)
{
}

SpriteAnimationEditorCtrl::~SpriteAnimationEditorCtrl()
{
}

bool SpriteAnimationEditorCtrl::Open(std::vector<std::vector<std::string>> animations, std::map<std::string, std::shared_ptr<SpriteFrame>> frames, std::shared_ptr<Palette> pal)
{
	return false;
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
}

void SpriteAnimationEditorCtrl::ClearGameData()
{
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
