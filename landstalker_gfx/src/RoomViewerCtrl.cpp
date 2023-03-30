#include "include\RoomViewerCtrl.h"

#include <wx\dcbuffer.h>

wxBEGIN_EVENT_TABLE(RoomViewerCtrl, wxScrolledCanvas)
EVT_PAINT(RoomViewerCtrl::OnPaint)
EVT_SIZE(RoomViewerCtrl::OnSize)
wxEND_EVENT_TABLE()

RoomViewerCtrl::RoomViewerCtrl(wxWindow* parent)
	: wxScrolledCanvas(parent, wxID_ANY),
	  m_roomnum(0)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

RoomViewerCtrl::~RoomViewerCtrl()
{
}

void RoomViewerCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_g = gd;
	Refresh(true);
}

void RoomViewerCtrl::ClearGameData()
{
	m_g = nullptr;
	Refresh(true);
}

void RoomViewerCtrl::SetRoomNum(uint16_t roomnum)
{
	m_roomnum = roomnum;
	Refresh(true);
}

void RoomViewerCtrl::OnDraw(wxDC& dc)
{
	dc.SetBackground(*wxBLUE_BRUSH);
	dc.Clear();
	auto font = dc.GetFont();
	font.MakeBold();
	font.SetPointSize(72);
	dc.SetFont(font);
	dc.SetTextForeground(*wxYELLOW);
	dc.DrawText(std::to_string(m_roomnum), 30, 30);
}

void RoomViewerCtrl::OnPaint(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void RoomViewerCtrl::OnSize(wxSizeEvent& evt)
{
	Refresh(false);
	evt.Skip();
}
