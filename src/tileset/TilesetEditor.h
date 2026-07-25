#ifndef _TILESETEDITOR_H_
#define _TILESETEDITOR_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

#include <vector>
#include <deque>
#include <functional>
#include <memory>
#include <set>
#include <map>
#include <string>

#include <landstalker/tileset/Tileset.h>
#include <landstalker/palettes/Palette.h>
#include <main/ImageBufferWx.h>
#include <landstalker/main/GameData.h>

class TilesetEditor : public wxVScrolledWindow
{
public:
	// The active drawing tool when drawing mode is enabled. Shape tools anchor on mouse
	// down, preview while dragging and commit as a single undo step on release. Picker
	// samples the pixel under the cursor into the primary (left) or secondary (right)
	// colour. PixelSelect drags out a rectangle of pixels that can be moved, duplicated,
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

	TilesetEditor(wxWindow* parent);
	TilesetEditor(wxWindow* parent, std::shared_ptr<Landstalker::Tileset>);
	~TilesetEditor();

	void SetGameData(std::shared_ptr<Landstalker::GameData> gd);
	std::shared_ptr<Landstalker::Tileset> GetTileset();
	bool Save(wxString filename, bool compressed = false);
	bool Open(wxString filename, bool compressed = false, int tile_width = 8, int tile_height = 8, int tile_bitdepth = 4);
	bool Open(std::shared_ptr<Landstalker::Tileset> ts);
	bool Open(std::vector<uint8_t>& pixels, bool uses_compression = false, int tile_width = 8, int tile_height = 8, int tile_bitdepth = 4);
	bool New(int r, int c);
	void RedrawTiles(int index = -1);
	void ForceRedraw();

	void SetPixelSize(int n);
	int GetPixelSize() const;
	int GetTilemapSize() const;
	bool GetCompressed() const;
	void SetColourMap(const std::vector<uint8_t>& cmap);
	std::vector<uint8_t> GetColourMap() const;
	void SetActivePalette(const std::string& name);
	std::string GetActivePalette() const;
	std::array<bool, 16> GetLockedColours() const;

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
	bool GetDrawingEnabled() const;
	void SetDrawingEnabled(bool enabled);
	Tool GetDrawTool() const;
	void SetDrawTool(Tool tool);
	bool GetPixelGridEnabled() const;
	void SetPixelGridEnabled(bool enabled);

	void SetPrimaryColour(uint8_t colour);
	uint8_t GetPrimaryColour() const;
	void SetSecondaryColour(uint8_t colour);
	uint8_t GetSecondaryColour() const;
	// Restricts drawing on a tile to its leftmost columns, for tilesets whose tiles are wider
	// than the artwork in them - the end credit font's variable width glyphs. A returned limit
	// of zero or less allows the full tile width.
	void SetDrawWidthLimiter(std::function<int(int)> limiter);

	bool IsPixelHoverValid() const;
	wxPoint GetHoveredPixel() const;
	int GetColourAtPixel(const Landstalker::Tile& tile, const wxPoint& point) const;
	int GetColour(int index) const;

	bool IsSelectionValid() const;
	Landstalker::Tile GetSelectedTile() const;
	bool IsHoverValid() const;
	Landstalker::Tile GetHoveredTile() const;

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo();
	void Redo();

	// Pixel-selection operations the frame's toolbar buttons drive directly.
	bool HasPixelSelection() const;
	void FlipSelection(bool horizontal);

