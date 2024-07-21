#ifndef _ENTITY_VIEWER_CTRL_H_
#define _ENTITY_VIEWER_CTRL_H_

#include <GameData.h>

#include <wx/wx.h>
#include <wx/vscroll.h>
#include <memory>

class EntityViewerCtrl : public wxHVScrolledWindow
{
public:
	EntityViewerCtrl(wxWindow* parent);
	virtual ~EntityViewerCtrl();

	virtual void SetGameData(std::shared_ptr<GameData> gd);
	virtual void ClearGameData();

	void SetActivePalette(const std::shared_ptr<Palette> pal);

	bool IsPlaying() const;
	void Play();
	void Pause();
	int GetAnimSpeed() const;
	void SetAnimSpeed(int speed);

	int GetAnimation() const;
	void SetAnimation(int anim);
	int GetAnimationFrame() const;
	void SetAnimationFrame(int frame);
	int GetFrame() const;
	void SetFrame(int frame);

	int GetPixelSize() const;
	void SetPixelSize(int size);

	void Open(uint8_t entity, uint8_t animation, std::shared_ptr<Palette> pal);
	void Open(uint8_t entity, std::shared_ptr<Palette> pal);
	void Open(std::shared_ptr<SpriteFrame> frame, std::shared_ptr<Palette> pal);


private:
	virtual wxCoord OnGetRowHeight(size_t row) const override;
	virtual wxCoord OnGetColumnWidth(size_t column) const override;

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnTimer(wxTimerEvent& evt);

	bool DrawTileAtPosition(wxDC& dc, int pos);
	void DrawTile(wxDC& dc, int x, int y, int tile);
	wxPoint SpriteToScreenXY(const wxPoint& point);
	void UpdateSprite();

	int m_entity_id = -1;
	int m_sprite_id = -1;
	int m_animation = 0;
	int m_frame = 0;
	int m_pixelsize = 4;

	int m_columns = 1;
	int m_rows = 1;
	int m_cellwidth = 8;
	int m_cellheight = 8;

	int m_speed = 4;
	bool m_playing = true;

	std::shared_ptr<GameData> m_gd;
	std::shared_ptr<SpriteFrame> m_sprite;
	std::shared_ptr<Palette> m_palette;

	wxTimer* m_timer = nullptr;

	wxDECLARE_EVENT_TABLE();
};

#endif // _ENTITY_VIEWER_CTRL_H_
