#ifndef _SPRITE_FRAME_EDITOR_CTRL_H_
#define _SPRITE_FRAME_EDITOR_CTRL_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

#include <vector>
#include <memory>
#include <set>
#include <map>
#include <string>
#include <deque>

#include <landstalker/main/GameData.h>
#include <main/ImageBufferWx.h>
#include <landstalker/sprites/SpriteFrame.h>
#include <landstalker/palettes/Palette.h>

class SpriteFrameEditorCtrl : public wxHVScrolledWindow
{
public:
	// SELECT is the original behaviour (tile selection, Ctrl+click for subsprites).
	// SUBSPRITE turns the mouse into a subsprite editor: drag bodies to move, drag the
	// resize handles to grow/shrink (1-4 tiles each way), right-click to add/delete.
	// DRAW paints pixels directly onto the sprite with the active tool and colours.
	enum class Mode
	{
		SELECT,
		SUBSPRITE,
		DRAW
	};

	// The active drawing tool in DRAW mode. Shape tools anchor on mouse down, preview
	// while dragging and commit as a single undo step on release. Picker samples the
	// pixel under the cursor into the primary (left) or secondary (right) colour.
	// PixelSelect drags out a rectangle of pixels that can be moved, duplicated,
	// deleted, flipped and copied/pasted.
	enum class Tool
	{
		Pencil,
		Line,
		RectangleOutline,
		RectangleFilled,
		CircleOutline,
		CircleFilled,
		Fill,
		Picker,
		PixelSelect
	};

	SpriteFrameEditorCtrl(wxWindow* parent);
	~SpriteFrameEditorCtrl();

	void SetMode(Mode mode);
	Mode GetMode() const;
	void SetDrawTool(Tool tool);
	Tool GetDrawTool() const;

	void SetPrimaryColour(uint8_t colour);
	uint8_t GetPrimaryColour() const;
	void SetSecondaryColour(uint8_t colour);
	uint8_t GetSecondaryColour() const;

	bool IsPixelHoverValid() const;
	// Hovered pixel in canvas-wide pixel coordinates (tile = /8, offset = %8).
	wxPoint GetHoveredPixel() const;
	int GetColourAtPixel(const wxPoint& pixel) const;

	// Pixel-selection operations the frame's toolbar buttons drive directly.
	bool HasPixelSelection() const;
	void FlipSelection(bool horizontal);

