#include "RoomViewerFrame.h"

RoomViewerFrame::RoomViewerFrame(wxWindow* parent)
	: EditorFrame(parent, wxID_ANY),
	  m_title(""),
	  m_mode(RoomViewerCtrl::Mode::NORMAL)
{
	m_mgr.SetManagedWindow(this);

	m_roomview = new RoomViewerCtrl(this);
	m_layerctrl = new LayerControlFrame(this);
	m_mgr.AddPane(m_roomview, wxAuiPaneInfo().CenterPane());
	m_mgr.AddPane(m_layerctrl, wxAuiPaneInfo().Right().Layer(1).Resizable(false).MinSize(220,200)
		.BestSize(220,200).FloatingSize(220,200).Caption("Layers"));

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();
	UpdateUI();
}

RoomViewerFrame::~RoomViewerFrame()
{
}

void RoomViewerFrame::SetMode(RoomViewerCtrl::Mode mode)
{
	m_mode = mode;
	Update();
}

void RoomViewerFrame::Update()
{
}

void RoomViewerFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_g = gd;
	if (m_roomview)
	{
		m_roomview->SetGameData(gd);
	}
	m_mode = RoomViewerCtrl::Mode::NORMAL;
	Update();
}

void RoomViewerFrame::ClearGameData()
{
	m_g = nullptr;
	if (m_roomview)
	{
		m_roomview->ClearGameData();
	}
	m_mode = RoomViewerCtrl::Mode::NORMAL;
	Update();
}

void RoomViewerFrame::SetRoomNum(uint16_t roomnum, RoomViewerCtrl::Mode mode)
{
	m_roomnum = roomnum;
	m_mode = mode;
	m_roomview->SetRoomNum(roomnum, mode);
	Update();
}
