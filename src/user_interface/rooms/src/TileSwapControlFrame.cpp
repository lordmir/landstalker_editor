#include <user_interface/rooms/include/TileSwapControlFrame.h>
#include <landstalker/misc/include/Utils.h>
#include <user_interface/rooms/include/RoomViewerFrame.h>

wxDEFINE_EVENT(EVT_TILESWAP_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_OPEN_PROPERTIES, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_MOVE_DOWN, wxCommandEvent);

wxDEFINE_EVENT(EVT_DOOR_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_OPEN_PROPERTIES, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_MOVE_DOWN, wxCommandEvent);

TileSwapControlFrame::TileSwapControlFrame(RoomViewerFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst),
	  m_rvf(parent),
	  m_selected(ID::TILESWAP),
	  m_mode(RoomEdit::Mode::NORMAL),
	  m_roomnum(0)
{
	m_swap_btn = new wxBitmapToggleButton(this, static_cast<int>(ID::TILESWAP), imglst->GetImage("map_edit_tileswaps"), wxDefaultPosition, {24, -1});
	m_door_btn = new wxBitmapToggleButton(this, static_cast<int>(ID::DOOR), imglst->GetImage("map_edit_doors"), wxDefaultPosition, { 24, -1 });
	m_swap_btn->SetToolTip("Edit Tileswap");
	m_door_btn->SetToolTip("Edit Doors");
	m_buttons_boxsizer->Add(m_swap_btn, 0, wxLEFT, 5);
	m_buttons_boxsizer->Add(m_door_btn, 0, 0, 0);
	UpdateUI();

	m_swap_btn->Bind(wxEVT_TOGGLEBUTTON, &TileSwapControlFrame::OnToggle, this);
	m_door_btn->Bind(wxEVT_TOGGLEBUTTON, &TileSwapControlFrame::OnToggle, this);
}

TileSwapControlFrame::~TileSwapControlFrame()
{
}

void TileSwapControlFrame::SetGameData(std::shared_ptr<GameData> gd)
{
	m_gd = gd;
}

void TileSwapControlFrame::ClearGameData()
{
	m_gd.reset();
}

void TileSwapControlFrame::SetSwaps(const std::vector<TileSwap>& swaps, const std::vector<Door>& doors, uint16_t roomnum)
{
	m_swaps = swaps;
	m_doors = doors;
	if (roomnum != m_roomnum)
	{
		if (m_swaps.empty() && !m_doors.empty())
		{
			m_selected = ID::DOOR;
		}
		else if (m_doors.empty())
		{
			m_selected = ID::TILESWAP;
		}
		m_roomnum = roomnum;
	}
	UpdateUI();
}

void TileSwapControlFrame::ResetSwaps()
{
	m_swaps.clear();
	m_doors.clear();
	m_selected = ID::TILESWAP;
	UpdateUI();
}

void TileSwapControlFrame::SetPage(TileSwapControlFrame::ID id)
{
	m_selected = id;
	UpdateUI();
}

TileSwapControlFrame::ID TileSwapControlFrame::GetPage() const
{
	return m_selected;
}

int TileSwapControlFrame::GetMaxSelection() const
{
	return (m_selected == ID::TILESWAP) ? m_swaps.size() : m_doors.size();
}

void TileSwapControlFrame::SetMode(RoomEdit::Mode mode)
{
	m_mode = mode;
	UpdateUI();
}

void TileSwapControlFrame::Select()
{
	(m_selected == ID::TILESWAP) ? FireEvent(EVT_TILESWAP_SELECT) : FireEvent(EVT_DOOR_SELECT);
}

void TileSwapControlFrame::Add()
{
	(m_selected == ID::TILESWAP) ? FireEvent(EVT_TILESWAP_ADD) : FireEvent(EVT_DOOR_ADD);
}

void TileSwapControlFrame::Delete()
{
	(m_selected == ID::TILESWAP) ? FireEvent(EVT_TILESWAP_DELETE) : FireEvent(EVT_DOOR_DELETE);
}

void TileSwapControlFrame::MoveUp()
{
	(m_selected == ID::TILESWAP) ? FireEvent(EVT_TILESWAP_MOVE_UP) : FireEvent(EVT_DOOR_MOVE_UP);
}

void TileSwapControlFrame::MoveDown()
{
	(m_selected == ID::TILESWAP) ? FireEvent(EVT_TILESWAP_MOVE_DOWN) : FireEvent(EVT_DOOR_MOVE_DOWN);
}

void TileSwapControlFrame::OpenElement()
{
	(m_selected == ID::TILESWAP) ? FireEvent(EVT_TILESWAP_OPEN_PROPERTIES) : FireEvent(EVT_DOOR_OPEN_PROPERTIES);
}

bool TileSwapControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return m_rvf->HandleKeyDown(key, modifiers);
}

void TileSwapControlFrame::UpdateOtherControls()
{
	m_door_btn->SetValue(m_door_btn->GetId() == static_cast<int>(m_selected));
	m_swap_btn->SetValue(m_swap_btn->GetId() == static_cast<int>(m_selected));
}

std::wstring TileSwapControlFrame::MakeLabel(int index) const
{
	if (index >= static_cast<int>(m_swaps.size()))
	{
		return L"<Invalid>";
	}
	if (!m_swaps[index].active)
	{
		return StrWPrintf(L"%02d: <Inactive>", index + 1);
	}
	if (m_selected == ID::TILESWAP)
	{
		if (m_mode == RoomEdit::Mode::BACKGROUND || m_mode == RoomEdit::Mode::FOREGROUND)
		{
			int offsetx = 0, offsety = 0;
			if (m_gd)
			{
				auto map = m_gd->GetRoomData()->GetMapForRoom(m_roomnum);
				if (map)
				{
					offsetx = map->GetData()->GetLeft() - (m_mode == RoomEdit::Mode::FOREGROUND ? 1 : 0);
					offsety = map->GetData()->GetTop();
				}
			}
			return StrWPrintf(L"%02d: M%01d (%02d, %02d) -> (%02d, %02d) %02dx%02d", index + 1,
				static_cast<int>(m_swaps[index].mode),
				m_swaps[index].map.src_x - offsetx, m_swaps[index].map.src_y - offsety,
				m_swaps[index].map.dst_x - offsetx, m_swaps[index].map.dst_y - offsety,
				m_swaps[index].map.width, m_swaps[index].map.height);
		}
		else
		{
			return StrWPrintf(L"%02d: H (%02d, %02d) -> (%02d, %02d) %02dx%02d", index + 1,
				m_swaps[index].heightmap.src_x, m_swaps[index].heightmap.src_y,
				m_swaps[index].heightmap.dst_x, m_swaps[index].heightmap.dst_y,
				m_swaps[index].heightmap.width, m_swaps[index].heightmap.height);
		}
	}
	else
	{
		const auto& door_sz_name = Door::SIZE_NAMES.at(m_doors[index].size);
		return StrWPrintf(L"%02d: (%02d, %02d) %s", index + 1, m_doors[index].x, m_doors[index].y,
			std::wstring(door_sz_name.cbegin(), door_sz_name.cend()).c_str());
	}
}

void TileSwapControlFrame::OnToggle(wxCommandEvent& evt)
{
	SetPage(static_cast<ID>(evt.GetId()));
}
