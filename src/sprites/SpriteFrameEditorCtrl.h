#ifndef _SPRITE_FRAME_EDITOR_CTRL_H_
#define _SPRITE_FRAME_EDITOR_CTRL_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

#include <vector>
#include <memory>
#include <set>
#include <map>
#include <string>

#include <landstalker/main/GameData.h>
#include <main/ImageBufferWx.h>
#include <landstalker/sprites/SpriteFrame.h>
#include <landstalker/palettes/Palette.h>

class SpriteFrameEditorCtrl : public wxHVScrolledWindow
{
public:
	SpriteFrameEditorCtrl(wxWindow* parent);
	~SpriteFrameEditorCtrl();

	bool Save(wxString filename, bool compressed = false);
	bool Open(wxString filename, int sprite_id);
	bool Open(std::vector<uint8_t>& pixels, int sprite_id);
	bool Open(std::shared_ptr<Landstalker::SpriteFrame> frame, std::shared_ptr<Landstalker::Palette> pal, int sprite_id);
	void RedrawTiles(int index = -1);
	void UpdateSubSprites();

	void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	void ClearGameData();

	void SetPixelSize(int n);
	int GetPixelSize() const;
	int GetTilemapSize() const;
	bool GetCompressed() const;
	void SetActivePalette(std::shared_ptr<Landstalker::Palette> pal);
	void SelectSubSprite(int sel);
	int GetSelectedSubSprite() const;
	int GetHoveredSubSprite() const;
	void ClearSubSpriteSelection();
	std::shared_ptr<Landstalker::Tileset> GetTileset();

	bool GetSelectionEnabled() const;
	void SetSelectionEnabled(bool enabled);
	bool GetHoverEnabled() const;
	void SetHoverEnabled(bool enabled);
	bool GetAlphaEnabled() const;
	void SetAlphaEnabled(bool enabled);
	bool GetBordersEnabled() const;
	void SetBordersEnabled(bool enabled);
	bool GetHitboxEnabled() const;
	void SetHitboxEnabled(bool enabled);

	std::pair<int, int> GetTilePosition(int tile) const;
	bool IsSelectionValid() const;
	Landstalker::Tile GetSelectedTile() const;
	std::pair<int, int> GetSelectedTilePosition() const;
	bool IsHoverValid() const;
	Landstalker::Tile GetHoveredTile() const;
	std::pair<int, int> GetHoveredTilePosition() const;
	int GetFirstTile() const;
	int GetSubspriteAt(int tile) const;

	void SelectTile(int tile);
	void ClearCell();
	void CutCell();
	void CopyCell();
	void PasteCell();
	void SwapCell();
	bool IsClipboardEmpty() const;

	bool HandleKeyDown(int key, int modifiers);

private:
	virtual wxCoord OnGetRowHeight(size_t row) const override;
	virtual wxCoord OnGetColumnWidth(size_t column) const override;

	void ClearSelections();
	void MoveSelectionUp();
	void MoveSelectionDown();
	void MoveSelectionLeft();
	void MoveSelectionRight();
	void MoveSubSpriteUp();
	void MoveSubSpriteDown();
	void MoveSubSpriteLeft();
	void MoveSubSpriteRight();
	void ExpandSubSpriteWidth();
	void ContractSubSpriteWidth();
	void ExpandSubSpriteHeight();
	void ContractSubSpriteHeight();
	bool CheckSubSpriteCollision(const Landstalker::SpriteFrame::SubSprite& s, int index);
	void InsertSubSprite();
	void DeleteSubSprite();
	void IncreaseSubSpritePriority();
	void DecreaseSubSpritePriority();
	void SelectNextSubSprite();
	void SelectPrevSubSprite();

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnTilesetFocus(wxFocusEvent& evt);
	int  ConvertXYToTile(const wxPoint& point);
	wxPoint ConvertTileToXY(int tile) const;

	wxPoint SpriteToScreenXY(wxPoint sprite);
	wxPoint ScreenToSpriteXY(wxPoint screen);

	bool IsTileInSprite(int tile) const;
	int GetSpriteTileNum(int tile);

	void UpdateTileBuffer();
	void UpdateSpriteTile(int tile);
	void UpdateAllSpriteTiles();

	bool UpdateRowCount();
	void DrawTile(wxDC& dc, int x, int y, int tile);
	bool DrawTileAtPosition(wxDC& dc, int pos);
	void DrawSelectionBorders(wxDC& dc);
	void PaintBitmap(wxDC& dc);
	void InitialiseBrushesAndPens();
	void ForceRedraw();
	const Landstalker::Palette& GetSelectedPalette();
	void SetMouseCursor(wxStockCursor cursor);

	void FireEvent(const wxEventType& e, const std::string& data);
	void FireEvent(const wxEventType& e, int data = 0);

	int m_sprite_id = -1;
	int m_pixelsize;
	bool m_selectable;
	int m_selectedtile;
	int m_hoveredtile;
	int m_tilebase;

	int m_columns;
	int m_rows;
	int m_cellwidth;
	int m_cellheight;

	bool m_enabletilenumbers;
	bool m_enableborders;
	bool m_enableselection;
	bool m_enablehover;
	bool m_enablealpha;
	bool m_enablesubsprites;
	bool m_enablehitbox;

	std::string m_name;

	std::shared_ptr<Landstalker::Palette> m_pal;
	std::shared_ptr<Landstalker::SpriteFrame> m_sprite;
	std::shared_ptr<Landstalker::GameData> m_gd;
	std::shared_ptr<Landstalker::Tileset> m_tiles;

	int m_ctrlwidth;
	int m_ctrlheight;
	std::set<int> m_redraw_list;
	bool m_redraw_all;
	mutable std::vector<uint8_t> m_clipboard;
	mutable std::vector<uint8_t> m_swapbuffer;
	int m_pendingswap;

	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxBrush> m_dark_alpha_brush;
	std::unique_ptr<wxPen> m_border_pen;
	std::unique_ptr<wxPen> m_selected_border_pen;
	std::unique_ptr<wxPen> m_highlighted_border_pen;
	std::unique_ptr<wxBrush> m_highlighted_brush;
	std::unique_ptr<wxBitmap> m_stipple;
	std::unique_ptr<wxBitmap> m_dark_stipple;

	int m_selected_subsprite = -1;
	int m_hovered_subsprite = -1;

	wxStockCursor m_cursor = wxStockCursor::wxCURSOR_ARROW;

	static const int MAX_WIDTH = 32;
	static const int MAX_HEIGHT = 32;
	static const int ORIGIN_X = 16;
	static const int ORIGIN_Y = 16;
	static const int MAX_SIZE = 1024;

	wxMemoryDC m_memdc;
	wxBitmap m_bmp;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_SPRITE_FRAME_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_ACTIVATE, wxCommandEvent);

#endif // _SPRITE_FRAME_EDITOR_CTRL_H_
