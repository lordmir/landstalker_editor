#ifndef _MAP_3D_EDITOR_H_
#define _MAP_3D_EDITOR_H_

#include <memory>
#include <vector>
#include <set>
#include <wx/wx.h>
#include <wx/window.h>
#include <GameData.h>

class RoomViewerFrame;
class ImageBuffer;

class Map3DEditor : public wxScrolledCanvas
{
public:
	enum class MouseEventType
	{
		NONE,
		MOVE,
		ENTER,
		LEAVE,
		LEFT_DOWN,
		LEFT_UP,
		RIGHT_DOWN,
		RIGHT_UP
	};
	using Coord = std::pair<int, int>;

	Map3DEditor(wxWindow* parent, RoomViewerFrame* frame, Tilemap3D::Layer layer);
	virtual ~Map3DEditor();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }

	void SetSelectedBlock(int block);
	int GetSelectedBlock() const;
	bool IsBlockSelected() const;

	void SetHovered(Coord sel);
	Coord GetHovered() const;
	bool IsHoverValid() const;

	void SetSelectedSwap(int swap);
	int GetSelectedSwap() const;
	bool IsSwapSelected() const;
	void SetSelectedDoor(int swap);
	int GetSelectedDoor() const;
	bool IsDoorSelected() const;

	void RefreshGraphics();
	void UpdateSwaps();
	void UpdateDoors();

	bool HandleKeyDown(unsigned int key, unsigned int modifiers);
	bool HandleDrawKeyDown(unsigned int key, unsigned int modifiers);
	bool HandleRegionKeyDown(unsigned int key, unsigned int modifiers);
	bool HandleMouse(MouseEventType type, bool left_down, bool right_down, unsigned int modifiers, int x, int y);
	bool HandleLeftDown(unsigned int modifiers);
	bool HandleRightDown(unsigned int modifiers);
private:

	void SetHoveredTile();
	void SelectHoveredTile();

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
	void OnLeftDown(wxMouseEvent& evt);
	void OnRightDown(wxMouseEvent& evt);
	void OnLeftUp(wxMouseEvent& evt);
	void OnRightUp(wxMouseEvent& evt);
	void OnShow(wxShowEvent& evt);
	std::vector<wxPoint> GetRegionPoly(int x, int y, int w, int h, TileSwap::Mode mode);
	std::vector<wxPoint> OffsetRegionPoly(std::vector<wxPoint> points, const Coord& offset);
	bool Pnpoly(const std::vector<wxPoint>& poly, int x, int y);

	Coord GetAbsoluteCoordinates(int screenx, int screeny) const;
	Coord GetCellPosition(int screenx, int screeny) const;
	std::pair<int, int> GetScreenPosition(const Coord& pos) const;
	bool IsCoordValid(const Coord& c) const;

	bool UpdateHoveredPosition(int screenx, int screeny);
	bool UpdateSelectedPosition(int screenx, int screeny);
	int GetFirstDoorRegion(const Coord& c);
	std::pair<int, bool> GetFirstSwapRegion(const Coord& c);

	void RefreshCursor(bool ctrl_down);
	void UpdateCursor(wxStockCursor cursor);
	void FireUpdateStatusEvent(const std::string& data, int pane = 0);
	void FireEvent(const wxEventType& e, int userdata);
	void FireEvent(const wxEventType& e, const std::string& userdata);
	void FireEvent(const wxEventType& e);
	void FireMapEvent(const wxEventType& e);

	void StartDrag();
	void StopDrag(bool cancel = false);
	void RefreshDrag();


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
	Coord m_selected;
	Coord m_hovered;
	Coord m_dragged;
	Coord m_dragged_orig_pos;
	bool m_dragging;
	int m_selected_region;
	bool m_selected_is_src;

	std::vector<std::pair<std::vector<wxPoint>, std::vector<wxPoint>>> m_swap_regions;
	std::vector<std::vector<wxPoint>> m_door_regions;

	std::unique_ptr<wxBitmap> m_bmp;
	std::unique_ptr<wxPen> m_priority_pen;

	std::vector<Door> m_doors;
	std::vector<TileSwap> m_swaps;

	std::array<bool, 4> m_mousebtnstate;

	bool m_show_blocknums;
	bool m_show_borders;
	bool m_show_priority;

	static const std::size_t TILE_WIDTH = 32;
	static const std::size_t TILE_HEIGHT = 32;
	static const int SCROLL_RATE = 16;
	int m_scroll_rate;

	wxStockCursor m_cursorid;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_MAPLAYER_UPDATE, wxCommandEvent);

#endif // _MAP_3D_EDITOR_H_
