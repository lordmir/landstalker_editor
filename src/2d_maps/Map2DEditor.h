#ifndef _MAP2DEDITOR_H_
#define _MAP2DEDITOR_H_

#include <wx/wx.h>
#include <wx/vscroll.h>
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

	enum class Mode
	{
		SELECT,
		PENCIL
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
private:
	virtual wxCoord OnGetRowHeight(size_t row) const override;
	virtual wxCoord OnGetColumnWidth(size_t row) const override;

	bool UpdateRowCount();
	void DrawTile(wxDC& dc, int x, int y, const Landstalker::Tile& tile);
	bool DrawTileAtPosition(wxDC& dc, int x, int y);
	void DrawSelectionBorders(wxDC& dc);
	void PaintBitmap(wxDC& dc);
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

	Mode m_mode;

	int m_pixelsize;
	bool m_selectable;
	int m_selectedtile;
	int m_hoveredtile;
	int m_tilebase;

	bool m_enabletilenumbers;
	bool m_enableborders;
	bool m_enableselection;
	bool m_enablehover;
	bool m_enablealpha;

	bool m_redraw_all;
	std::set<int> m_redraw_list;
	Landstalker::Tile m_drawtile;

	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxPen> m_border_pen;
	std::unique_ptr<wxPen> m_selected_border_pen;
	std::unique_ptr<wxPen> m_highlighted_border_pen;
	std::unique_ptr<wxBrush> m_highlighted_brush;
	std::unique_ptr<wxBitmap> m_stipple;

	wxMemoryDC m_memdc;
	wxBitmap m_bmp;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_MAP_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_HOVER, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_EDIT_REQUEST, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_MAP_ACTIVATE, wxCommandEvent);

#endif // _MAP2DEDITOR_H_
