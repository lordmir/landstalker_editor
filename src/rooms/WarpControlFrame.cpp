#include <rooms/WarpControlFrame.h>
#include <landstalker/misc/Utils.h>
#include <rooms/RoomViewerFrame.h>

wxDEFINE_EVENT(EVT_WARP_SELECT, wxCommandEvent);
wxDEFINE_EVENT(EVT_WARP_OPEN_PROPERTIES, wxCommandEvent);
wxDEFINE_EVENT(EVT_WARP_ADD, wxCommandEvent);
wxDEFINE_EVENT(EVT_WARP_DELETE, wxCommandEvent);

WarpControlFrame::WarpControlFrame(RoomViewerFrame* parent, ImageList* imglst)
    : SelectionControlFrame(parent, imglst),
    m_rvf(parent)
{
    m_ctrl_move_up->Show(false);
    m_ctrl_move_down->Show(false);
}

WarpControlFrame::~WarpControlFrame()
{
}

void WarpControlFrame::SetWarps(const std::vector<Landstalker::WarpList::Warp>& warps)
{
    m_warps = warps;
    UpdateUI();
}

void WarpControlFrame::ResetWarps()
{
    m_warps.clear();
    UpdateUI();
}

int WarpControlFrame::GetMaxSelection() const
{
    return m_warps.size();
}

void WarpControlFrame::Select()
{
    FireEvent(EVT_WARP_SELECT);
}

void WarpControlFrame::OpenElement()
{
    FireEvent(EVT_WARP_OPEN_PROPERTIES);
}

bool WarpControlFrame::HandleKeyPress(unsigned int key, unsigned int modifiers)
{
    return m_rvf->HandleKeyDown(key, modifiers);
}

void WarpControlFrame::Add()
{
    FireEvent(EVT_WARP_ADD);
}

void WarpControlFrame::Delete()
{
    FireEvent(EVT_WARP_DELETE);
}

void WarpControlFrame::MoveUp()
{
}

void WarpControlFrame::MoveDown()
{
}

std::wstring WarpControlFrame::MakeLabel(int index) const
{
    uint16_t room = m_rvf->GetRoomNum();
    const auto& warp = m_warps.at(index);
    if (warp.IsValid())
    {
        if (room == warp.room1)
        {
            return Landstalker::StrWPrintf(L"%02d: %03d (%02d,%02d) <-> %03d (%02d, %02d)", index + 1, warp.room1,
                warp.x1, warp.y1, warp.room2, warp.x2, warp.y2);
        }
        else
        {
            return Landstalker::StrWPrintf(L"%02d: %03d (%02d,%02d) <-> %03d (%02d, %02d)", index + 1, warp.room2,
                warp.x2, warp.y2, warp.room1, warp.x1, warp.y1);
        }
    }
    else
    {
        if (room == warp.room1)
        {
            return Landstalker::StrWPrintf(L"%02d: %03d (%02d,%02d) <-> PENDING", index + 1, warp.room1,
                warp.x1, warp.y1);
        }
        else if (room == warp.room2)
        {
            return Landstalker::StrWPrintf(L"%02d: %03d (%02d,%02d) <-> PENDING", index + 1, warp.room2,
                warp.x2, warp.y2);
        }
        else
        {
            if (warp.room1 == 0xFFFF)
            {
                return Landstalker::StrWPrintf(L"%02d: PENDING <-> %03d (%02d,%02d)", index + 1, warp.room2,
                    warp.x2, warp.y2);
            }
            else
            {
                return Landstalker::StrWPrintf(L"%02d: PENDING <-> %03d (%02d,%02d)", index + 1, warp.room1,
                    warp.x1, warp.y1);
            }
        }
    }
}
