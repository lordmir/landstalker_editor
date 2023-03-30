#ifndef _ROOM_VIEWER_CTRL_H_
#define _ROOM_VIEWER_CTRL_H_

#include <wx/wx.h>
#include <wx/window.h>
#include <memory>
#include <cstdint>
#include "GameData.h"
#include "ImageBuffer.h"

class RoomViewerCtrl : public wxScrolledCanvas
{
public:
	enum class Mode : uint8_t
	{
		NORMAL,
		HEIGHTMAP,
		WARPS,
		ENTITY_PLACEMENT
	};

	enum class Layer : uint8_t
	{
		BACKGROUND1,
		BACKGROUND2,
		BG_SPRITES,
		FOREGROUND,
		FG_SPRITES,
		HEIGHTMAP,
		WARPS
	};

	RoomViewerCtrl(wxWindow* parent);
	virtual ~RoomViewerCtrl();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetRoomNum(uint16_t roomnum, Mode mode);
	uint16_t GetRoomNum() const { return m_roomnum; }
	Mode GetMode() const { return m_mode; }
private:
	void DrawRoom(uint16_t roomnum);
	std::shared_ptr<wxBitmap> DrawHeightmapVisualisation(std::shared_ptr<Tilemap3D> map, uint8_t opacity);
	void DrawHeightmapCell(wxGraphicsContext& gc, int x, int y, int z, int width, int height, int restrictions, int classification);
	void ForceRepaint();
	void SetOpacity(wxImage& image, uint8_t opacity);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnEraseBackground(wxEraseEvent& evt);
	void OnSize(wxSizeEvent& evt);

	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;
	Mode m_mode;
	bool m_redraw;

	std::map<Layer, std::shared_ptr<ImageBuffer>> m_layer_bufs;
	std::map<Layer, std::shared_ptr<wxBitmap>> m_layers;
	std::map<Layer, uint8_t> m_layer_opacity;
	wxBitmap* m_bmp;

	std::list<std::pair<WarpList::Warp, std::vector<wxPoint2DDouble>>> m_warp_poly;
	std::list<std::pair<uint16_t, std::vector<wxPoint2DDouble>>> m_link_poly;

	static const std::size_t TILE_WIDTH = 32;
	static const std::size_t TILE_HEIGHT = 16;
	static const int SCROLL_RATE = 8;

	wxDECLARE_EVENT_TABLE();
};

#endif // _ROOM_VIEWER_CTRL_H_
