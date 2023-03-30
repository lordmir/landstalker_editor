#ifndef _ROOM_VIEWER_CTRL_H_
#define _ROOM_VIEWER_CTRL_H_

#include <wx/wx.h>
#include <wx/window.h>
#include <memory>
#include <cstdint>
#include "GameData.h"

class RoomViewerCtrl : public wxScrolledCanvas
{
public:
	RoomViewerCtrl(wxWindow* parent);
	virtual ~RoomViewerCtrl();

	void SetGameData(std::shared_ptr<GameData> gd);
	void ClearGameData();

	void SetRoomNum(uint16_t roomnum);
	uint16_t GetRoomNum() const { return m_roomnum; }
private:

	void OnDraw(wxDC& dc);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);

	std::shared_ptr<GameData> m_g;
	uint16_t m_roomnum;

	wxDECLARE_EVENT_TABLE();
};

#endif // _ROOM_VIEWER_CTRL_H_
