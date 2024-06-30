#ifndef _TILEEDITOR_H_
#define _TILEEDITOR_H_

#include <wx/wx.h>
#include <wx/vscroll.h>

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "Palette.h"
#include "Tileset.h"
#include "Tile.h"
#include "GameData.h"
#include "DataTypes.h"

class TileEditor : public wxHVScrolledWindow
{
public:
	struct Point
	{
		int x;
		int y;
		Point(int x_, int y_) : x(x_), y(y_) {}
		Point(const Point& p) : x(p.x), y(p.y) {}
		Point& operator=(const Point& rhs)
		{
			x = rhs.x;
			y = rhs.y;
			return *this;
		}
		bool operator==(const Point& rhs) const
		{
			return (x == rhs.x) && (y == rhs.y);
		}
		bool operator!=(const Point& rhs) const
		{
			return !(*this == rhs);
		}
	};
	struct Size { int w; int h; };

	TileEditor(wxWindow* parent);
	~TileEditor();

	void SetGameData(std::shared_ptr<GameData> gd);
	void SetTileset(std::shared_ptr<Tileset> tileset);
	void SetTile(const Tile& tile);

	void Redraw();
	int GetPixelSize() const;
	void SetActivePalette(const std::string& name);
	void SetActivePalette(std::shared_ptr<Palette> pal);
	std::string GetActivePalette() const;
	const Tile& GetTile() const;

	void SetPrimaryColour(uint8_t colour);
	uint8_t GetPrimaryColour() const;
	void SetSecondaryColour(uint8_t colour);
	uint8_t GetSecondaryColour() const;

	bool GetEditEnabled() const;
	void SetEditEnabled(bool enabled);
	bool GetAlphaEnabled() const;
	void SetAlphaEnabled(bool enabled);
	bool GetBordersEnabled() const;
	void SetBordersEnabled(bool enabled);

	bool IsPointValid(const Point& point) const;
	bool IsSelectionValid() const;
	Point GetSelectedPixel() const;
	bool IsHoverValid() const;
	Point GetHoveredPixel() const;

	int GetColourAtPixel(const Point& point) const;
	bool SetColourAtPixel(const Point& point, int colour);
	int GetColour(int index) const;
	void Clear();

	int  ConvertXYToPixel(const Point& point);
	Point ConvertPixelToXY(int pixel);

private:
	virtual wxCoord OnGetRowHeight(size_t row) const override;
	virtual wxCoord OnGetColumnWidth(size_t column) const override;

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseUpDown(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseEnter(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void MouseDraw(const Point& point);
	void OnTilesetFocus(wxFocusEvent& evt);
	int  ConvertMouseXYToPixel(const wxPoint& point);
	void ForceRedraw();
	void AutoSize();

	bool SetColour(const Point& point, int colour);
	void SetPixelSize(int n);
	int ValidateColour(int colour) const;
	void InitialiseBrushesAndPens();
	wxBrush GetBrush(int index);

	void FireEvent(const wxEventType& e);
	int m_ctrlwidth;
	int m_ctrlheight;
	int m_pixelsize;
	uint8_t m_primary_colour;
	uint8_t m_secondary_colour;
	Point m_selectedpixel;
	Point m_hoveredpixel;
	bool m_secondary_active;
	bool m_drawing;

	std::unique_ptr<wxBrush> m_alpha_brush;
	std::unique_ptr<wxPen> m_border_pen;
	std::unique_ptr<wxPen> m_selected_border_pen;
	std::unique_ptr<wxPen> m_highlighted_border_pen;
	std::unique_ptr<wxBrush> m_highlighted_brush;
	std::unique_ptr<wxBitmap> m_stipple;

	bool m_enableborders;
	bool m_enableedit;
	bool m_enablealpha;

	std::shared_ptr<GameData> m_gd;
	std::shared_ptr<Tileset> m_tileset;
	std::string m_selected_palette_name;
	std::shared_ptr<PaletteEntry> m_selected_palette_entry;
	std::shared_ptr<Palette> m_selected_palette;
	Tile m_tile;
	std::vector<uint8_t> m_pixels;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_TILE_CHANGE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILE_ACTIVATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILE_PIXEL_SELECT, wxCommandEvent);
wxDECLARE_EVENT(EVT_TILE_PIXEL_HOVER, wxCommandEvent);

#endif // _TILEEDITOR_H_