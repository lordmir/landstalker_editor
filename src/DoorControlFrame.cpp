#include "DoorControlFrame.h"
#include <Utils.h>
#include <RoomViewerFrame.h>

wxDEFINE_EVENT(EVT_DOOR_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_OPEN_PROPERTIES, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_DELETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_MOVE_UP, wxCommandEvent);
wxDEFINE_EVENT(EVT_DOOR_MOVE_DOWN, wxCommandEvent);

DoorControlFrame::DoorControlFrame(RoomViewerFrame* parent, ImageList* imglst)
	: SelectionControlFrame(parent, imglst),
	  m_rvf(parent)
{
}

DoorControlFrame::~DoorControlFrame()
{
}

void DoorControlFrame::SetDoors(const std::vector<Door>& doors)
{
	m_doors = doors;
	UpdateUI();
}

void DoorControlFrame::ResetSwaps()
{
	m_doors.clear();
	UpdateUI();
}

int DoorControlFrame::GetMaxSelection() const
{
	return m_doors.size();
}

void DoorControlFrame::Select()
{
	FireEvent(EVT_DOOR_SELECT);
}

void DoorControlFrame::Add()
{
	FireEvent(EVT_DOOR_ADD);
}

void DoorControlFrame::Delete()
{
	FireEvent(EVT_DOOR_DELETE);
}

void DoorControlFrame::MoveUp()
{
	FireEvent(EVT_DOOR_MOVE_UP);
}

void DoorControlFrame::MoveDown()
{
	FireEvent(EVT_DOOR_MOVE_DOWN);
}

void DoorControlFrame::OpenElement()
{
	FireEvent(EVT_DOOR_OPEN_PROPERTIES);
}

bool DoorControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
	return m_rvf->HandleKeyDown(key, modifiers);
}

std::string DoorControlFrame::MakeLabel(int index) const
{
	return StrPrintf("%02d: (%02d, %02d) %s", index + 1, m_doors[index].x, m_doors[index].y,
		Door::SIZE_NAMES.at(m_doors[index].size).c_str());
}
