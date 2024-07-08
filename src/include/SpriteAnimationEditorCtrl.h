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
	bool Open(std::vector<std::vector<std::string>> animations, std::map<std::string, std::shared_ptr<SpriteFrame>> frames, std::shared_ptr<Palette> pal);
	void RedrawTiles(int index = -1);
	void SetPixelSize(int n);
	int GetPixelSize() const;
	void SetActivePalette(std::shared_ptr<Palette> pal);

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

	std::vector<std::vector<std::string>> m_animations;
	std::map<std::string, std::shared_ptr<SpriteFrame>> m_frames;
	std::shared_ptr<Palette> m_pal;
	std::shared_ptr<GameData> m_gd;
};

#endif // _SPRITE_ANIMATION_EDITOR_CTRL_H_
