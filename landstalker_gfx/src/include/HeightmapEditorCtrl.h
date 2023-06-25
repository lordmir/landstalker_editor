#ifndef _HEIGHTMAP_EDITOR_CTRL_H_
#define _HEIGHTMAP_EDITOR_CTRL_H_

#include <wx/wx.h>
#include <wx/window.h>
#include <memory>
#include <cstdint>
#include "GameData.h"
#include "ImageBuffer.h"
#include "Tilemap3DCmp.h"

class RoomViewerFrame;

class HeightmapEditorCtrl : public wxScrolledCanvas
{
public:

	HeightmapEditorCtrl(wxWindow* parent, RoomViewerFrame* frame);
	virtual ~HeightmapEditorCtrl();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }

	void SetZoom(double zoom);
	double GetZoom() const { return m_zoom; }
	void RefreshGraphics();

	bool HandleKeyDown(unsigned int key, unsigned int modifiers);
private:
	void DrawRoomHeightmap();
	std::shared_ptr<wxBitmap> DrawHeightmapVisualisation(std::shared_ptr<Tilemap3D> map, uint8_t opacity);
	std::shared_ptr<wxBitmap> DrawHeightmapGrid(std::shared_ptr<Tilemap3D> map);
	void DrawHeightmapCell(wxGraphicsContext& gc, int x, int y, int z, int width, int height, int restrictions,
		int classification, bool draw_walls = true, wxColor border_colour = *wxWHITE);
	void ForceRepaint();
	void ForceRedraw();
	void UpdateScroll();
	bool Pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y);
	void GoToRoom(uint16_t room);
	void SetOpacity(wxImage& image, uint8_t opacity);

	std::vector<wxPoint> GetTilePoly(int x, int y, int width = TILE_WIDTH, int height = TILE_HEIGHT) const;
	wxColor GetCellBackground(uint8_t restrictions, uint8_t type, uint8_t z);
	wxColor GetCellBorder(uint8_t restrictions, uint8_t type, uint8_t z);
	bool IsCellHidden(uint8_t restrictions, uint8_t type, uint8_t z);

	void FireEvent(const wxEventType& e, long userdata);
	void FireEvent(const wxEventType& e, const std::string& userdata);
	void FireEvent(const wxEventType& e);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnEraseBackground(wxEraseEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	std::pair<int, int> GetAbsoluteCoordinates(int screenx, int screeny);
	void OnLeftClick(wxMouseEvent& evt);
	void OnLeftDblClick(wxMouseEvent& evt);
	void OnRightClick(wxMouseEvent& evt);
	void OnRightDblClick(wxMouseEvent& evt);

	std::shared_ptr<GameData> m_g;
	std::shared_ptr<Tilemap3D> m_map;
	RoomViewerFrame* m_frame;
	uint16_t m_roomnum;
	int m_width;
	int m_height;
	bool m_redraw;
	bool m_repaint;
	double m_zoom;

	wxBitmap* m_bmp;

	static const std::size_t TILE_WIDTH = 64;
	static const std::size_t TILE_HEIGHT = 32;
	static const int SCROLL_RATE = 8;
	int m_scroll_rate;
	int m_selected;

	std::string m_status_text;
	std::vector<std::string> m_errors;

	wxDECLARE_EVENT_TABLE();
};

#endif // _HEIGHTMAP_EDITOR_CTRL_H_
