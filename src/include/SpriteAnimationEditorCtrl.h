#ifndef _SPRITE_ANIMATION_EDITOR_CTRL_H_
#define _SPRITE_ANIMATION_EDITOR_CTRL_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

#include <Palette.h>
#include <SpriteFrame.h>
#include <GameData.h>

#include <memory>
#include <string>
#include <vector>

class SpriteAnimationEditorCtrl : public wxHVScrolledWindow
{
public:
	SpriteAnimationEditorCtrl(wxWindow* parent);
	virtual ~SpriteAnimationEditorCtrl();
	bool Open(uint8_t sprite_id, uint8_t anim_id, uint8_t frame_id, std::shared_ptr<Palette> pal);
	void RedrawTiles(int index = -1);
	void SetPixelSize(int n);
	int GetPixelSize() const;
	void SetActivePalette(std::shared_ptr<Palette> pal);
	int GetAnimSpeed() const;
	void SetAnimSpeed(int speed);
	bool IsPlaying() const;
	void Play();
	void Pause();

	void SetAnimation(int animation_name);
	int GetAnimation() const;
	void SetCurrentFrame(int frame);
	int GetCurrentFrame() const;

	bool IsSelectionValid() const;
	std::shared_ptr<SpriteFrame> GetSelectedFrame() const;

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();
private:
	virtual wxCoord OnGetRowHeight(size_t row) const override;
	virtual wxCoord OnGetColumnWidth(size_t column) const override;

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);

	int m_sprite_id;
	int m_animation;
	int m_frame;

	std::vector<std::vector<std::string>> m_animations;
	std::map<std::string, std::shared_ptr<SpriteFrameEntry>> m_frames;
	std::shared_ptr<Palette> m_pal;
	std::shared_ptr<GameData> m_gd;
};

#endif // _SPRITE_ANIMATION_EDITOR_CTRL_H_
