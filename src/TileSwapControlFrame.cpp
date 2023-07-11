#include "TileSwapControlFrame.h"
#include <Utils.h>
#include <RoomViewerFrame.h>

wxDEFINE_EVENT(EVT_TILESWAP_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_OPEN_PROPERTIES, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_TILESWAP_MOVE_DOWN, wxCommandEvent);

TileSwapControlFrame::TileSwapControlFrame(RoomViewerFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst),
	  m_rvf(parent)
{
}

TileSwapControlFrame::~TileSwapControlFrame()
{
}

void TileSwapControlFrame::SetSwaps(const std::vector<TileSwap>& swaps)
{
	m_swaps = swaps;
	UpdateUI();
}

void TileSwapControlFrame::ResetSwaps()
{
	m_swaps.clear();
	UpdateUI();
}

int TileSwapControlFrame::GetMaxSelection() const
{
	return m_swaps.size();
}

void TileSwapControlFrame::Select()
{
	FireEvent(EVT_TILESWAP_SELECT);
}

void TileSwapControlFrame::Add()
{
	FireEvent(EVT_TILESWAP_ADD);
}

void TileSwapControlFrame::Delete()
{
	FireEvent(EVT_TILESWAP_DELETE);
}

void TileSwapControlFrame::MoveUp()
{
	FireEvent(EVT_TILESWAP_MOVE_UP);
}

void TileSwapControlFrame::MoveDown()
{
	FireEvent(EVT_TILESWAP_MOVE_DOWN);
}

void TileSwapControlFrame::OpenElement()
{
	FireEvent(EVT_TILESWAP_OPEN_PROPERTIES);
}

bool TileSwapControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return m_rvf->HandleKeyDown(key, modifiers);
}

std::string TileSwapControlFrame::MakeLabel(int index) const
{
	return StrPrintf("%02d: [%01d] (%02d, %02d) -> (%02d, %02d)", index + 1,
		static_cast<int>(m_swaps[index].mode),
		m_swaps[index].map.src_x, m_swaps[index].map.src_y,
		m_swaps[index].map.dst_x, m_swaps[index].map.dst_y);
}
