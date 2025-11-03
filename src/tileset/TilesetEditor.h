#ifndef _TILESETEDITOR_H_
#define _TILESETEDITOR_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

#include <vector>
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

	bool IsSelectionValid() const;
	Landstalker::Tile GetSelectedTile() const;
	bool IsHoverValid() const;
	Landstalker::Tile GetHoveredTile() const;

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
	void OnMouseDown(wxMouseEvent& evt);
	void OnDoubleClick(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnTilesetFocus(wxFocusEvent& evt);
	int  ConvertXYToTile(const wxPoint& point);

	bool UpdateRowCount();
	void DrawAllTiles(wxDC& dest);
	void DrawTileList(wxDC& dest);
	void DrawGrid(wxDC& dest);
	void DrawSelectionBorders(wxDC& dc);
	void PaintBitmap(wxDC& src, wxDC& dst);
	void InitialiseBrushesAndPens();

	void FireEvent(const wxEventType& e, const std::string& data);


	int m_pixelsize;
	bool m_selectable;
	int m_selectedtile;
	int m_hoveredtile;
	int m_tilebase;

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
	std::unique_ptr<wxBitmap> m_tiles_bmp;

	ImageBufferWx m_buf;
	wxBitmap m_bmp;
	wxBitmap m_bg_bmp;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_TILESET_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_ACTIVATE, wxCommandEvent);

#endif // _TILESETEDITOR_H_
