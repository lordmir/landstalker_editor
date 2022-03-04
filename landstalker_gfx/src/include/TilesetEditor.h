#ifndef _TILESETEDITOR_H_
#define _TILESETEDITOR_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

#include <vector>
#include <memory>
#include <set>
#include <map>
#include <string>

#include "Tileset.h"
#include "Palette.h"
#include "ImageBuffer.h"

class TilesetEditor : public wxVScrolledWindow
{
public:
	TilesetEditor(wxWindow* parent);
	TilesetEditor(wxWindow* parent, std::shared_ptr<Tileset>, std::shared_ptr<std::map<std::string, Palette>> palettes);
	~TilesetEditor();

	void SetColour(int c);
	void SetPalettes(std::shared_ptr<std::map<std::string, Palette>> palettes);
	std::shared_ptr<Tileset> GetTileset();
	bool Save(wxString filename, bool compressed = false);
	bool Open(wxString filename, int tile_width = 8, int tile_height = 8, int tile_bitdepth = 4);
	bool Open(std::vector<uint8_t>& pixels, bool uses_compression = false, int tile_width = 8, int tile_height = 8, int tile_bitdepth = 4);
	bool New(int r, int c);
	void RedrawTiles(int index = -1);

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
	Tile GetSelectedTile() const;
	bool IsHoverValid() const;
	Tile GetHoveredTile() const;

	void SelectTile(int tile);
	void InsertTileBefore(const Tile& tile);
	void InsertTileAfter(const Tile& tile);
	void InsertTileAtEnd();
	void DeleteTileAt(const Tile& tile);
	void CutTile(const Tile& tile);
	void CopyTile(const Tile& tile) const;
	void PasteTile(const Tile& tile);
	void SwapTile(const Tile& tile);
	void EditTile(const Tile& tile);
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
	void DrawTile(wxDC& dc, int x, int y, int tile);
	bool DrawTileAtPosition(wxDC& dc, int pos);
	void DrawSelectionBorders(wxDC& dc);
	void PaintBitmap(wxDC& dc);
	void InitialiseBrushesAndPens();
	void ForceRedraw();
	const Palette& GetSelectedPalette();

	void FireEvent(const wxEventType& e, const std::string& data);


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

	std::string m_name;

	std::shared_ptr<std::map<std::string, Palette>> m_palettes;
	std::string m_selected_palette;
	Palette m_default_palette;
	std::shared_ptr<Tileset> m_tileset = nullptr;
	int m_ctrlwidth;
	int m_ctrlheight;
	std::set<int> m_redraw_list;
	bool m_redraw_all;
	mutable std::vector<uint8_t> m_clipboard;
	int m_pendingswap;

	wxBrush* m_alpha_brush = nullptr;
	wxPen* m_border_pen = nullptr;
	wxPen* m_selected_border_pen = nullptr;
	wxPen* m_highlighted_border_pen = nullptr;
	wxBrush* m_highlighted_brush = nullptr;

	ImageBuffer m_buf;

	wxMemoryDC m_memdc;
	wxBitmap m_bmp;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_TILESET_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILESET_ACTIVATE, wxCommandEvent);

#endif // _TILESETEDITOR_H_
