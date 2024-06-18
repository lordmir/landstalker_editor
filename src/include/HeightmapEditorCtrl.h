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
	using Coord = std::pair<int, int>;
	enum class MouseEventType
	{
		NONE,
		MOVE,
		ENTER,
		LEAVE,
		LEFT_DOWN,
		LEFT_UP,
		LEFT_DCLICK,
		RIGHT_DOWN,
		RIGHT_UP,
		RIGHT_DCLICK
	};

	HeightmapEditorCtrl(wxWindow* parent, RoomViewerFrame* frame);
	virtual ~HeightmapEditorCtrl();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }

	void SetZoom(double zoom);
	double GetZoom() const { return m_zoom; }
	void RefreshGraphics();
	void UpdateSwaps();
	void UpdateDoors();
	void UpdateWarps(const std::vector<WarpList::Warp>& warps);
	void UpdateEntities(const std::vector<Entity>& entities);

	bool IsCoordValid(const HeightmapEditorCtrl::Coord& c) const;
	bool IsHoverValid() const;

	void SetSelectedSwap(int swap);
	int GetSelectedSwap() const;
	bool IsSwapSelected() const;
	const std::vector<TileSwap>& GetTileswaps() const;
	int GetTotalTileswaps() const;
	void AddTileswap();
	void DeleteTileswap();

	void SetSelectedDoor(int door);
	int GetSelectedDoor() const;
	bool IsDoorSelected() const;
	const std::vector<Door>& GetDoors() const;
	int GetTotalDoors() const;

	bool HandleKeyDown(unsigned int key, unsigned int modifiers);
	bool HandleDrawKeyDown(unsigned int key, unsigned int modifiers);
	bool HandleRegionKeyDown(unsigned int key, unsigned int modifiers);

	void ClearSelection();
	void SetSelection(int ix, int iy);
	std::pair<int, int> GetSelection() const;
	bool IsSelectionValid() const;
	void NudgeSelectionUp();
	void NudgeSelectionDown();
	void NudgeSelectionLeft();
	void NudgeSelectionRight();
	void NudgeHeightmapUp();
	void NudgeHeightmapDown();
	void NudgeHeightmapLeft();
	void NudgeHeightmapRight();
	void InsertRowAbove();
	void InsertRowBelow();
	void DeleteRow();
	void InsertColumnLeft();
	void InsertColumnRight();
	void DeleteColumn();

	uint8_t GetSelectedHeight() const;
	void SetSelectedHeight(uint8_t height);
	void IncreaseSelectedHeight();
	void DecreaseSelectedHeight();
	void ClearSelectedCell();

	uint8_t GetSelectedRestrictions() const;
	void SetSelectedRestrictions(uint8_t restrictions);
	bool IsSelectedPlayerPassable() const;
	void ToggleSelectedPlayerPassable();
	bool IsSelectedNPCPassable() const;
	void ToggleSelectedNPCPassable();
	bool IsSelectedRaftTrack() const;
	void ToggleSelectedRaftTrack();
	void IncrementSelectedRestrictions();
	void DecrementSelectedRestrictions();

	uint8_t GetSelectedType() const;
	void SetSelectedType(uint8_t type);
	void IncrementSelectedType();
	void DecrementSelectedType();

	bool HandleMouse(MouseEventType type, bool left_down, bool right_down, unsigned int modifiers, int x, int y);
	bool HandleLeftDown(unsigned int modifiers);
	bool HandleLeftDClick(unsigned int modifiers);
	bool HandleRightDown(unsigned int modifiers);
