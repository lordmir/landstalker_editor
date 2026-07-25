#ifndef _BLOCKSET_EDITOR_CTRL_H_
#define _BLOCKSET_EDITOR_CTRL_H_

#include <wx/wx.h>
#include <wx/vscroll.h>
#include <deque>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include <landstalker/tileset/Tileset.h>
#include <landstalker/palettes/Palette.h>
#include <landstalker/blockset/Block.h>
#include <landstalker/blockset/BlocksetCmp.h>
#include <landstalker/main/GameData.h>

class EditorFrame;

class BlocksetEditorCtrl : public wxVScrolledWindow
{
public:
	struct Position
	{
		int x;
		int y;
	};

	// BOX_SELECT drags out a rectangle of tile slots (spanning blocks) that can be moved
	// (Shift+drag to duplicate, Ctrl+drag to stamp continuously), cleared, copied/pasted
	// and have its H/V/P attribute bits toggled en masse. The shape modes drag out lines,
	// rectangles and circles of the draw tile across the slot grid; FILL flood-fills
	// contiguous identical slots with it.
	enum class Mode
	{
		BLOCK_SELECT,
		TILE_SELECT,
		PENCIL,
		BOX_SELECT,
		LINE,
		RECTANGLE_OUTLINE,
		RECTANGLE_FILLED,
		CIRCLE_OUTLINE,
		CIRCLE_FILLED,
		FILL
	};

	static bool IsShapeMode(Mode mode);
	static bool IsDrawMode(Mode mode);

	// How a box selection's H/V/P attribute operations behave.
	enum class AttrToggleMode
	{
		FlipMirror,  // tristate set/clear and mirror the slot layout (a true flip)
		TriState,    // tristate set/clear only; slots stay where they are
		Toggle       // invert each slot's own bit, no tristate logic
	};

	BlocksetEditorCtrl(EditorFrame* parent);
	~BlocksetEditorCtrl();

	void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
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
	std::shared_ptr<Landstalker::Tileset> GetTileset();
	std::shared_ptr<Landstalker::Palette> GetPalette();
	std::shared_ptr<std::vector<Landstalker::MapBlock>> GetBlocks();
	void SetMode(const Mode& mode);
	Mode GetMode() const;
	void SetDrawTile(const Landstalker::Tile& tile);
	Landstalker::Tile GetDrawTile() const;

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

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo();
	void Redo();
	// Groups several mutations (e.g. the two halves of a swap) into one undo entry.
	void BeginUndoGroup();
	void EndUndoGroup();

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
	Landstalker::MapBlock GetSelectedBlock() const;
	Landstalker::Tile GetSelectedTile() const;
	void SetSelectedBlock(const Landstalker::MapBlock& block);
	void SetSelectedTile(const Landstalker::Tile& tile);
	Landstalker::MapBlock GetHoveredBlock() const;
	Landstalker::Tile GetHoveredTile() const;
	void SetHoveredTile(const Landstalker::Tile& tile);
	Landstalker::MapBlock GetBlock(int index) const;
	Landstalker::Tile GetTile(int block_idx, int tile_idx) const;
	void SetBlock(int block, const Landstalker::MapBlock& new_block);
	void SetTile(int block_idx, int tile_idx, const Landstalker::Tile& new_tile);
	bool IsBlockIndexValid(int block_index) const;
	bool IsTileIndexValid(int tile_index) const;
	int GetControlBlockWidth() const;
	void ForceRedraw();

	// Box-selection keyboard interface; returns true if the key was consumed.
	bool HandleKeyDown(int key, int modifiers);
	// Box-selection operations the frame's toolbar buttons drive directly.
	bool HasBoxSelection() const;
	void ToggleBoxAttribute(Landstalker::TileAttributes::Attribute attr,
		AttrToggleMode toggle_mode = AttrToggleMode::FlipMirror);

private:
	// H/V/P key handling: box ops in BOX_SELECT, draw-tile flags in the draw modes.
	// Other modes fall through to the frame's per-tile handling.
	bool HandleAttributeKey(Landstalker::TileAttributes::Attribute attr, int modifiers);
	// Box selection (BOX_SELECT mode), in tile-slot coordinates: the blockset shown as a
	// grid of columns*2 x rows*2 tiles. A "floating" selection carries lifted or pasted
	// tiles that haven't been stamped back yet.
	enum class SelDrag
	{
		None,
		Marquee,
		Move,       // lift, clear source slots, stamp on release
		Duplicate,  // shift: lift without clearing, stamp on release
		Stamp       // ctrl: lift without clearing, stamp continuously while dragging
	};
	struct SlotClipboard
	{
		wxRect rect;
		std::vector<Landstalker::Tile> tiles;
	};
	wxPoint RawSlotFromPoint(const wxPoint& point) const;
	bool IsSlotValid(int sx, int sy) const;
	Landstalker::Tile GetSlotTile(int sx, int sy) const;
	bool SetSlotTile(int sx, int sy, const Landstalker::Tile& tile);
	void BeginBoxAction(int sx, int sy);
	void UpdateBoxDrag(int sx, int sy);
	void FinishBoxDrag();
	void CancelBoxDrag();
	bool CancelActiveBoxOp();
	void ClearBoxSelection(bool confirm_floating);
	void ConfirmBoxFloating();
	void DiscardBoxFloating();
	void LiftBoxSelection(bool erase_source);
	bool StampBoxFloating();
	void RenderBoxFloatBitmap();
	void ClearBoxCells();
	void AdjustBoxTileIds(int delta);
	void CopyBoxSelection();
	void CutBoxSelection();
	void PasteBoxCells();
	void SelectAllBoxCells();
	std::vector<Landstalker::Tile> ReadBoxRect(const wxRect& rect) const;
	void RefreshBoxRect(const wxRect& rect);
	// Shape tools (LINE/RECTANGLE_*/CIRCLE_* modes) and FILL, in tile-slot coordinates.
	void BeginShapeDrag(int sx, int sy);
	void UpdateShapeDrag(int sx, int sy);
	void CommitShapeDrag();
	void CancelShapeDrag();
	std::vector<wxPoint> MakeShapeSlots() const;
	void RefreshShapeRect();
	void DrawShapePreview(wxDC& dc);
	void FloodFillAt(int sx, int sy);
	// Continuous pencil strokes (PENCIL mode): slots paint while the button is held, with
	// coalesced mouse samples joined by line segments, as in the pixel editors.
	void BeginStroke(int sx, int sy);
	void StrokeTo(int sx, int sy);
	void EndStroke();
	void CancelStroke();
	void DrawBoxSelection(wxDC& dc);
	void ResetBoxSelectionState();
	void PushUndoState(Landstalker::Blockset&& state);
	void OnMouseUp(wxMouseEvent& evt);
	void OnCaptureLost(wxMouseCaptureLostEvent& evt);
	void RefreshStatusbar();
	virtual wxCoord OnGetRowHeight(size_t row) const override;

