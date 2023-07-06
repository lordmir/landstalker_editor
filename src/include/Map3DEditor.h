#ifndef _MAP_3D_EDITOR_H_
#define _MAP_3D_EDITOR_H_

#include <memory>
#include <wx/wx.h>
#include <wx/window.h>
#include <GameData.h>

class RoomViewerFrame;
class ImageBuffer;

class Map3DEditor : public wxScrolledCanvas
{
public:

	Map3DEditor(wxWindow* parent, RoomViewerFrame* frame, Tilemap3D::Layer layer);
	virtual ~Map3DEditor();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }
	wxString GetStatusText() const;

	void RefreshGraphics();
private:
	void ForceRedraw();
	void RecreateBuffer();
	void UpdateScroll();
	void DrawMap();
	void DrawTiles();
	void DrawTile(int tile);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnEraseBackground(wxEraseEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnLeftClick(wxMouseEvent& evt);
	void OnRightClick(wxMouseEvent& evt);

	std::pair<int, int> GetAbsoluteCoordinates(int screenx, int screeny);
	std::pair<int, int> GetCellPosition(int screenx, int screeny);
	bool IsCoordValid(std::pair<int, int> c);

	bool UpdateHoveredPosition(int screenx, int screeny);
	bool UpdateSelectedPosition(int screenx, int screeny);

	void FireEvent(const wxEventType& e, long userdata);
	void FireEvent(const wxEventType& e, const std::string& userdata);
	void FireEvent(const wxEventType& e);

	std::shared_ptr<GameData> m_g;
	std::shared_ptr<Tilemap3D> m_map;
	std::unique_ptr<ImageBuffer> m_layer_buf;
	std::unique_ptr<ImageBuffer> m_bg_buf;
	RoomViewerFrame* m_frame;
	uint16_t m_roomnum;
	Tilemap3D::Layer m_layer;
	mutable wxString m_status_text;
	int m_width;
	int m_height;
	bool m_redraw;
	bool m_repaint;
	double m_zoom;
	std::pair<int, int> m_selected;
	std::pair<int, int> m_hovered;

	wxBitmap* m_bmp;

	static const std::size_t TILE_WIDTH = 32;
	static const std::size_t TILE_HEIGHT = 32;
	static const int SCROLL_RATE = 8;
	int m_scroll_rate;

	wxDECLARE_EVENT_TABLE();
};

#endif // _MAP_3D_EDITOR_H_
