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
		BG_SPRITES_WIREFRAME_BG,
		BG_SPRITES,
		BG_SPRITES_WIREFRAME_FG,
		FOREGROUND,
		HEIGHTMAP,
		FG_SPRITES_WIREFRAME_BG,
		FG_SPRITES,
		FG_SPRITES_WIREFRAME_FG,
		WARPS
	};

	RoomViewerCtrl(wxWindow* parent);
	virtual ~RoomViewerCtrl();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetRoomNum(uint16_t roomnum, Mode mode);
	uint16_t GetRoomNum() const { return m_roomnum; }
	Mode GetMode() const { return m_mode; }

	void SelectEntity(int selection);
	void ClearSelection();
	int GetTotalEntities() const;
	bool IsEntitySelected() const;
	const Entity& GetSelectedEntity() const;
	int GetSelectedEntityIndex() const;
	const std::vector<Entity>& GetEntities() const;

	void SetZoom(double zoom);
	double GetZoom() const { return m_zoom; }
	void SetLayerOpacity(Layer layer, uint8_t opacity);
	uint8_t GetLayerOpacity(Layer layer) const;
	void RefreshGraphics();

	int GetErrorCount() const;
	std::string GetErrorText(int errnum) const;

	const std::string& GetStatusText() const;
private:
	struct SpriteQ
	{
		uint8_t id;
		uint8_t palette;
		int x;
		int y;
		bool hflip;
		bool selected;
		bool background;
		std::shared_ptr<SpriteFrame> frame;
		Entity entity;
	};
	void DrawRoom(uint16_t roomnum);
	std::vector<std::shared_ptr<Palette>> PreparePalettes(uint16_t roomnum);
	std::vector<SpriteQ> PrepareSprites(uint16_t roomnum);
	void DrawSprites(const std::vector<SpriteQ>& q);
	void DrawSpriteHitboxes(const std::vector<SpriteQ>& q);
	void RedrawAllSprites();
	void UpdateLayer(const Layer& layer, std::shared_ptr<wxBitmap> bmp);

	void UpdateRoomDescText(uint16_t roomnum);
	std::shared_ptr<wxBitmap> DrawRoomWarps(uint16_t roomnum);
	void DrawWarp(wxGraphicsContext& gc, const WarpList::Warp& warp, std::shared_ptr<Tilemap3D> map, int tile_width, int tile_height);
	void AddRoomLink(wxGraphicsContext* gc, const std::string& label, uint16_t room, int x, int y);
	void DrawRoomHeightmap(uint16_t roomnum);
	std::shared_ptr<wxBitmap> DrawHeightmapVisualisation(std::shared_ptr<Tilemap3D> map, uint8_t opacity);
	std::shared_ptr<wxBitmap> DrawHeightmapGrid(std::shared_ptr<Tilemap3D> map);
	void DrawHeightmapCell(wxGraphicsContext& gc, int x, int y, int z, int width, int height, int restrictions,
		int classification, bool draw_walls = true, wxColor border_colour = *wxWHITE);
	void ForceRepaint();
	void ForceRedraw();
	void SetOpacity(wxImage& image, uint8_t opacity);
	void UpdateScroll();
	void UpdateBuffer();
	void RedrawRoom();
	bool Pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y);
	void GoToRoom(uint16_t room);
	void UpdateEntityProperties(int entity);

	void FireEvent(const wxEventType& e, long userdata);
	void FireEvent(const wxEventType& e, const std::string& userdata);
	void FireEvent(const wxEventType& e);

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnEraseBackground(wxEraseEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnLeftClick(wxMouseEvent& evt);
	void OnLeftDblClick(wxMouseEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);

	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;
	Mode m_mode;
	std::vector<std::shared_ptr<Palette>> m_rpalette;
	int m_width;
	int m_height;
	int m_buffer_width;
	int m_buffer_height;
	bool m_redraw;
	bool m_repaint;
	double m_zoom;

	std::map<Layer, std::shared_ptr<ImageBuffer>> m_layer_bufs;
	std::map<Layer, std::shared_ptr<wxBitmap>> m_layers;
	std::map<Layer, uint8_t> m_layer_opacity;
	wxBitmap* m_bmp;
	std::vector<Entity> m_entities;

	std::list<std::pair<WarpList::Warp, std::vector<wxPoint2DDouble>>> m_warp_poly;
	std::list<std::pair<uint16_t, std::vector<wxPoint2DDouble>>> m_link_poly;
	std::list<std::pair<int, std::vector<wxPoint2DDouble>>> m_entity_poly;

	static const std::size_t TILE_WIDTH = 32;
	static const std::size_t TILE_HEIGHT = 16;
	static const std::size_t HM_CELL_WIDTH = 32;
	static const std::size_t HM_CELL_HEIGHT = 32;
	static const int SCROLL_RATE = 8;
	int m_scroll_rate;
	int m_selected;

	std::string m_status_text;
	std::vector<std::string> m_errors;

	wxDECLARE_EVENT_TABLE();
};

#endif // _ROOM_VIEWER_CTRL_H_