	bool UpdateRowCount();
	void RenderTilesBitmap();
	bool DrawBlockPriority(wxDC& dc, int x, int y, const Landstalker::MapBlock& block);
	void DrawOverlays(wxDC& dc, int s, int e, int c0, int c1);
	void DrawSelectionBorders(wxDC& dc);
	void RefreshBlock(int block);
	void PushUndo();
	void RestoreHistoryState(Landstalker::Blockset&& state);
	void ClearHistory();
	void InitialiseBrushesAndPens();
	Landstalker::Palette& GetSelectedPalette();
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

	std::shared_ptr<Landstalker::BlocksetEntry> m_blockset_entry;
	std::shared_ptr<Landstalker::Blockset> m_blocks;
	std::shared_ptr<Landstalker::Tileset> m_tileset;
	std::shared_ptr<Landstalker::Palette> m_pal;
	std::string m_pal_name;
	std::shared_ptr<Landstalker::GameData> m_gd;

	Mode m_mode;
	int m_columns;
	int m_rows;

	int m_pixelsize;
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

	Landstalker::Tile m_drawtile;

	// Box-selection state. Slots over missing blocks in a partial last row carry this
	// sentinel in lifted data and are never stamped.
	static constexpr uint16_t SEL_INVALID_TILE = 0xFFFF;
	wxRect m_sel_rect;                 // empty = no selection
	SelDrag m_sel_drag = SelDrag::None;
	wxPoint m_sel_anchor = { 0, 0 };   // marquee anchor, or grab offset within the rect
	bool m_sel_floating = false;
	bool m_sel_from_paste = false;     // paste floats persist after release until confirmed
	bool m_sel_op_changed = false;     // whether the current drag has altered the blockset
	std::vector<Landstalker::Tile> m_float_tiles;
	std::unique_ptr<wxBitmap> m_float_bmp;
	// Pre-drag state: pushed on commit, restored on cancel; null when no drag op is live.
	std::unique_ptr<Landstalker::Blockset> m_sel_snapshot;
	SlotClipboard m_slot_clipboard;

	// Shape-tool drag state: nothing touches the blockset until the drag commits on release.
	bool m_shape_active = false;
	wxPoint m_shape_anchor = { 0, 0 };
	wxPoint m_shape_current = { 0, 0 };

	// Pencil-stroke state: the whole stroke is one undo entry, pushed on release.
	bool m_stroke_active = false;
	bool m_stroke_changed = false;
	wxPoint m_stroke_last = { 0, 0 };
	std::unique_ptr<Landstalker::Blockset> m_stroke_snapshot;

	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxPen> m_priority_pen;
	std::unique_ptr<wxPen> m_border_pen;
	std::unique_ptr<wxPen> m_tile_border_pen;
	std::unique_ptr<wxPen> m_selected_border_pen;
	std::unique_ptr<wxPen> m_highlighted_border_pen;
	std::unique_ptr<wxBrush> m_highlighted_brush;
	std::unique_ptr<wxBitmap> m_stipple;
	EditorFrame* m_frame;

	// The blockset rendered once at native resolution; painting blits scaled from this.
	// Allocating a bitmap per tile per paint made opening the editor crawl.
	std::unique_ptr<wxBitmap> m_tiles_bmp;
	bool m_tiles_bmp_dirty = true;

	// Undo history as whole-blockset snapshots - a few kilobytes each.
	std::deque<Landstalker::Blockset> m_undo_stack;
	std::deque<Landstalker::Blockset> m_redo_stack;
	int m_undo_group_depth = 0;
	bool m_undo_group_pushed = false;

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
