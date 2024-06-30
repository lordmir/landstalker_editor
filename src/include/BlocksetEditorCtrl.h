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

#include "Tileset.h"
#include "Palette.h"
#include "Block.h"
#include "BlocksetCmp.h"
#include "GameData.h"

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

	void SetColour(int c);
	bool Save(const wxString& filename);
	bool Open(const wxString& filename);
	bool Open(const std::vector<uint8_t>& data);
	bool Open(const std::string& name);
	bool OpenRoom(uint16_t num);
	bool New();
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
	uint16_t GetTileSelection() const;
	uint16_t GetTileHover() const;
	MapBlock GetSelectedBlock() const;
	Tile GetSelectedTile() const;
	void SetSelectedBlock(const MapBlock& block);
	void SetSelectedTile(const Tile& tile);
	MapBlock GetHoveredBlock() const;
	void SetHoveredBlock(const MapBlock& tile);
	Tile GetHoveredTile() const;
	void SetHoveredTile(const Tile& tile);
	MapBlock GetBlockAtPosition(const Position& block) const;
	Tile GetTileAtPosition(const Position& block, const Position& tile) const;
	void SetBlockAtPosition(const Position& block, const MapBlock& new_block);
	void SetTileAtPosition(const Position& block, const Position& tile, const Tile& new_tile);
	bool IsPositionValid(const Position& tp) const;
private:
	void RefreshStatusbar();
	virtual wxCoord OnGetRowHeight(size_t row) const override;

	bool UpdateRowCount();
	bool DrawTile(wxDC& dc, int x, int y, const Tile& tile);
	void DrawTilePixels(wxDC& dc, int x, int y, const Tile& tile);
	bool DrawBlock(wxDC& dc, int x, int y, int idx, const MapBlock& block);
	void DrawSelectionBorders(wxDC& dc);
	void PaintBitmap(wxDC& dc);
	void InitialiseBrushesAndPens();
	void ForceRedraw();
	Palette& GetSelectedPalette();
	Position ToBlockPosition(int index) const;
	int ToBlockIndex(const Position& tp) const;
	Position ToTilePosition(int index);
	int ToTileIndex(const Position& tp);
	int ConvertXYToBlockIdx(const wxPoint& point) const;
	Position ConvertXYToBlockPos(const wxPoint& point) const;
	int ConvertXYToTileIdx(const wxPoint& point) const;
	Position ConvertXYToTilePos(const wxPoint& point) const;
	void SelectBlock(const Position& tp);
	void SelectTile(const Position& tp);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);

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

#endif // _BLOCKSET_EDITOR_CTRL_H_
