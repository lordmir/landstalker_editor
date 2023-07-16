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

	void SetSelectedBlock(int block);
	int GetSelectedBlock() const;
	bool IsBlockSelected() const;

	void RefreshGraphics();
	void UpdateSwaps();
	void UpdateDoors();
private:
	void RefreshStatusbar();
	void ForceRedraw();
	void RecreateBuffer();
	void UpdateScroll();
	void DrawMap(wxDC& dc);
	void DrawTiles();
	void DrawTile(int tile);
	void DrawCell(wxDC& dc, const std::pair<int, int>& pos, const wxPen& pen, const wxBrush& brush);
	void DrawTileSwaps(wxDC& dc);
	void DrawDoors(wxDC& dc);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnEraseBackground(wxEraseEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnLeftClick(wxMouseEvent& evt);
	void OnRightClick(wxMouseEvent& evt);
	void OnShow(wxShowEvent& evt);
	std::vector<wxPoint> GetRegionPoly(int x, int y, int w, int h, TileSwap::Mode mode);

	std::pair<int, int> GetAbsoluteCoordinates(int screenx, int screeny) const;
	std::pair<int, int> GetCellPosition(int screenx, int screeny) const;
	std::pair<int, int> GetScreenPosition(const std::pair<int, int>& pos) const;
	bool IsCoordValid(std::pair<int, int> c) const;

	bool UpdateHoveredPosition(int screenx, int screeny);
	bool UpdateSelectedPosition(int screenx, int screeny);

	void FireUpdateStatusEvent(const std::string& data, int pane = 0);
	void FireEvent(const wxEventType& e, int userdata);
	void FireEvent(const wxEventType& e, const std::string& userdata);
	void FireEvent(const wxEventType& e);
	void FireMapEvent(const wxEventType& e);

	std::shared_ptr<GameData> m_g;
	std::shared_ptr<Tilemap3D> m_map;
	std::unique_ptr<ImageBuffer> m_layer_buf;
	std::unique_ptr<ImageBuffer> m_bg_buf;

	std::shared_ptr<Tileset> m_tileset;
	std::shared_ptr<Palette> m_pal;
	std::shared_ptr<Blockset> m_blockset;

	RoomViewerFrame* m_frame;
	uint16_t m_roomnum;
	Tilemap3D::Layer m_layer;
	int m_width;
	int m_height;
	bool m_redraw;
	bool m_repaint;
	double m_zoom;
	int m_selected_block;
	std::pair<int, int> m_selected;
	std::pair<int, int> m_hovered;

	std::unique_ptr<wxBitmap> m_bmp;
	std::unique_ptr<wxPen> m_priority_pen;

	std::vector<Door> m_doors;
	std::vector<TileSwap> m_swaps;

	bool m_show_blocknums;
	bool m_show_borders;
	bool m_show_priority;

	static const std::size_t TILE_WIDTH = 32;
	static const std::size_t TILE_HEIGHT = 32;
	static const int SCROLL_RATE = 8;
	int m_scroll_rate;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_MAPLAYER_UPDATE, wxCommandEvent);

#endif // _MAP_3D_EDITOR_H_
