#include "include\RoomViewerFrame.h"

RoomViewerFrame::RoomViewerFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_title(""),
	  m_mode(Mode::NORMAL)
{
	m_mgr.SetManagedWindow(this);

	m_roomview = new RoomViewerCtrl(this);
	m_mgr.AddPane(m_roomview, wxAuiPaneInfo().CenterPane());

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

RoomViewerFrame::~RoomViewerFrame()
{
}

void RoomViewerFrame::SetMode(Mode mode)
{
	m_mode = mode;
	Update();
}

void RoomViewerFrame::Update()
{
	m_roomview->Refresh(true);
}

void RoomViewerFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_g = gd;
	if (m_roomview)
	{
		m_roomview->SetGameData(gd);
	}
	m_mode = Mode::NORMAL;
	Update();
}

void RoomViewerFrame::ClearGameData()
{
	m_g = nullptr;
	if (m_roomview)
	{
		m_roomview->ClearGameData();
	}
	m_mode = Mode::NORMAL;
	Update();
}

void RoomViewerFrame::SetRoomNum(uint16_t roomnum)
{
	m_roomnum = roomnum;
	m_roomview->SetRoomNum(roomnum);
	Update();
}
