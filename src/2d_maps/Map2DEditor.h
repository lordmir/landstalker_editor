#ifndef _MAP2DEDITOR_H_
#define _MAP2DEDITOR_H_

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
#include <landstalker/main/DataTypes.h>
#include <landstalker/main/GameData.h>
#include <landstalker/2d_maps/Tilemap2DRLE.h>

class Map2DEditor : public wxHVScrolledWindow
{
public:

	struct TilePosition
	{
		int x;
		int y;
	};

	// BOX_SELECT drags out a rectangle of map cells that can be moved (Shift+drag to
	// duplicate, Ctrl+drag to stamp continuously), cleared, copied/pasted and have its
	// H/V/P attribute bits toggled en masse. The shape modes drag out lines, rectangles
	// and circles of the draw tile; FILL flood-fills contiguous identical cells with it.
	enum class Mode
	{
		SELECT,
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
		FlipMirror,  // tristate set/clear and mirror the cell layout (a true flip)
		TriState,    // tristate set/clear only; cells stay where they are
		Toggle       // invert each cell's own bit, no tristate logic
	};

	Map2DEditor(wxWindow* parent);
	~Map2DEditor();

	bool Save(const wxString& filename, Landstalker::Tilemap2D::Compression compression = Landstalker::Tilemap2D::Compression::NONE, int base = 0);
	bool Open(const wxString& filename, Landstalker::Tilemap2D::Compression compression = Landstalker::Tilemap2D::Compression::NONE, int base = 0);
	bool Open(const std::vector<Landstalker::Tile>& map, int width = 0, int height = 0, int base = 0);
	bool Open(const std::vector<uint8_t>& map, Landstalker::Tilemap2D::Compression compression = Landstalker::Tilemap2D::Compression::NONE, int width = 0, int height = 0, int base = 0);
	bool Open(std::shared_ptr<Landstalker::Tilemap2DEntry> map);
	bool New(int width, int height, int base = 0);
	void RedrawTiles(int index = -1);
	void RedrawMapTile(const TilePosition& tp);
	void RedrawMapTile(int index = -1);
	void RedrawAll();

	void SetGameData(std::shared_ptr<Landstalker::GameData> gd) { m_g = gd; }
	void ClearGameData() { m_g = nullptr; }

	void SetPixelSize(int n);
	int GetPixelSize() const;
	void SetActivePalette(const std::string& name);
	std::string GetActivePalette() const;
	void SetTileset(const std::string& name);
	std::string GetTileset() const;
	int GetTilemapWidth() const;
	int GetTilemapHeight() const;
	std::shared_ptr<Landstalker::Tileset> GetTileset();
	std::shared_ptr<Landstalker::Palette> GetPalette();
	std::shared_ptr<Landstalker::Tilemap2D> GetMap();
	void SetMode(const Mode& mode);
	Mode GetMode() const;
	void SetDrawTile(const Landstalker::Tile& tile);
	Landstalker::Tile GetDrawTile();

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

	bool InsertRow(int row);
	bool DeleteRow(int row);
	bool InsertColumn(int column);
	bool DeleteColumn(int column);

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo();
	void Redo();

	bool IsSelectionValid() const;
	bool IsHoverValid() const;
	TilePosition GetSelection() const;
	void SetSelection(const TilePosition& tp);
	TilePosition GetHover() const;
	Landstalker::Tile GetSelectedTile() const;
	void SetSelectedTile(const Landstalker::Tile& tile);
	Landstalker::Tile GetHoveredTile() const;
	void SetHoveredTile(const Landstalker::Tile& tile);
	Landstalker::Tile GetTileAtPosition(const TilePosition& tp) const;
	void SetTileAtPosition(const TilePosition& tp, const Landstalker::Tile& tile);
	bool IsPositionValid(const TilePosition& tp) const;
	// Box-selection keyboard interface; returns true if the key was consumed.
	bool HandleKeyDown(int key, int modifiers);
	// Box-selection operations the frame's toolbar buttons drive directly.
	bool HasBoxSelection() const;
	void ToggleBoxAttribute(Landstalker::TileAttributes::Attribute attr,
		AttrToggleMode toggle_mode = AttrToggleMode::FlipMirror);
private:
	// H/V/P key handling: box ops in BOX_SELECT, draw-tile flags in the draw modes,
	// the selected cell's flags in SELECT.
	bool HandleAttributeKey(Landstalker::TileAttributes::Attribute attr, int modifiers);
	virtual wxCoord OnGetRowHeight(size_t row) const override;
	virtual wxCoord OnGetColumnWidth(size_t row) const override;

	void DrawTile(wxDC& dc, int x, int y, const Landstalker::Tile& tile);
	void RenderTilesBitmap();
	void RefreshMapTile(int tile);
	void DrawOverlays(wxDC& dc, int sx, int ex, int sy, int ey);
	void DrawSelectionBorders(wxDC& dc);
	void PushUndo();
	void PushUndoState(Landstalker::Tilemap2D&& state);
	void RestoreHistoryState(Landstalker::Tilemap2D&& state);
	void ClearHistory();