	void SelectTile(int tile);
	void InsertTileBefore(const Landstalker::Tile& tile);
	void InsertTileAfter(const Landstalker::Tile& tile);
	void InsertTilesAtEnd(int count = 1);
	void DeleteTileAt(const Landstalker::Tile& tile);
	void CutTile(const Landstalker::Tile& tile);
	void CopyTile(const Landstalker::Tile& tile) const;
	void PasteTile(const Landstalker::Tile& tile);
	void SwapTile(const Landstalker::Tile& tile);
	void EditTile(const Landstalker::Tile& tile);
	bool IsClipboardEmpty() const;

private:
	virtual wxCoord OnGetRowHeight(size_t row) const override;

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	bool HandleDrawKey(wxKeyEvent& evt);
	void CycleColour(int delta, bool secondary);
	void PickColourAt(int gx, int gy, bool secondary);
	void OnMouseDown(wxMouseEvent& evt);
	void OnRightDown(wxMouseEvent& evt);
	void OnMouseUp(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnCaptureLost(wxMouseCaptureLostEvent& evt);
	void OnTilesetFocus(wxFocusEvent& evt);
	int  ConvertXYToTile(const wxPoint& point);
	bool ConvertXYToTilePixel(const wxPoint& point, int& tile, wxPoint& pixel) const;
	void MouseDraw(const wxPoint& mousepos);
	bool PaintGlobalPixel(int gx, int gy, uint8_t colour, wxRect& damage);
	void StartDrawAction(const wxPoint& mousepos);
	void CommitShape();
	void CancelShape();
	void CancelStroke();
	bool CancelActiveDrawOp();
	void FloodFillAt(int gx, int gy, uint8_t colour);

	// Pixel selection (PixelSelect tool), in tileset-wide pixel coordinates. A "floating"
	// selection carries lifted or pasted content that hasn't been stamped back yet.
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
	int GetColourAtGlobalPixel(int gx, int gy) const;
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
	void ResetSelectionState();
	std::vector<wxPoint> MakeShapePoints(Tool tool, const wxPoint& a, const wxPoint& b) const;
	wxRect GlobalPixelBoxToClient(const wxPoint& a, const wxPoint& b) const;
	void RefreshTileRect(int tile);
	void RefreshPixelRect(int tile, const wxPoint& pixel);
	int  ValidateColour(int colour) const;
	wxColour GetPaletteColour(int index) const;

	bool UpdateRowCount();
	void RenderTilesetBitmap();
	void UpdateTilesetBitmap();
	void DrawGrid(wxDC& dest, const wxRect& damage);
	void DrawSelectionBorders(wxDC& dc);
	void DrawPixelGrid(wxDC& dc, const wxRect& damage);
	void DrawGlyphLimitOverlay(wxDC& dc, const wxRect& damage);
	void DrawPixelCursor(wxDC& dc);
	void DrawShapePreview(wxDC& dc);
	void InitialiseBrushesAndPens();

	void PushUndo(std::vector<uint8_t>&& state);
	void RestoreHistoryState(std::vector<uint8_t>&& state);
	void ClearHistory();

	void FireEvent(const wxEventType& e, const std::string& data);


	int m_pixelsize;
	int m_selectedtile;
	int m_hoveredtile;

	int m_columns;
	int m_rows;
	int m_tilewidth;
	int m_tileheight;
	int m_cellwidth;
	int m_cellheight;

	bool m_enabletilenumbers;
	bool m_enableborders;
	bool m_enableselection;
	bool m_enablehover;
	bool m_enablealpha;
	bool m_enabledrawing;
	bool m_enablepixelgrid;

	bool m_drawing;
	bool m_secondary_active;
	uint8_t m_primary_colour;
	uint8_t m_secondary_colour;
	wxPoint m_hoveredpixel;
	// Previous stroke point in tileset-wide pixel coordinates, or (-1,-1) between strokes.
	wxPoint m_last_drawn;
	Tool m_tool;
	// In-progress shape drag, in tileset-wide pixel coordinates.
	bool m_shape_active;
	bool m_shape_secondary;
	wxPoint m_shape_start;
	wxPoint m_shape_end;
	// True while the current stroke has painted something; the change event fires once on
	// stroke end rather than per pixel.
	bool m_stroke_dirty;
	// Undo history as whole-tileset snapshots: a few kilobytes each, which buys a much
	// simpler implementation than per-operation deltas.
	std::deque<std::vector<uint8_t>> m_undo_stack;
	std::deque<std::vector<uint8_t>> m_redo_stack;
	// Snapshot taken when a stroke starts, pushed to the undo stack on its first painted pixel.
	std::vector<uint8_t> m_stroke_snapshot;
	std::function<int(int)> m_draw_width_limiter;

	// Pixel-selection state.
	static constexpr uint8_t SEL_TRANSPARENT = 0xFF;  // missing/limited pixels in lifted data
	wxRect m_sel_rect;                 // empty = no selection
	SelDrag m_sel_drag = SelDrag::None;
	wxPoint m_sel_anchor = { 0, 0 };   // marquee anchor, or grab offset within the rect
	bool m_sel_floating = false;
	bool m_sel_from_paste = false;     // paste floats persist after release until confirmed
	bool m_sel_op_changed = false;     // whether the current drag has altered the tileset
	std::vector<uint8_t> m_float_data;
	std::unique_ptr<wxBitmap> m_float_bmp;
	std::vector<uint8_t> m_sel_snapshot;  // pre-drag state: pushed on commit, restored on cancel
	bool m_sel_snapshot_valid = false;
	PixelClipboard m_pixel_clipboard;

	std::string m_name;

	std::string m_palette;
	std::shared_ptr<Landstalker::Tileset> m_tileset = nullptr;
	std::string m_selected_palette_name;
	std::shared_ptr<Landstalker::PaletteEntry> m_selected_palette_entry;
	std::shared_ptr<Landstalker::Palette> m_selected_palette;
	std::shared_ptr<Landstalker::GameData> m_gd = nullptr;
	int m_ctrlwidth;
	int m_ctrlheight;
	std::set<int> m_redraw_list;
	bool m_redraw_all;
	mutable std::vector<uint8_t> m_clipboard;
	int m_pendingswap;

	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxPen> m_border_pen;
	std::unique_ptr<wxPen> m_selected_border_pen;
	std::unique_ptr<wxPen> m_highlighted_border_pen;
	std::unique_ptr<wxBrush> m_highlighted_brush;
	std::unique_ptr<wxBitmap> m_stipple;
	// The tileset rendered at native (unzoomed) resolution; the paint handler scales the
	// visible band out of this in a single blit. Kept small on purpose - a zoomed backing
	// bitmap grows with the square of the zoom factor and makes GDI crawl.
	std::unique_ptr<wxBitmap> m_tiles_bmp;

	ImageBufferWx m_buf;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_TILESET_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_ACTIVATE, wxCommandEvent);
// Fired when the pen colours change from the keyboard: string = colour | 0x100 if secondary.
wxDECLARE_EVENT(EVT_TILESET_COLOUR_PICK, wxCommandEvent);

#endif // _TILESETEDITOR_H_
