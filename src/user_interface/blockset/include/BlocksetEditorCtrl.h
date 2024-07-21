#ifndef _BLOCKSET_EDITOR_CTRL_H_
#define _BLOCKSET_EDITOR_CTRL_H_

#include <wx/wx.h>
#include <wx/vscroll.h>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include <landstalker/tileset/include/Tileset.h>
#include <landstalker/palettes/include/Palette.h>
#include <landstalker/blockset/include/Block.h>
#include <landstalker/blockset/include/BlocksetCmp.h>
#include <landstalker/main/include/GameData.h>

class EditorFrame;

class BlocksetEditorCtrl : public wxVScrolledWindow
{
public:
	struct Position
	{
		int x;
		int y;
	};

	enum class Mode
	{
		BLOCK_SELECT,
		TILE_SELECT,
		PENCIL
	};

	BlocksetEditorCtrl(EditorFrame* parent);
	~BlocksetEditorCtrl();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	bool Open(const std::string& name);
	bool OpenRoom(uint16_t num);
	void RedrawTiles(int index = -1);
	void RedrawBlock(int index = -1);

	void SetPixelSize(int n);
	int GetPixelSize() const;
	void SetActivePalette(const std::string& name);
	std::string GetActivePalette() const;
	int GetBlockmapSize() const;
	int GetBlockWidth() const;
	int GetBlockHeight() const;
	std::shared_ptr<Tileset> GetTileset();
	std::shared_ptr<Palette> GetPalette();
	std::shared_ptr<std::vector<MapBlock>> GetBlocks();
	void SetMode(const Mode& mode);
	Mode GetMode() const;
	void SetDrawTile(const Tile& tile);
	Tile GetDrawTile();

	void ToggleHFlip(int block, int tile);
	void ToggleVFlip(int block, int tile);
	void TogglePriority(int block, int tile);
	void ToggleSelectedHFlip();
	void ToggleSelectedVFlip();
	void ToggleSelectedPriority();
	void SetSelectedHFlip(bool hflip);
	void SetSelectedVFlip(bool vflip);
	void SetSelectedPriority(bool priority);
	bool GetSelectedHFlip() const;
	bool GetSelectedVFlip() const;
	bool GetSelectedPriority() const;

	bool GetTileNumbersEnabled() const;
	void SetTileNumbersEnabled(bool enabled);
	bool GetSelectionEnabled() const;
	void SetSelectionEnabled(bool enabled);
	bool GetHoverEnabled() const;
	void SetHoverEnabled(bool enabled);
	bool GetAlphaEnabled() const;
	void SetAlphaEnabled(bool enabled);
	bool GetBordersEnabled() const;
	void SetBordersEnabled(bool enabled);

	bool InsertBlock(int row);
	bool DeleteBlock(int row);

	bool IsBlockSelectionValid() const;
	bool IsBlockHoverValid() const;
	bool IsTileSelectionValid() const;
	bool IsTileHoverValid() const;
	uint16_t GetBlockSelection() const;
	uint16_t GetBlockHover() const;
	void SetBlockSelection(int block);
	void SetTileHover(int block, int tile);
	void SetTileSelection(int block, int tile);
	uint16_t GetTileSelection() const;
	uint16_t GetTileHover() const;
	MapBlock GetSelectedBlock() const;
	Tile GetSelectedTile() const;
	void SetSelectedBlock(const MapBlock& block);
	void SetSelectedTile(const Tile& tile);
	MapBlock GetHoveredBlock() const;
	Tile GetHoveredTile() const;
	void SetHoveredTile(const Tile& tile);
	MapBlock GetBlock(int index) const;
	Tile GetTile(int block_idx, int tile_idx) const;
	void SetBlock(int block, const MapBlock& new_block);
	void SetTile(int block_idx, int tile_idx, const Tile& new_tile);
	bool IsBlockIndexValid(int block_index) const;
	bool IsTileIndexValid(int tile_index) const;
	int GetControlBlockWidth() const;
	void ForceRedraw();

private:
	void RefreshStatusbar();
	virtual wxCoord OnGetRowHeight(size_t row) const override;

	bool UpdateRowCount();
	bool DrawTile(wxDC& dc, int x, int y, const Tile& tile);
	void DrawTilePixels(wxDC& dc, int x, int y, const Tile& tile);
	bool DrawBlock(wxDC& dc, int x, int y, const MapBlock& block);
	bool DrawBlockPriority(wxDC& dc, int x, int y, const MapBlock& block);
	void DrawSelectionBorders(wxDC& dc);
	void PaintBitmap(wxDC& dc);
	void InitialiseBrushesAndPens();
	Palette& GetSelectedPalette();
	Position ToBlockPosition(int index) const;
	int ToBlockIndex(const Position& tp) const;
	Position ToTilePosition(int index);
	int ToTileIndex(const Position& tp);
	int ConvertXYToBlockIdx(const wxPoint& point) const;
	int ConvertXYToTileIdx(const wxPoint& point) const;

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);

	void FireUpdateStatusEvent(const std::string& data, int pane = 0);
	void FireEvent(const wxEventType& e, const std::string& data);
	void FireTilesetEvent(const wxEventType& e, const std::string& data);
	void FireBlockEvent(const wxEventType& e, const std::string& data);

	std::shared_ptr<BlocksetEntry> m_blockset_entry;
	std::shared_ptr<Blockset> m_blocks;
	std::shared_ptr<Tileset> m_tileset;
	std::shared_ptr<Palette> m_pal;
	std::string m_pal_name;
	std::shared_ptr<GameData> m_gd;

	Mode m_mode;
	int m_columns;
	int m_rows;

	int m_pixelsize;
	bool m_selectable;
	int m_selectedblock;
	int m_hoveredblock;
	int m_selectedtile;
	int m_hoveredtile;

	const int block_width;
	const int block_height;
	int m_cellwidth;
	int m_cellheight;
	int m_tilewidth;
	int m_tileheight;
	int m_ctrlwidth;
	int m_ctrlheight;

	bool m_enableblocknumbers;
	bool m_enabletilenumbers;
	bool m_enableborders;
	bool m_enabletileborders;
	bool m_enableselection;
	bool m_enablehover;
	bool m_enablealpha;

	bool m_redraw_all;
	std::set<int> m_redraw_list;
	Tile m_drawtile;

	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxPen> m_priority_pen;
	std::unique_ptr<wxPen> m_border_pen;
	std::unique_ptr<wxPen> m_tile_border_pen;
	std::unique_ptr<wxPen> m_selected_border_pen;
	std::unique_ptr<wxPen> m_highlighted_border_pen;
	std::unique_ptr<wxBrush> m_highlighted_brush;
	std::unique_ptr<wxBitmap> m_stipple;
	EditorFrame* m_frame;

	wxMemoryDC m_memdc;
	wxBitmap m_bmp;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_BLOCK_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_BLOCK_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_BLOCK_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_BLOCK_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_BLOCK_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_BLOCK_ACTIVATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILE_SELECT, wxCommandEvent);

#endif // _BLOCKSET_EDITOR_CTRL_H_