	bool Save(wxString filename, bool compressed = false);
	bool Open(wxString filename, int sprite_id);
	bool Open(std::vector<uint8_t>& pixels, int sprite_id);
	bool Open(std::shared_ptr<Landstalker::SpriteFrame> frame, std::shared_ptr<Landstalker::Palette> pal, int sprite_id);
	void RedrawTiles(int index = -1);
	void UpdateSubSprites();
	// Replaces the frame's subsprite layout wholesale (e.g. from the optimiser), re-deriving the
	// tiles from the canvas. Pushes an undo state first, so the change can be undone like any edit.
	void ApplyOptimisedSubsprites(const std::vector<Landstalker::SpriteFrame::SubSprite>& subsprites);

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

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo();
	void Redo();
	// Snapshots the current state onto the undo stack. Call before every mutation of the
	// canvas tiles or the subsprite list, from whichever side (control or frame) makes it.
	void PushUndo();
	void ClearHistory();

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
	void OnMouseUp(wxMouseEvent& evt);
	void OnRightDown(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnCaptureLost(wxMouseCaptureLostEvent& evt);
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

	// Subsprite-mode mouse interaction.
	enum HandleEdge
	{
		EDGE_LEFT = 1,
		EDGE_RIGHT = 2,
		EDGE_TOP = 4,
		EDGE_BOTTOM = 8
	};
	struct SubSpriteHandle
	{
		int x = 0;
		int y = 0;
		int edges = 0;
	};
	std::vector<SubSpriteHandle> GetSubSpriteHandles();
	bool HitTestSubSpriteHandles(const wxPoint& logical, int& edges);
	wxPoint MouseToLogical(const wxPoint& point) const;
	void DoSubSpriteDrag(const wxPoint& logical);
	void DoSubSpriteResize(const wxPoint& logical);
	void UpdateSubSpriteCursor(const wxPoint& logical);
	void DrawSubSpriteHandles(wxDC& dc);
	void AddSubSpriteAt(int tx, int ty);
	void EndSubSpriteDrag();

	// Draw-mode painting, in canvas-wide pixel coordinates.
	void FlushSpriteTileSync();
	void StartDrawAction(const wxPoint& logical);
	void MouseDrawMove(const wxPoint& logical);
	bool PaintGlobalPixel(int gx, int gy, uint8_t colour, wxRect& damage);
	void CommitShape();
	void CancelShape();
	void CancelStroke();
	bool CancelActiveDrawOp();
	void FloodFillAt(int gx, int gy, uint8_t colour);
	std::vector<wxPoint> MakeShapePoints(Tool tool, const wxPoint& a, const wxPoint& b) const;
	wxRect GlobalPixelBoxToClient(const wxPoint& a, const wxPoint& b) const;
	void RefreshGlobalPixel(const wxPoint& pixel);
	void EndStroke();
	wxColour GetPaletteColour(int index) const;
	void DrawPixelCursor(wxDC& dc);
	void DrawShapePreview(wxDC& dc);
	void PickColourAt(int gx, int gy, bool secondary);
	void CycleColour(int delta, bool secondary);

	// Pixel selection (PixelSelect tool). The rectangle lives in canvas pixel coords.
	// A "floating" selection carries lifted or pasted content that hasn't been stamped
	// onto the canvas yet.
	enum class SelDrag
	{
		None,
		Marquee,
		Move,       // lift, clear source with secondary, stamp on release
		Duplicate,  // shift: lift without clearing, stamp on release
		Stamp       // ctrl: lift without clearing, stamp continuously while dragging
	};
	struct PixelClipboard
	{
		wxRect rect;
		std::vector<uint8_t> data;
	};
	void BeginSelectionAction(int gx, int gy);
	void UpdateSelectionDrag(int gx, int gy);
	void FinishSelectionDrag();
	void CancelSelectionDrag();
	void ClearPixelSelection(bool confirm_floating);
	void ConfirmFloating();
	void DiscardFloating();
	void LiftSelection(bool erase_source);
	bool StampFloating();
	void RenderFloatBitmap();
	void FillSelection(uint8_t colour);
	void CopySelection();
	void CutSelection();
	void PastePixels();
	void SelectAllPixels();
	void SelectHoveredCell();
	std::vector<uint8_t> ReadRect(const wxRect& rect) const;
	void RefreshSelectionRect(const wxRect& rect);
	void DrawPixelSelection(wxDC& dc);

	bool UpdateRowCount();
	void RenderTilesBitmap();
	void PatchTilesBitmap();
	void DrawOverlays(wxDC& dc, int sx, int ex, int sy, int ey);
	void RefreshTileRect(int tile);
	void DrawSelectionBorders(wxDC& dc);
	void InitialiseBrushesAndPens();
	void ForceRedraw();
	const Landstalker::Palette& GetSelectedPalette();
	void SetMouseCursor(wxStockCursor cursor);

	void FireEvent(const wxEventType& e, const std::string& data);
	void FireEvent(const wxEventType& e, int data = 0);

	int m_sprite_id = -1;
	int m_pixelsize;
	int m_selectedtile;
	int m_hoveredtile;

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

	Mode m_mode = Mode::SELECT;
	bool m_dragging_subsprite = false;
	bool m_resizing_subsprite = false;
	int m_drag_subsprite = -1;  // 1-based, like m_selected_subsprite
	int m_drag_edges = 0;
	wxPoint m_drag_offset = { 0, 0 };  // grab point relative to the subsprite origin, in tiles
	bool m_drag_undo_pushed = false;   // one undo entry per drag gesture

	wxStockCursor m_cursor = wxStockCursor::wxCURSOR_ARROW;

	// constexpr rather than const: std::min/std::clamp take references, which ODR-uses the
	// member, and a plain in-class const has no definition for the linker (GCC rejects it;
	// C++17 constexpr statics are implicitly inline).
	static constexpr int MAX_WIDTH = 32;
	static constexpr int MAX_HEIGHT = 32;
	static constexpr int ORIGIN_X = 16;
	static constexpr int ORIGIN_Y = 16;
	static constexpr int MAX_SIZE = 1024;

	// All sprite tiles rendered once at native resolution (out-of-sprite tiles darkened);
	// painting blits scaled from this. Allocating a bitmap per tile per paint made
	// opening the editor crawl.
	std::unique_ptr<wxBitmap> m_tiles_bmp;
	bool m_tiles_bmp_dirty = true;

	// Undo history. The 32x32 canvas tileset is the master copy of the artwork (the sprite's
	// own tileset is re-derived from it on every edit), so a snapshot of the canvas bits plus
	// the subsprite layout captures the whole editable state.
	struct UndoState
	{
		std::vector<uint8_t> tiles;
		std::vector<Landstalker::SpriteFrame::SubSprite> subsprites;
	};
	UndoState MakeUndoState() const;
	void RestoreUndoState(const UndoState& state);
	void PushUndoState(UndoState&& state);
	std::deque<UndoState> m_undo_stack;
	std::deque<UndoState> m_redo_stack;

	// Draw-mode state.
	Tool m_tool = Tool::Pencil;
	bool m_drawing = false;
	bool m_secondary_active = false;
	uint8_t m_primary_colour = 1;
	uint8_t m_secondary_colour = 0;
	wxPoint m_hoveredpixel = { -1, -1 };
	// Previous stroke point in canvas-wide pixel coordinates, or (-1,-1) between strokes.
	wxPoint m_last_drawn = { -1, -1 };
	// In-progress shape drag, in canvas-wide pixel coordinates.
	bool m_shape_active = false;
	bool m_shape_secondary = false;
	wxPoint m_shape_start = { -1, -1 };
	wxPoint m_shape_end = { -1, -1 };
	// True while the current stroke has painted something; the change event fires once on
	// stroke end rather than per pixel.
	bool m_stroke_dirty = false;
	// Tiles painted since the last sync into the sprite's own tileset; synced once per
	// operation rather than per pixel (each sync copies the whole tile).
	std::set<int> m_pending_sync;

	// Pixel-selection state.
	static constexpr uint8_t SEL_TRANSPARENT = 0xFF;  // out-of-sprite marker in lifted data
	wxRect m_sel_rect;                 // empty = no selection
	SelDrag m_sel_drag = SelDrag::None;
	wxPoint m_sel_anchor = { 0, 0 };   // marquee anchor, or grab offset within the rect
	bool m_sel_floating = false;
	bool m_sel_from_paste = false;     // paste floats persist after release until confirmed
	bool m_sel_op_changed = false;     // whether the current drag has altered the canvas
	std::vector<uint8_t> m_float_data;
	std::unique_ptr<wxBitmap> m_float_bmp;
	UndoState m_sel_snapshot;          // pre-drag state, pushed on commit / restored on cancel
	bool m_sel_snapshot_valid = false;
	PixelClipboard m_pixel_clipboard;
	// Snapshot taken when a stroke starts, pushed to the undo stack on its first painted pixel.
	UndoState m_stroke_snapshot;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_SPRITE_FRAME_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_SPRITE_FRAME_ACTIVATE, wxCommandEvent);
// Fired when the picker samples a colour: int = colour index | 0x100 if secondary.
wxDECLARE_EVENT(EVT_SPRITE_FRAME_COLOUR_PICK, wxCommandEvent);

#endif // _SPRITE_FRAME_EDITOR_CTRL_H_