	// Box selection (BOX_SELECT mode), in map cell coordinates. A "floating" selection
	// carries lifted or pasted cells that haven't been stamped back yet.
	enum class SelDrag
	{
		None,
		Marquee,
		Move,       // lift, clear source cells, stamp on release
		Duplicate,  // shift: lift without clearing, stamp on release
		Stamp       // ctrl: lift without clearing, stamp continuously while dragging
	};
	struct CellClipboard
	{
		wxRect rect;
		std::vector<Landstalker::Tile> tiles;
	};
	wxPoint RawCellFromPoint(const wxPoint& point) const;
	void BeginBoxAction(int cx, int cy);
	void UpdateBoxDrag(int cx, int cy);
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
	// Shape tools (LINE/RECTANGLE_*/CIRCLE_* modes) and FILL, in map cell coordinates.
	void BeginShapeDrag(int cx, int cy);
	void UpdateShapeDrag(int cx, int cy);
	void CommitShapeDrag();
	void CancelShapeDrag();
	std::vector<wxPoint> MakeShapeCells() const;
	void RefreshShapeRect();
	void DrawShapePreview(wxDC& dc);
	void FloodFillAt(int cx, int cy);
	// Continuous pencil strokes (PENCIL mode): cells paint while the button is held, with
	// coalesced mouse samples joined by line segments, as in the pixel editors.
	void BeginStroke(int cx, int cy);
	void StrokeTo(int cx, int cy);
	void EndStroke();
	void CancelStroke();
	void RefreshBoxRect(const wxRect& rect);
	void DrawBoxSelection(wxDC& dc);
	void ResetBoxSelectionState();
	void InitialiseBrushesAndPens();
	void ForceRedraw();
	std::shared_ptr<Landstalker::Palette> GetSelectedPalette();
	TilePosition ToPosition(int index) const;
	int ToIndex(const TilePosition& tp) const;
	int ConvertXYToTileIdx(const wxPoint& point) const;
	TilePosition ConvertXYToTilePos(const wxPoint& point) const;
	void SelectTile(const TilePosition& tp);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);
	void OnMouseUp(wxMouseEvent& evt);
	void OnCaptureLost(wxMouseCaptureLostEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);

	void FireEvent(const wxEventType& e, const std::string& data);
	void FireTilesetEvent(const wxEventType& e, const std::string& data);

	std::shared_ptr<Landstalker::Tilemap2D> m_map;
	std::shared_ptr<Landstalker::Tilemap2DEntry> m_map_entry;
	std::shared_ptr<Landstalker::Tileset> m_tileset;
	std::shared_ptr<Landstalker::TilesetEntry> m_tileset_entry;
	std::shared_ptr<Landstalker::GameData> m_g;
	std::shared_ptr<Landstalker::PaletteEntry> m_active_palette;
	Landstalker::Palette m_default_palette;
	// The map rendered once at native resolution; painting blits scaled from this.
	// Allocating a bitmap per tile per paint made opening the editor crawl.
	std::unique_ptr<wxBitmap> m_tiles_bmp;
	bool m_tiles_bmp_dirty = true;

	// Undo history as whole-tilemap snapshots - a few kilobytes each.
	std::deque<Landstalker::Tilemap2D> m_undo_stack;
	std::deque<Landstalker::Tilemap2D> m_redo_stack;

	Mode m_mode;

	int m_pixelsize;
	int m_selectedtile;
	int m_hoveredtile;

	bool m_enabletilenumbers;
	bool m_enableborders;
	bool m_enableselection;
	bool m_enablehover;
	bool m_enablealpha;

	Landstalker::Tile m_drawtile;

	// Box-selection state.
	wxRect m_sel_rect;                 // empty = no selection
	SelDrag m_sel_drag = SelDrag::None;
	wxPoint m_sel_anchor = { 0, 0 };   // marquee anchor, or grab offset within the rect
	bool m_sel_floating = false;
	bool m_sel_from_paste = false;     // paste floats persist after release until confirmed
	bool m_sel_op_changed = false;     // whether the current drag has altered the map
	std::vector<Landstalker::Tile> m_float_tiles;
	std::unique_ptr<wxBitmap> m_float_bmp;
	// Pre-drag state: pushed on commit, restored on cancel; null when no drag op is live.
	std::unique_ptr<Landstalker::Tilemap2D> m_sel_snapshot;
	CellClipboard m_cell_clipboard;

	// Shape-tool drag state: nothing touches the map until the drag commits on release.
	bool m_shape_active = false;
	wxPoint m_shape_anchor = { 0, 0 };
	wxPoint m_shape_current = { 0, 0 };

	// Pencil-stroke state: the whole stroke is one undo entry, pushed on release.
	bool m_stroke_active = false;
	bool m_stroke_changed = false;
	wxPoint m_stroke_last = { 0, 0 };
	std::unique_ptr<Landstalker::Tilemap2D> m_stroke_snapshot;

	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxPen> m_border_pen;
	std::unique_ptr<wxPen> m_selected_border_pen;
	std::unique_ptr<wxPen> m_highlighted_border_pen;
	std::unique_ptr<wxBrush> m_highlighted_brush;
	std::unique_ptr<wxBitmap> m_stipple;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_MAP_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_TILE_PICK, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_ACTIVATE, wxCommandEvent);

#endif // _MAP2DEDITOR_H_