private:
	void RefreshStatusbar();
	void RefreshCursor(bool ctrl_down);
	void UpdateCursor(wxStockCursor cursor);
	void DrawRoomHeightmapBackground(wxDC& dc);
	void DrawRoomHeightmapForeground(wxDC& dc);
	void DrawCellRange(wxDC& dc, float x, float y, float w = 1.0f, float h = 1.0f, int s = 1);
	void DrawDoors(wxDC& dc);
	void DrawTileSwaps(wxDC& dc);
	void DrawWarps(wxDC& dc);
	void DrawEntities(wxDC& dc);
	void DrawSelectionCursors(wxDC& dc);
	void ForceRepaint();
	void ForceRedraw();
	void RecreateBuffer();
	void UpdateScroll();
	bool Pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y);
	void GoToRoom(uint16_t room);
	void SetOpacity(wxImage& image, uint8_t opacity);
	bool IsCoordValid(std::pair<int, int> c);

	std::vector<wxPoint> GetTilePoly(float x, float y, float cols = 1.0f, float rows = 1.0f, int shrink = 0, int width = TILE_WIDTH, int height = TILE_HEIGHT) const;
	wxColor GetCellBackground(uint8_t restrictions, uint8_t type, uint8_t z);
	wxColor GetCellBorder(uint8_t restrictions, uint8_t type, uint8_t z);
	bool IsCellHidden(uint8_t restrictions, uint8_t type, uint8_t z);

	void FireUpdateStatusEvent(const std::string& data, int pane = 0);
	void FireEvent(const wxEventType& e, int userdata);
	void FireEventLong(const wxEventType& e, long userdata);
	void FireEvent(const wxEventType& e, const std::string& userdata);
	void FireEvent(const wxEventType& e);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnEraseBackground(wxEraseEvent& evt);
	void OnSize(wxSizeEvent& evt);
	std::pair<int, int> GetAbsoluteCoordinates(int screenx, int screeny);
	std::pair<int, int> GetHMPosition(int screenx, int screeny);
	bool UpdateHoveredPosition(int screenx, int screeny);
	bool UpdateSelectedPosition(int screenx, int screeny);
	void OnLeftClick(wxMouseEvent& evt);
	void OnLeftDblClick(wxMouseEvent& evt);
	void OnRightClick(wxMouseEvent& evt);
	void OnRightDblClick(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnLeftDown(wxMouseEvent& evt);
	void OnLeftDClick(wxMouseEvent& evt);
	void OnRightDown(wxMouseEvent& evt);
	void OnLeftUp(wxMouseEvent& evt);
	void OnRightUp(wxMouseEvent& evt);
	void OnRightDClick(wxMouseEvent& evt);
	void OnShow(wxShowEvent& evt);
	int GetFirstDoorRegion(const Coord& c);
	std::pair<int, bool> GetFirstSwapRegion(const Coord& c);

	void StartDrag();
	void StopDrag(bool cancel = false);
	void RefreshDrag();

	std::shared_ptr<GameData> m_g;
	std::shared_ptr<Tilemap3D> m_map;
	RoomViewerFrame* m_frame;
	uint16_t m_roomnum;
	int m_width;
	int m_height;
	bool m_redraw;
	bool m_repaint;
	double m_zoom;
	Coord m_selected;
	Coord m_hovered;
	Coord m_cpysrc;
	Coord m_dragged;
	Coord m_dragged_orig_pos;
	std::vector<Door> m_doors;
	std::vector<TileSwap> m_swaps;
	std::vector<Entity> m_entities;
	std::vector<WarpList::Warp> m_warps;

	std::unique_ptr<wxBitmap> m_bmp;
	bool m_dragging;
	int m_selected_region;
	bool m_selected_is_src;

	static const std::size_t TILE_WIDTH = 64;
	static const std::size_t TILE_HEIGHT = 32;
	static const int SCROLL_RATE = 32;
	int m_scroll_rate;
	wxStockCursor m_cursorid;

	std::vector<std::string> m_errors;

	wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(EVT_HEIGHTMAP_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(EVT_HEIGHTMAP_MOVE, wxCommandEvent);
wxDECLARE_EVENT(EVT_HEIGHTMAP_CELL_SELECTED, wxCommandEvent);

#endif // _HEIGHTMAP_EDITOR_CTRL_H_
