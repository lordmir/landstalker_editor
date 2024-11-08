#include <user_interface/rooms/include/HeightmapEditorCtrl.h>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <user_interface/main/include/EditorFrame.h>
#include <user_interface/rooms/include/RoomViewerFrame.h>

wxDEFINE_EVENT(EVT_HEIGHTMAP_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_HEIGHTMAP_MOVE, wxCommandEvent);
wxDEFINE_EVENT(EVT_HEIGHTMAP_CELL_SELECTED, wxCommandEvent);

wxBEGIN_EVENT_TABLE(HeightmapEditorCtrl, wxScrolledCanvas)
EVT_ERASE_BACKGROUND(HeightmapEditorCtrl::OnEraseBackground)
EVT_PAINT(HeightmapEditorCtrl::OnPaint)
EVT_SIZE(HeightmapEditorCtrl::OnSize)
EVT_MOTION(HeightmapEditorCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(HeightmapEditorCtrl::OnMouseLeave)
EVT_LEFT_DOWN(HeightmapEditorCtrl::OnLeftDown)
EVT_RIGHT_DOWN(HeightmapEditorCtrl::OnRightDown)
EVT_LEFT_UP(HeightmapEditorCtrl::OnLeftUp)
EVT_RIGHT_UP(HeightmapEditorCtrl::OnRightUp)
EVT_LEFT_DCLICK(HeightmapEditorCtrl::OnLeftDClick)
EVT_RIGHT_DCLICK(HeightmapEditorCtrl::OnRightDClick)
EVT_SHOW(HeightmapEditorCtrl::OnShow)
wxEND_EVENT_TABLE()

HeightmapEditorCtrl::HeightmapEditorCtrl(wxWindow* parent, RoomViewerFrame* frame)
    : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
      m_g(nullptr),
      m_map(nullptr),
      m_frame(frame),
      m_roomnum(0),
      m_width(1),
      m_height(1),
      m_redraw(false),
      m_repaint(false),
      m_zoom(1.0),
      m_hovered(-1, -1),
      m_cpysrc(-1, -1),
      m_dragged(-1, -1),
      m_dragged_orig_pos(-1, -1),
      m_bmp(std::make_unique<wxBitmap>()),
      m_dragging(false),
      m_selected_region(-1),
      m_selected_is_src(false),
      m_preview_swap(false),
      m_scroll_rate(SCROLL_RATE),
      m_cursorid(wxCURSOR_ARROW)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
}

HeightmapEditorCtrl::~HeightmapEditorCtrl()
{
}

void HeightmapEditorCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
    m_g = gd;
    Refresh(true);
}

void HeightmapEditorCtrl::ClearGameData()
{
    m_g = nullptr;
    Refresh(true);
}

void HeightmapEditorCtrl::SetRoomNum(uint16_t roomnum)
{
    if (m_g)
    {
        m_map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
        m_entities = m_g->GetSpriteData()->GetRoomEntities(roomnum);
        m_warps = m_g->GetRoomData()->GetWarpsForRoom(roomnum);
        if (m_roomnum != roomnum)
        {
            SetSelectedSwap(-1);
            m_roomnum = roomnum;
        }
        else
        {
            if (!IsDoorSelected() && !IsSwapSelected())
            {
                SetSelectedSwap(-1);
            }
        }
        UpdateSwaps();
        UpdateDoors();
        RecreateBuffer();
    }
}

void HeightmapEditorCtrl::SetZoom(double zoom)
{
    m_zoom = zoom;
    RecreateBuffer();
}

void HeightmapEditorCtrl::RefreshGraphics()
{
    UpdateScroll();
    ForceRedraw();
}

void HeightmapEditorCtrl::UpdateSwaps()
{
    if (m_g)
    {
        auto old_swap_count = m_swaps.size();
        m_swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
        if (m_swaps.size() != old_swap_count)
        {
            m_preview_swap = false;
        }
    }
}

void HeightmapEditorCtrl::UpdateDoors()
{
    if (m_g)
    {
        m_doors = m_g->GetRoomData()->GetDoors(m_roomnum);
    }
}

void HeightmapEditorCtrl::UpdateWarps(const std::vector<WarpList::Warp>& warps)
{
    if (m_g)
    {
        m_warps = warps;
    }
}

void HeightmapEditorCtrl::UpdateEntities(const std::vector<Entity>& entities)
{
    if (m_g)
    {
        m_entities = entities;
    }
}

bool HeightmapEditorCtrl::IsCoordValid(const HeightmapEditorCtrl::Coord& c) const
{
    return (m_map != nullptr &&
        (c.first >= 0 && c.first < m_map->GetWidth() &&
            c.second >= 0 && c.second < m_map->GetHeight()));
}

bool HeightmapEditorCtrl::IsHoverValid() const
{
    return IsCoordValid(m_hovered);
}

void HeightmapEditorCtrl::SetSelectedSwap(int swap)
{
    int new_region = -1;
    if (swap > 0 && swap <= static_cast<int>(m_swaps.size()))
    {
        new_region = swap - 1;
    }
    if (new_region != m_selected_region)
    {
        wxLogDebug("Selected tileswap = %d, was %d", new_region, m_selected_region);
        m_selected_region = new_region;
        Refresh();
    }
}

int HeightmapEditorCtrl::GetSelectedSwap() const
{
    return IsSwapSelected() ? m_selected_region + 1 : -1;
}

bool HeightmapEditorCtrl::IsSwapSelected() const
{
    return (m_selected_region >= 0 && m_selected_region < static_cast<int>(m_swaps.size()));
}

const std::vector<TileSwap>& HeightmapEditorCtrl::GetTileswaps() const
{
    return m_swaps;
}

int HeightmapEditorCtrl::GetTotalTileswaps() const
{
    return m_swaps.size();
}

void HeightmapEditorCtrl::AddTileswap()
{
    m_swaps.push_back(TileSwap());
    SetSelectedSwap(m_swaps.size() - 1);
    Refresh();
}

void HeightmapEditorCtrl::DeleteTileswap()
{
    if (m_swaps.size() > 0 && IsSwapSelected())
    {
        auto prev = GetSelectedSwap();
        m_swaps.erase(m_swaps.cbegin() + prev);
        SetSelectedSwap(prev);
        Refresh();
    }
}

void HeightmapEditorCtrl::SetSelectedDoor(int door)
{
    int new_region = -1;
    if (door > 0 && door <= static_cast<int>(m_doors.size()))
    {
        new_region = 0xFF + door;
    }
    if (new_region != m_selected_region)
    {
        wxLogDebug("Selected door = %d, was %d", new_region, m_selected_region);
        m_selected_region = new_region;
        Refresh();
    }
}

int HeightmapEditorCtrl::GetSelectedDoor() const
{
    return IsDoorSelected() ? m_selected_region - 0xFF : -1;
}

bool HeightmapEditorCtrl::IsDoorSelected() const
{
    return (m_selected_region >= 0x100 && m_selected_region < static_cast<int>(0x100 + m_doors.size()));
}

const std::vector<Door>& HeightmapEditorCtrl::GetDoors() const
{
    return m_doors;
}

int HeightmapEditorCtrl::GetTotalDoors() const
{
    return m_doors.size();
}

bool HeightmapEditorCtrl::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
    RefreshCursor((modifiers & wxMOD_CONTROL) > 0);
    if (key == WXK_ESCAPE)
    {
        StopDrag(true);
        m_selected.clear();
        m_hovered = { -1, -1 };
        SetSelectedSwap(-1);
        SetSelectedDoor(-1);
        FireEvent(EVT_TILESWAP_SELECT, -1);
        FireEvent(EVT_DOOR_SELECT, -1);
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        RefreshStatusbar();
        ForceRedraw();
        return true;
    }
    if (!HandleRegionKeyDown(key, modifiers))
    {
        return HandleDrawKeyDown(key, modifiers);
    }
    return true;
}

bool HeightmapEditorCtrl::HandleRegionKeyDown(unsigned int key, unsigned int modifiers)
{
#define INCR(X) X = std::clamp(X + 1, 0 , 0x3F)
#define DECR(X) X = std::clamp(X - 1, 0 , 0x3F)
#define INCSZ(X) X = std::clamp(X + 1, 1 , 0x3F)
#define DECSZ(X) X = std::clamp(X - 1, 1 , 0x3F)
#define INCDSZ(X, MAX) X = static_cast<decltype(X)>((static_cast<int>(X) + MAX + 1) % MAX)
#define DECDSZ(X, MAX) X = static_cast<decltype(X)>((static_cast<int>(X) + MAX - 1) % MAX)

    bool upd = false;

    switch (key)
    {
    case WXK_TAB:
        if (!(m_swaps.empty() && m_doors.empty()))
        {
            int position = m_selected_region;
            if (modifiers == 0)
            {
                position++;
                if ((position >= static_cast<int>(m_swaps.size())) && (position < 0x100))
                {
                    position = 0x100;
                }
                if ((position < 0) || (position >= static_cast<int>(0x100 + m_doors.size())))
                {
                    position = m_swaps.empty() ? (m_doors.empty() ? -1 : 0x100) : 0;
                }
            }
            else if (modifiers == wxMOD_SHIFT)
            {
                position--;
                if ((position >= static_cast<int>(m_swaps.size())) && (position < 0x100))
                {
                    position = m_swaps.size() - 1;
                }
                if ((position < 0) || (position >= static_cast<int>(0x100 + m_doors.size())))
                {
                    position = m_doors.empty() ? (m_swaps.empty() ? -1 : m_swaps.size() - 1) : 0xFF + m_doors.size();
                }
            }
            if (position != m_selected_region)
            {
                m_selected_region = position;
                if (IsDoorSelected())
                {
                    FireEvent(EVT_DOOR_SELECT, GetSelectedDoor());
                }
                else if (IsSwapSelected())
                {
                    FireEvent(EVT_TILESWAP_SELECT, GetSelectedSwap());
                }
                else
                {
                    FireEvent(EVT_DOOR_SELECT, -1);
                    FireEvent(EVT_TILESWAP_SELECT, -1);
                }
                Refresh();
            }
        }
        break;
    case WXK_INSERT:
        if ((modifiers & wxMOD_SHIFT) > 0)
        {
            FireEvent(EVT_DOOR_ADD);
        }
        else
        {
            FireEvent(EVT_TILESWAP_ADD);
        }
        break;
    case WXK_DELETE:
        if (IsSwapSelected())
        {
            FireEvent(EVT_TILESWAP_DELETE, GetSelectedSwap());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_DELETE, GetSelectedDoor());
        }
        break;
    case '[':
    case '{':
        if (IsSwapSelected())
        {
            FireEvent(EVT_TILESWAP_MOVE_UP, GetSelectedSwap());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_MOVE_UP, GetSelectedDoor());
        }
        break;
    case ']':
    case '}':
        if (IsSwapSelected())
        {
            FireEvent(EVT_TILESWAP_MOVE_DOWN, GetSelectedSwap());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_MOVE_DOWN, GetSelectedDoor());
        }
        break;
    case WXK_LEFT:
    case 'a':
    case 'A':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECDSZ(m_doors[GetSelectedDoor() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_doors[GetSelectedDoor() - 1].x);
                upd = true;
            }
        }
        else if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                DECR(m_swaps[GetSelectedSwap() - 1].heightmap.dst_x);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECSZ(m_swaps[GetSelectedSwap() - 1].heightmap.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedSwap() - 1].heightmap.src_x);
                upd = true;
            }
        }
        break;
    case WXK_RIGHT:
    case 'd':
    case 'D':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCDSZ(m_doors[GetSelectedDoor() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_doors[GetSelectedDoor() - 1].x);
                upd = true;
            }
        }
        else if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                INCR(m_swaps[GetSelectedSwap() - 1].heightmap.dst_x);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCSZ(m_swaps[GetSelectedSwap() - 1].heightmap.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedSwap() - 1].heightmap.src_x);
                upd = true;
            }
        }
        break;
    case WXK_UP:
    case 'w':
    case 'W':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECDSZ(m_doors[GetSelectedDoor() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_doors[GetSelectedDoor() - 1].y);
                upd = true;
            }
        }
        else if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                DECR(m_swaps[GetSelectedSwap() - 1].heightmap.dst_y);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECSZ(m_swaps[GetSelectedSwap() - 1].heightmap.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedSwap() - 1].heightmap.src_y);
                upd = true;
            }
        }
        break;
    case WXK_DOWN:
    case 's':
    case 'S':
        if (IsDoorSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCDSZ(m_doors[GetSelectedDoor() - 1].size, Door::SIZES.size());
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_doors[GetSelectedDoor() - 1].y);
                upd = true;
            }
        }
        else if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_ALT))
            {
                INCR(m_swaps[GetSelectedSwap() - 1].heightmap.dst_y);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCSZ(m_swaps[GetSelectedSwap() - 1].heightmap.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedSwap() - 1].heightmap.src_y);
                upd = true;
            }
        }
        break;
    case WXK_RETURN:
        if (IsSwapSelected())
        {
            FireEvent(EVT_TILESWAP_OPEN_PROPERTIES, GetSelectedSwap());
        }
        else if (IsDoorSelected())
        {
            FireEvent(EVT_DOOR_OPEN_PROPERTIES, GetSelectedDoor());
        }
        break;
    case 'P':
    case 'p':
        if (!m_preview_swap && IsSwapSelected())
        {
            m_preview_swap = true;
        }
        else if (m_preview_swap)
        {
            m_preview_swap = false;
        }
        RefreshStatusbar();
        ForceRedraw();
        break;
    }

    if (upd)
    {
        if (m_g)
        {
            m_g->GetRoomData()->SetDoors(m_roomnum, m_doors);
            m_g->GetRoomData()->SetTileSwaps(m_roomnum, m_swaps);
            if (IsDoorSelected())
            {
                FireEvent(EVT_DOOR_UPDATE, GetSelectedDoor());
            }
            else if (IsSwapSelected())
            {
                FireEvent(EVT_TILESWAP_UPDATE, GetSelectedSwap());
            }
        }
        Refresh();
    }

    return upd;
#undef DECDSZ
#undef INCDSZ
#undef DECSZ
#undef INCSZ
#undef DECR
#undef INCR
}

bool HeightmapEditorCtrl::HandleKeyUp(unsigned int /*key*/, unsigned int /*modifiers*/)
{
    return false;
}

bool HeightmapEditorCtrl::HandleDrawKeyDown(unsigned int key, unsigned int modifiers)
{
    switch(key)
    {
    case WXK_ESCAPE:
        ClearSelection();
        return false;
    case WXK_UP:
    case 'w':
    case 'W':
        if (modifiers == (wxMOD_SHIFT | wxMOD_ALT))
        {
            InsertRowAbove();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapUp();
        }
        else
        {
            NudgeSelectionUp();
        }
        return false;
    case WXK_DOWN:
    case 's':
    case 'S':
        if (modifiers == (wxMOD_SHIFT | wxMOD_ALT))
        {
            InsertRowBelow();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapDown();
        }
        else
        {
            NudgeSelectionDown();
        }
        return false;
    case WXK_LEFT:
    case 'a':
    case 'A':
        if (modifiers == (wxMOD_SHIFT | wxMOD_ALT))
        {
            InsertColumnLeft();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapLeft();
        }
        else
        {
            NudgeSelectionLeft();
        }
        return false;
    case WXK_RIGHT:
    case 'd':
    case 'D':
        if (modifiers == (wxMOD_SHIFT | wxMOD_ALT))
        {
            InsertColumnRight();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapRight();
        }
        else
        {
            NudgeSelectionRight();
        }
        return false;
    case WXK_PAGEUP:
        IncreaseHeight();
        return false;
    case WXK_PAGEDOWN:
        DecreaseSelectedHeight();
        return false;
    case WXK_SPACE:
        ToggleSelectedPlayerPassable();
        return false;
    case 'n':
    case 'N':
        ToggleSelectedNPCPassable();
        return false;
    case 'r':
    case 'R':
        ToggleSelectedRaftTrack();
        return false;
    case WXK_ADD:
    case '+':
        if (modifiers == (wxMOD_SHIFT | wxMOD_ALT))
        {
            IncrementSelectedRestrictions();
        }
        else
        {
            IncrementSelectedType();
        }
        return false;
    case WXK_SUBTRACT:
    case '-':
        if (modifiers == (wxMOD_SHIFT | wxMOD_ALT))
        {
            DecrementSelectedRestrictions();
        }
        else
        {
            DecrementSelectedType();
        }
        return false;
    case 'C':
    case 'c':
        for (size_t i = 0; i < m_selected.size(); ++i) {
            SetSelectedType(i, 0);
        }
        return false;
    case 'X':
    case 'x':
        for (size_t i = 0; i < m_selected.size(); ++i) {
            SetSelectedRestrictions(i, 0);
        }
        return false;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        //SetSelectedHeight(key - '0');
        //return false;
    case WXK_NUMPAD0:
    case WXK_NUMPAD1:
    case WXK_NUMPAD2:
    case WXK_NUMPAD3:
    case WXK_NUMPAD4:
    case WXK_NUMPAD5:
    case WXK_NUMPAD6:
    case WXK_NUMPAD7:
    case WXK_NUMPAD8:
    case WXK_NUMPAD9:
        //SetSelectedHeight(key - WXK_NUMPAD0);
        return false;
    case WXK_DELETE:
        if (modifiers == (wxMOD_SHIFT | wxMOD_ALT))
        {
            DeleteColumn();
        }
        else if (modifiers == wxMOD_ALT)
        {
            DeleteRow();
        }
        else
        {
            ClearSelectedCell();
        }
        return false;
    }
	return true;
}

void HeightmapEditorCtrl::ClearSelection()
{
    m_selected.clear();
    FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
    Refresh(false);
}

void HeightmapEditorCtrl::SetSelection(int ix, int iy)
{
    if (m_selected[0].first != ix || m_selected[0].second != iy)
    {
        if (ix >= 0 && ix < m_map->GetHeightmapWidth() && iy >= 0 && iy < m_map->GetHeightmapWidth())
        {
            m_selected[0] = { ix, iy };
            FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
            Refresh(false);
        }
        else
        {
            ClearSelection();
        }
    }
}

std::pair<int, int> HeightmapEditorCtrl::GetSelection(int index) const
{
    return m_selected[index];
}

bool HeightmapEditorCtrl::IsSingleSelection() const
{
    return m_selected.size() == 1;
}

bool HeightmapEditorCtrl::IsMultipleSelection() const
{
    return m_selected.size() > 1;
}

bool HeightmapEditorCtrl::IsSelectionEmpty() const
{
    return m_selected.empty();
}

void HeightmapEditorCtrl::NudgeSelectionUp()
{
    if(m_selected.size() == 1)
    {
        bool upd = false;
        if (m_selected[0].first == -1 && m_hovered.first == -1)
        {
            m_selected[0] = { 0, 0 };
            upd = true;
        }
        else if (m_selected[0].first > 0)
        {
            if (m_selected[0].first == -1)
            {
                m_selected[0] = m_hovered;
            }
            m_selected[0].first--;
            upd = true;
        }
        if (upd)
        {
            FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
            Refresh(false);
        }
    }
}

void HeightmapEditorCtrl::NudgeSelectionDown()
{
    if(m_selected.size() == 1)
    {
        bool upd = false;
        if (m_selected[0].first == -1 && m_hovered.first == -1)
        {
            m_selected[0] = { 0, 0 };
            upd = true;
        }
        else if (m_selected[0].first < m_map->GetHeightmapWidth() - 1)
        {
            if (m_selected[0].first == -1)
            {
                m_selected[0] = m_hovered;
            }
            m_selected[0].first++;
            upd = true;
        }
        if (upd)
        {
            FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
            Refresh(false);
        }
    }
}

void HeightmapEditorCtrl::NudgeSelectionLeft()
{
    if(m_selected.size() == 1)
    {
        bool upd = false;
        if (m_selected[0].first == -1 && m_hovered.first == -1)
        {
            m_selected[0] = { 0, 0 };
            upd = true;
        }
        else if (m_selected[0].second < m_map->GetHeightmapHeight() - 1)
        {
            if (m_selected[0].first == -1)
            {
                m_selected[0] = m_hovered;
            }
            m_selected[0].second++;
            upd = true;
        }
        if (upd)
        {
            FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
            Refresh(false);
        }
    }
}

void HeightmapEditorCtrl::NudgeSelectionRight()
{
    if(m_selected.size() == 1)
    {
        bool upd = false;
        if (m_selected[0].first == -1 && m_hovered.first == -1)
        {
            m_selected[0] = { 0, 0 };
            upd = true;
        }
        else if (m_selected[0].second > 0)
        {
            if (m_selected[0].first == -1)
            {
                m_selected[0] = m_hovered;
            }
            m_selected[0].second--;
            upd = true;
        }
        if (upd)
        {
            FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
            Refresh(false);
        }
    }
}

void HeightmapEditorCtrl::NudgeHeightmapUp()
{
    int t = m_map->GetTop();
    if (t > 0)
    {
        m_map->SetTop(t - 1);
        FireEvent(EVT_HEIGHTMAP_MOVE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::NudgeHeightmapDown()
{
    int t = m_map->GetTop();
    if (t < 63)
    {
        m_map->SetTop(t + 1);
        FireEvent(EVT_HEIGHTMAP_MOVE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::NudgeHeightmapLeft()
{
    int l = m_map->GetLeft();
    if (l > 0)
    {
        m_map->SetLeft(l - 1);
        FireEvent(EVT_HEIGHTMAP_MOVE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::NudgeHeightmapRight()
{
    int l = m_map->GetLeft();
    if (l < 63)
    {
        m_map->SetLeft(l + 1);
        FireEvent(EVT_HEIGHTMAP_MOVE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::InsertRowAbove()
{
    if (m_selected.size() == 1 && m_selected[0].second != -1 && m_map->GetHeightmapWidth() < 64)
    {
        m_map->InsertHeightmapRow(m_selected[0].first);
        m_selected[0].first++;
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::InsertRowBelow()
{
    if (m_selected[0].first != -1 && m_map->GetHeightmapWidth() < 64)
    {
        m_map->InsertHeightmapRow(m_selected[0].first);
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::DeleteRow()
{
    if (m_selected.size() == 1 && m_selected[0].first != -1 && m_map->GetHeightmapWidth() > 1)
    {
        m_map->DeleteHeightmapRow(m_selected[0].first);
        if (m_selected[0].first >= m_map->GetHeightmapWidth())
        {
            m_selected[0].first = m_map->GetHeightmapWidth() - 1;
        }
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::InsertColumnLeft()
{
    if (m_selected.size() == 1 && m_selected[0].first != -1 && m_map->GetHeightmapHeight() < 64)
    {
        m_map->InsertHeightmapColumn(m_selected[0].second);
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::InsertColumnRight()
{
    if (m_selected.size() == 1 && m_selected[0].second != -1 && m_map->GetHeightmapHeight() < 64)
    {
        m_map->InsertHeightmapColumn(m_selected[0].second);
        m_selected[0].second++;
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::DeleteColumn()
{
    if (m_selected.size() == 1 && m_selected[0].first != -1 && m_map->GetHeightmapHeight() > 1)
    {
        m_map->DeleteHeightmapColumn(m_selected[0].second);
        if (m_selected[0].second >= m_map->GetHeightmapHeight())
        {
            m_selected[0].second = m_map->GetHeightmapHeight() - 1;
        }
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

uint8_t HeightmapEditorCtrl::GetSelectedHeight(int index) const
{
    return m_map->GetHeight({ m_selected[index].first, m_selected[index].second });
}

void HeightmapEditorCtrl::SetSelectedHeight(int index, uint8_t height)
{
    m_map->SetHeight({ m_selected[index].first, m_selected[index].second }, height);
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

void HeightmapEditorCtrl::IncreaseHeight()
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        uint8_t height = GetSelectedHeight(i);
        if (height < 15)
        {
            ++height;
        }
        SetSelectedHeight(i, height);
    }
}

void HeightmapEditorCtrl::DecreaseSelectedHeight()
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        uint8_t height = GetSelectedHeight(i);
        if (height > 0)
        {
            --height;
        }
        SetSelectedHeight(i, height);
    }
}

void HeightmapEditorCtrl::ClearSelectedCell()
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        m_map->SetHeight({ m_selected[i].first, m_selected[i].second }, 0);
        m_map->SetCellProps({ m_selected[i].first, m_selected[i].second }, 4);
        m_map->SetCellType({ m_selected[i].first, m_selected[i].second }, 0);
    }
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

uint8_t HeightmapEditorCtrl::GetSelectedRestrictions(int index) const
{
    return m_map->GetCellProps({ m_selected[index].first, m_selected[index].second });
}

void HeightmapEditorCtrl::SetSelectedRestrictions(int index, uint8_t restrictions)
{
    m_map->SetCellProps({ m_selected[index].first, m_selected[index].second }, restrictions);
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

bool HeightmapEditorCtrl::IsSelectedPlayerPassable() const
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if ((GetSelectedRestrictions(i) & 0x04) != 0) {
            return false;
        }
    }
    return true;
}

void HeightmapEditorCtrl::ToggleSelectedPlayerPassable()
{
    bool hasFlag = IsSelectedPlayerPassable();
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if (hasFlag) {
            SetSelectedRestrictions(i, GetSelectedRestrictions(i) | 0x04);
        } else {
            SetSelectedRestrictions(i, GetSelectedRestrictions(i) & ~0x04);
        }
    }
}

bool HeightmapEditorCtrl::IsSelectedNPCPassable() const
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if ((GetSelectedRestrictions(i) & 0x02) != 0) {
            return false;
        }
    }
    return true;
}

void HeightmapEditorCtrl::ToggleSelectedNPCPassable()
{
    bool hasFlag = IsSelectedNPCPassable();
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if (hasFlag) {
            SetSelectedRestrictions(i, GetSelectedRestrictions(i) | 0x02);
        } else {
            SetSelectedRestrictions(i, GetSelectedRestrictions(i) & ~0x02);
        }
    }
}

bool HeightmapEditorCtrl::IsSelectedRaftTrack() const
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if ((GetSelectedRestrictions(i) & 0x01) != 0) {
            return false;
        }
    }
    return true;
}

void HeightmapEditorCtrl::ToggleSelectedRaftTrack()
{
    bool hasFlag = IsSelectedRaftTrack();
    for (size_t i = 0; i < m_selected.size(); ++i) {
        if (hasFlag) {
            SetSelectedRestrictions(i, GetSelectedRestrictions(i) | 0x01);
        } else {
            SetSelectedRestrictions(i, GetSelectedRestrictions(i) & ~0x01);
        }
    }
}

void HeightmapEditorCtrl::IncrementSelectedRestrictions()
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        SetSelectedRestrictions(i, (GetSelectedRestrictions(i) + 1) & 0x0F);
    }
}

void HeightmapEditorCtrl::DecrementSelectedRestrictions()
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        SetSelectedRestrictions(i, (GetSelectedRestrictions(i) - 1) & 0x0F);
    }
}

uint8_t HeightmapEditorCtrl::GetSelectedType(int index) const
{
    return m_map->GetCellType({m_selected[index].first, m_selected[index].second});
}

void HeightmapEditorCtrl::SetSelectedType(int index, uint8_t type)
{
    m_map->SetCellType({ m_selected[index].first, m_selected[index].second }, type);
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

void HeightmapEditorCtrl::IncrementSelectedType()
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        SetSelectedType(i, (GetSelectedType(i) + 1) & 0xFF);
    }
}

void HeightmapEditorCtrl::DecrementSelectedType()
{
    for (size_t i = 0; i < m_selected.size(); ++i) {
        SetSelectedType(i, (GetSelectedType(i) - 1) & 0xFF);
    }
}

bool HeightmapEditorCtrl::HandleMouse(MouseEventType type, bool left_down, bool right_down, unsigned int modifiers, int x, int y)
{
    // Refresh hover position
    if (type != MouseEventType::LEAVE && UpdateHoveredPosition(x, y))
    {
        RefreshStatusbar();
        Refresh(false);
    }

    // Refresh cursor
    RefreshCursor((modifiers & wxMOD_CONTROL) > 0);

    // Refresh drag state
    if (m_dragging)
    {
        if (left_down)
        {
            RefreshDrag();
        }
        else if (right_down)
        {
            StopDrag(true);
        }
        else
        {
            StopDrag();
        }
        return true;
    }

    // Handle buttons
    if (type == MouseEventType::LEFT_DCLICK)
    {
        return HandleLeftDClick(modifiers);
    }
    else if (type == MouseEventType::LEFT_DOWN)
    {
        return HandleLeftDown(modifiers);
    }
    else if (type == MouseEventType::RIGHT_DOWN)
    {
        return HandleRightDown(modifiers);
    }

    return false;
}

bool HeightmapEditorCtrl::HandleLeftDown(unsigned int modifiers)
{
    if ((modifiers & wxMOD_CONTROL) > 0)
    {
        auto sw = GetFirstSwapRegion(m_hovered);
        auto dw = GetFirstDoorRegion(m_hovered);
        if (sw.first != -1)
        {
            FireEvent(EVT_TILESWAP_SELECT, sw.first + 1);
            m_selected_is_src = sw.second;
        }
        else if (dw != -1)
        {
            FireEvent(EVT_DOOR_SELECT, dw + 1);
        }
        else if (IsDoorSelected() || IsSwapSelected())
        {
            SetSelectedDoor(-1);
            SetSelectedSwap(-1);
            FireEvent(EVT_TILESWAP_SELECT, -1);
            FireEvent(EVT_DOOR_SELECT, -1);
        }
        Refresh();
    }
    else if ((modifiers & wxMOD_SHIFT) > 0)
    {
        // Multiple selection
        auto it = std::find(m_selected.begin(), m_selected.end(), m_hovered);
        if (it != m_selected.end()) {
            m_selected.erase(it);
        }
        else
        {
            m_selected.push_back(m_hovered);
        }
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        ForceRedraw();
    }
    else
    {
        //if (m_selected[0] != m_hovered)
        //{
            m_selected.clear();
            m_selected.push_back(m_hovered);
            FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
            if (m_cpysrc.first != -1 && m_selected[0].first != -1 && m_selected[0] != m_cpysrc)
            {
                m_map->SetHeight({ m_selected[0].first, m_selected[0].second }, m_map->GetHeight({ m_cpysrc.first, m_cpysrc.second }));
                m_map->SetCellProps({ m_selected[0].first, m_selected[0].second }, m_map->GetCellProps({ m_cpysrc.first, m_cpysrc.second }));
                m_map->SetCellType({ m_selected[0].first, m_selected[0].second }, m_map->GetCellType({ m_cpysrc.first, m_cpysrc.second }));
                FireEvent(EVT_HEIGHTMAP_UPDATE);
            }
            ForceRedraw();
        //}
    }
    return false;
}

bool HeightmapEditorCtrl::HandleLeftDClick(unsigned int /*modifiers*/)
{
    return false;
}

bool HeightmapEditorCtrl::HandleRightDown(unsigned int modifiers)
{
    if ((modifiers & wxMOD_CONTROL) == 0)
    {
        if (m_selected[0] != m_hovered)
        {
            m_selected[0] = m_hovered;
            m_cpysrc = m_selected[0];
            Refresh(false);
        }
    }
    else
    {
        auto sw = GetFirstSwapRegion(m_hovered);
        auto dw = GetFirstDoorRegion(m_hovered);
        if (sw.first != -1)
        {
            FireEvent(EVT_TILESWAP_SELECT, sw.first + 1);
            m_selected_is_src = sw.second;
        }
        else if (dw != -1)
        {
            FireEvent(EVT_DOOR_SELECT, dw + 1);
        }
        else if (IsDoorSelected() || IsSwapSelected())
        {
            SetSelectedDoor(-1);
            SetSelectedSwap(-1);
            FireEvent(EVT_TILESWAP_SELECT, -1);
            FireEvent(EVT_DOOR_SELECT, -1);
        }
        if (!m_preview_swap && IsSwapSelected())
        {
            m_preview_swap = true;
        }
        else if (m_preview_swap)
        {
            m_preview_swap = false;
        }
        RefreshStatusbar();
        ForceRedraw();
    }
    return false;
}

bool HeightmapEditorCtrl::AnySelectedMaxHeight()
{
    return std::any_of(m_selected.begin(), m_selected.end(), [&, i = 0](const auto&) mutable { return GetSelectedHeight(i++) < 15; });
}

bool HeightmapEditorCtrl::AnySelectedMinHeight()
{
    return std::any_of(m_selected.begin(), m_selected.end(), [&, i = 0](const auto&) mutable { return GetSelectedHeight(i++) > 0; });
}

void HeightmapEditorCtrl::RefreshStatusbar()
{
    std::string msg = "";
    std::string swpmsg = "";
    if (m_hovered.first != -1)
    {
        uint8_t z = 0, r = 0, t = 0;
        if (m_map != nullptr)
        {
            z = m_map->GetHeight({ m_hovered.first, m_hovered.second });
            r = m_map->GetCellProps({ m_hovered.first, m_hovered.second });
            t = m_map->GetCellType({ m_hovered.first, m_hovered.second });
        }
        msg = StrPrintf("(%04d, %04d) : Z:%02d R:%01X T:%02X", m_hovered.first, m_hovered.second, z, r, t);
    }
    if (IsSwapSelected() || IsDoorSelected())
    {
        int sel = m_selected_region & 0xFF;
        swpmsg = StrPrintf("Selected %s #%d", IsSwapSelected() ? "tile swap" : "door", sel);
        if (m_preview_swap)
        {
            swpmsg += " [PREVIEW ACTIVE]";
        }
    }
    FireUpdateStatusEvent(msg, 0);
    FireUpdateStatusEvent(swpmsg, 2);
}

void HeightmapEditorCtrl::RefreshCursor(bool ctrl_down)
{
    if (m_dragging)
    {
        UpdateCursor(wxCURSOR_SIZING);
    }
    else if (ctrl_down)
    {
        auto sw = GetFirstSwapRegion(m_hovered);
        auto dw = GetFirstDoorRegion(m_hovered);
        if (sw.first != -1 || dw != -1)
        {
            UpdateCursor(wxCURSOR_HAND);
        }
        else
        {
            UpdateCursor(wxCURSOR_ARROW);
        }
    }
    else
    {
        UpdateCursor(wxCURSOR_ARROW);
    }
}

void HeightmapEditorCtrl::UpdateCursor(wxStockCursor cursor)
{
    if (cursor != m_cursorid)
    {
        SetCursor(cursor);
        m_cursorid = cursor;
    }
}

void HeightmapEditorCtrl::DrawRoomHeightmapBackground(wxDC& dc)
{
    if (m_map)
    {
        dc.SetPen(*wxTRANSPARENT_PEN);

        auto lines = GetTilePoly(0, 0);
        for (int iy = 0; iy < m_map->GetHeightmapHeight(); ++iy)
        {
            for (int ix = 0; ix < m_map->GetHeightmapWidth(); ++ix)
            {
                int x = (m_map->GetHeightmapHeight() + ix - iy - 1) * TILE_WIDTH / 2;
                int y = (ix + iy) * TILE_HEIGHT / 2;
                auto [restrictions, z, type] = GetHeightmapCell(ix, iy);
                dc.SetBrush(wxBrush(GetCellBackground(restrictions, type, z)));
                dc.DrawPolygon(lines.size(), lines.data(), x, y);
            }
        }
    }
}

void HeightmapEditorCtrl::DrawRoomHeightmapForeground(wxDC& dc)
{
    if (m_map)
    {
        auto lines = GetTilePoly(0, 0);
        auto font = wxFont(wxSize(0, TILE_HEIGHT / 2), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false);
        dc.SetFont(font);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColor(128, 128, 128)));
        for (int iy = 0; iy < m_map->GetHeightmapHeight(); ++iy)
        {
            for (int ix = 0; ix < m_map->GetHeightmapWidth(); ++ix)
            {
                int x = (m_map->GetHeightmapHeight() + ix - iy - 1) * TILE_WIDTH / 2;
                int y = (ix + iy) * TILE_HEIGHT / 2;
                auto [restrictions, z, type] = GetHeightmapCell(ix, iy);
                if (!IsCellHidden(restrictions, type, z))
                {
                    std::string lbl1 = StrPrintf("%02X", type);
                    std::string lbl2 = StrPrintf("%X", z);
                    std::string lbl3 = StrPrintf("%X", restrictions);
                    wxCoord tw, th;
                    dc.SetTextForeground(type == 0 ? *wxLIGHT_GREY : wxColor(0xFF00FF));
                    dc.GetTextExtent(lbl1, &tw, &th);
                    dc.DrawText(lbl1, x + TILE_WIDTH / 2 - tw / 2, y + TILE_HEIGHT / 4 - th / 2 + 2);
                    dc.SetTextForeground(*wxCYAN);
                    dc.GetTextExtent(lbl2, &tw, &th);
                    dc.DrawText(lbl2, x + TILE_WIDTH / 2 - 5 * tw / 4, y + 5 * TILE_HEIGHT / 8 - th / 2 + 2);
                    dc.SetTextForeground(restrictions == 0 ? *wxLIGHT_GREY : *wxYELLOW);
                    dc.GetTextExtent(lbl3, &tw, &th);
                    dc.DrawText(lbl3, x + TILE_WIDTH / 2 + 1 * tw / 4, y + 5 * TILE_HEIGHT / 8 - th / 2 + 2);
                }
            }
        }
        for (int iy = 0; iy < m_map->GetHeightmapHeight(); ++iy)
        {
            for (int ix = 0; ix < m_map->GetHeightmapWidth(); ++ix)
            {
                int x = (m_map->GetHeightmapHeight() + ix - iy - 1) * TILE_WIDTH / 2;
                int y = (ix + iy) * TILE_HEIGHT / 2;
                dc.DrawPolygon(lines.size(), lines.data(), x, y);
            }
        }
    }
}

void HeightmapEditorCtrl::DrawCellRange(wxDC& dc, float x, float y, float w, float h, int s)
{
    int xx = (m_map->GetHeightmapHeight() + x - y - 1) * TILE_WIDTH / 2;
    int yy = (x + y) * TILE_HEIGHT / 2;
    auto lines = GetTilePoly(0, 0, w, h, s);
    dc.DrawPolygon(lines.size(), lines.data(), xx, yy);
}

void HeightmapEditorCtrl::DrawDoors(wxDC& dc)
{
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    int hovered = -1;
    if (GetFirstSwapRegion(m_hovered).first != -1)
    {
        hovered = GetFirstDoorRegion(m_hovered);
    }
    for (int i = static_cast<int>(m_doors.size()) - 1; i >= 0; --i)
    {
        const auto& door = m_doors.at(i);
        std::unique_ptr<wxPen> m_door_pen(
            new wxPen(
                hovered == i ? wxColor(192, 255, 240) : wxColor(128, 255, 192),
                m_selected_region == 0x100 + i ? 4 : 2,
                wxPENSTYLE_DOT
            )
        );
        dc.SetPen(*m_door_pen);
        if (m_map && GetHeightmapCellType(door.x, door.y) == static_cast<int>(Tilemap3D::FloorType::DOOR_NW))
        {
            DrawCellRange(dc, door.x, door.y, 1, Door::SIZES.at(door.size).first, 2);
        }
        else
        {
            DrawCellRange(dc, door.x, door.y, Door::SIZES.at(door.size).first, 1, 2);
        }
    }
}

void HeightmapEditorCtrl::DrawTileSwaps(wxDC& dc)
{
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    auto hovered = GetFirstSwapRegion(m_hovered);
    for (int i = static_cast<int>(m_swaps.size()) - 1; i >= 0; --i)
    {
        std::unique_ptr<wxPen> m_swap_src_pen(
            new wxPen(
                hovered.first == i ? wxColor(128, 255, 128) : *wxGREEN,
                m_selected_region == i ? 4 : 2,
                wxPENSTYLE_DOT
            )
        );
        std::unique_ptr<wxPen> m_swap_dst_pen(
            new wxPen(
                hovered.first == i ? wxColor(140, 240, 255) : wxColor(64, 192, 255),
                m_selected_region == i ? 4 : 2,
                wxPENSTYLE_DOT
            )
        );
        const TileSwap& swap = m_swaps.at(i);
        dc.SetPen(*m_swap_dst_pen);
        DrawCellRange(dc, swap.heightmap.dst_x, swap.heightmap.dst_y, swap.heightmap.width, swap.heightmap.height, 2);
        dc.SetPen(*m_swap_src_pen);
        DrawCellRange(dc, swap.heightmap.src_x, swap.heightmap.src_y, swap.heightmap.width, swap.heightmap.height, 2);
    }
}

void HeightmapEditorCtrl::DrawWarps(wxDC& dc)
{
    std::unique_ptr<wxPen> m_warp_pen(new wxPen(*wxYELLOW, 2, wxPENSTYLE_SHORT_DASH));
    dc.SetPen(*m_warp_pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    for (const auto& warp : m_warps)
    {
        if (m_roomnum == warp.room1)
        {
            DrawCellRange(dc, warp.x1 - 12, warp.y1 - 12, warp.x_size, warp.y_size, 0);
        }
        if (m_roomnum == warp.room2)
        {
            DrawCellRange(dc, warp.x2 - 12, warp.y2 - 12, warp.x_size, warp.y_size, 0);
        }
    }
}

void HeightmapEditorCtrl::DrawEntities(wxDC& dc)
{
    std::unique_ptr<wxBrush> m_entity_brush1(new wxBrush(wxColor(32, 32, 32), wxBRUSHSTYLE_CROSS_HATCH));
    std::unique_ptr<wxBrush> m_entity_brush2(new wxBrush(wxColor(32, 32, 32), wxBRUSHSTYLE_CROSSDIAG_HATCH));
    dc.SetPen(*wxBLUE_PEN);

    for (const auto& entity : m_entities)
    {
        auto hitbox = m_g->GetSpriteData()->GetEntityHitbox(entity.GetType());
        dc.SetBrush(*m_entity_brush1);
        //DrawCellRange(dc, entity.GetXDbl() - 12.5, entity.GetYDbl() - 12.5, hitbox.first / 8.0, hitbox.first / 8.0, 2);
        dc.SetBrush(*m_entity_brush2);
        DrawCellRange(dc, entity.GetXDbl() - 12.5, entity.GetYDbl() - 12.5, hitbox.first / 8.0, hitbox.first / 8.0, 2);
    }
}

void HeightmapEditorCtrl::DrawSelectionCursors(wxDC& dc)
{
    auto lines = GetTilePoly(0, 0);

    // Draw cursor for hovered tile
    if (m_hovered.first != -1 && std::find(m_selected.begin(), m_selected.end(), m_hovered) == m_selected.end())
    {
        int x = (m_map->GetHeightmapHeight() + m_hovered.first - m_hovered.second - 1) * TILE_WIDTH / 2;
        int y = (m_hovered.first + m_hovered.second) * TILE_HEIGHT / 2;
        dc.SetPen(*wxWHITE_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawPolygon(lines.size(), lines.data(), x, y);
    }

    // Draw cursor for each selected tile
    for (const auto& coord : m_selected)
    {
        if (coord.first != -1 && coord != m_hovered)
        {
            int x = (m_map->GetHeightmapHeight() + coord.first - coord.second - 1) * TILE_WIDTH / 2;
            int y = (coord.first + coord.second) * TILE_HEIGHT / 2;
            dc.SetPen(*wxYELLOW_PEN);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawPolygon(lines.size(), lines.data(), x, y);
        }
    }

    // Draw cursor if hovered tile is also selected
    if (m_hovered.first != -1 && std::find(m_selected.begin(), m_selected.end(), m_hovered) != m_selected.end())
    {
        int x = (m_map->GetHeightmapHeight() + m_hovered.first - m_hovered.second - 1) * TILE_WIDTH / 2;
        int y = (m_hovered.first + m_hovered.second) * TILE_HEIGHT / 2;
        dc.SetPen(wxColor(255, 255, 128));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawPolygon(lines.size(), lines.data(), x, y);
    }

    // Draw cursor for the copy source tile
    if (m_cpysrc.first != -1)
    {
        std::unique_ptr<wxPen> m_cpysrc_pen(new wxPen(*wxCYAN, 1, wxPENSTYLE_SHORT_DASH));
        int x = (m_map->GetHeightmapHeight() + m_cpysrc.first - m_cpysrc.second - 1) * TILE_WIDTH / 2;
        int y = (m_cpysrc.first + m_cpysrc.second) * TILE_HEIGHT / 2;
        dc.SetPen(*m_cpysrc_pen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawPolygon(lines.size(), lines.data(), x, y);
    }
}


void HeightmapEditorCtrl::SetOpacity(wxImage& image, uint8_t opacity)
{
    uint8_t* alpha = image.GetAlpha();
    for (int i = 0; i < (image.GetHeight() * image.GetWidth()); i++)
    {
        *alpha = (*alpha < opacity) ? *alpha : opacity;
        alpha++;
    }
}

bool HeightmapEditorCtrl::IsCoordValid(std::pair<int, int> c)
{
    return (m_map != nullptr &&
            (c.first >= 0 && c.first < m_map->GetHeightmapWidth() &&
             c.second >= 0 && c.second < m_map->GetHeightmapHeight()));
}

bool HeightmapEditorCtrl::UpdateHoveredPosition(int screenx, int screeny)
{
    auto i = GetHMPosition(screenx, screeny);
    if (m_hovered.first != i.first || m_hovered.second != i.second)
    {
        m_hovered = i;
        return true;
    }
    return false;
}

bool HeightmapEditorCtrl::UpdateSelectedPosition(int screenx, int screeny)
{
    auto i = GetHMPosition(screenx, screeny);
    if (m_selected[0].first != i.first || m_selected[0].second != i.second)
    {
        m_selected[0] = i;
        FireEvent(EVT_HEIGHTMAP_CELL_SELECTED);
        return true;
    }
    return false;
}

std::vector<wxPoint> HeightmapEditorCtrl::GetTilePoly(float x, float y, float cols, float rows, int s, int width, int height) const
{
    return {
        wxPoint(x     + (width / 2)                     , y + s                                ),
        wxPoint(x - s + (width / 2) * (cols + 1)        , y     + (height / 2) * (cols)        ),
        wxPoint(x     + (width / 2) * (cols - rows + 1) , y - s + (height / 2) * (cols + rows) ),
        wxPoint(x + s - (width / 2) * (rows - 1)        , y     + (height / 2) * (rows)        ),
        wxPoint(x     + (width / 2)                     , y + s                                )
    };
}

wxColor HeightmapEditorCtrl::GetCellBackground(uint8_t restrictions, uint8_t type, uint8_t zz)
{
    wxColor bg = *wxBLACK;
    if (!IsCellHidden(restrictions, type, zz))
    {
        switch (restrictions)
        {
        case 0x0:
            bg.Set(zz * 3, 48 + zz * 12, zz * 3);
            break;
        case 0x4:
            bg.Set(48 + zz * 12, zz * 3, zz * 3);
            break;
        case 0x2:
            bg.Set(32 + zz * 3, 32 + zz * 3, 48 + zz * 12);
            break;
        case 0x6:
            bg.Set(48 + zz * 12, 32 + zz * 3, 48 + zz * 12);
            break;
        default:
            bg.Set(48 + zz * 12, 48 + zz * 12, zz * 3);
            break;
        }
    }
    return bg;
}

wxColor HeightmapEditorCtrl::GetCellBorder(uint8_t restrictions, uint8_t type, uint8_t z)
{
    return IsCellHidden(restrictions, type, z) ? *wxLIGHT_GREY : *wxWHITE;
}

void HeightmapEditorCtrl::ForceRepaint()
{
    m_repaint = true;
    Refresh(true);
}

void HeightmapEditorCtrl::ForceRedraw()
{
    m_redraw = true;
    m_repaint = true;
    Refresh(true);
}

void HeightmapEditorCtrl::RecreateBuffer()
{
    m_width = (m_map->GetHeightmapWidth() + m_map->GetHeightmapHeight()) * TILE_WIDTH / 2 + 1;
    m_height = (m_map->GetHeightmapWidth() + m_map->GetHeightmapHeight()) * TILE_HEIGHT / 2 + 1;
    m_width *= m_zoom;
    m_height *= m_zoom;
    if (!IsCoordValid(m_hovered))
    {
        m_hovered = { -1, -1 };
    }
    //if (!IsCoordValid(m_selected[0]))
    //{
    //    m_selected[0] = { -1, -1 };
    //}
    m_cpysrc = {-1, -1};
    RefreshGraphics();
}

void HeightmapEditorCtrl::UpdateScroll()
{
    m_scroll_rate = std::ceil(SCROLL_RATE * m_zoom);
    SetVirtualSize(std::ceil(m_zoom * m_width), std::ceil(m_zoom * m_height));
    SetScrollRate(m_scroll_rate, m_scroll_rate);
    AdjustScrollbars();
}

bool HeightmapEditorCtrl::Pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y)
{
    std::size_t i, j;
    bool c = false;
    for (i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].m_y > y) != (poly[j].m_y > y)) &&
            (x < (poly[j].m_x - poly[i].m_x) * (y - poly[i].m_y) / (poly[j].m_y - poly[i].m_y) + poly[i].m_x))
            c = !c;
    }
    return c;
}

void HeightmapEditorCtrl::GoToRoom(uint16_t room)
{
    const auto& name = m_g->GetRoomData()->GetRoom(room)->name;
    FireEvent(EVT_GO_TO_NAV_ITEM, "Rooms/" + name);
}

bool HeightmapEditorCtrl::IsCellHidden(uint8_t restrictions, uint8_t type, uint8_t z)
{
    return (restrictions == 0x4 && type == 0 && z == 0);
}

void HeightmapEditorCtrl::FireUpdateStatusEvent(const std::string& data, int pane)
{
    wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
    evt.SetString(data);
    evt.SetInt(pane);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void HeightmapEditorCtrl::FireEvent(const wxEventType& e, int userdata)
{
    wxCommandEvent evt(e);
    evt.SetInt(userdata);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void HeightmapEditorCtrl::FireEventLong(const wxEventType& e, long userdata)
{
    wxCommandEvent evt(e);
    evt.SetExtraLong(userdata);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void HeightmapEditorCtrl::FireEvent(const wxEventType& e, const std::string& userdata)
{
    wxCommandEvent evt(e);
    evt.SetString(userdata);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void HeightmapEditorCtrl::FireEvent(const wxEventType& e)
{
    wxCommandEvent evt(e);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void HeightmapEditorCtrl::OnDraw(wxDC& dc)
{
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    dc.SetUserScale(m_zoom, m_zoom);
    DrawRoomHeightmapBackground(dc);
    DrawEntities(dc);
    DrawRoomHeightmapForeground(dc);
    DrawWarps(dc);
    DrawDoors(dc);
    DrawTileSwaps(dc);
    DrawSelectionCursors(dc);
}

void HeightmapEditorCtrl::OnPaint(wxPaintEvent& /*evt*/)
{
    wxBufferedPaintDC dc(this);
    this->PrepareDC(dc);
    this->OnDraw(dc);
}

void HeightmapEditorCtrl::OnEraseBackground(wxEraseEvent& /*evt*/)
{
}

void HeightmapEditorCtrl::OnSize(wxSizeEvent& evt)
{
    Refresh(false);
    evt.Skip();
}

void HeightmapEditorCtrl::OnMouseMove(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::MOVE, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEAVE, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnLeftDown(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEFT_DOWN, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnLeftDClick(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEFT_DCLICK, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnRightDown(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::RIGHT_DOWN, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnLeftUp(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEFT_UP, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnRightUp(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::RIGHT_UP, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnRightDClick(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::RIGHT_DCLICK, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void HeightmapEditorCtrl::OnShow(wxShowEvent& evt)
{
    RefreshStatusbar();
    evt.Skip();
}

std::pair<int, int> HeightmapEditorCtrl::GetAbsoluteCoordinates(int screenx, int screeny)
{
    int x, y;
    GetViewStart(&x, &y);
    x *= m_scroll_rate;
    y *= m_scroll_rate;
    x += screenx;
    y += screeny;
    x = std::ceil(x / m_zoom);
    y = std::ceil(y / m_zoom);
    return { x, y };
}

std::pair<int, int> HeightmapEditorCtrl::GetHMPosition(int screenx, int screeny)
{
    auto xy = GetAbsoluteCoordinates(screenx, screeny);
    const int W = static_cast<int>(TILE_WIDTH) / 2;
    const int H = static_cast<int>(TILE_HEIGHT) / 2;

    int ix = xy.first * H + xy.second * W - (m_map->GetHeightmapHeight() - 2) * W * H;
    ix /= 2 * W * H;
    --ix;
    int iy = -xy.first * H + xy.second * W + (m_map->GetHeightmapHeight() + 2) * W * H;
    iy /= 2 * W * H;
    --iy;

    if (ix < 0 || ix >= m_map->GetHeightmapWidth() || iy < 0 || iy >= m_map->GetHeightmapHeight())
    {
        ix = -1;
        iy = -1;
    }
    return { ix, iy };
}

void HeightmapEditorCtrl::OnLeftClick(wxMouseEvent& evt)
{
    UpdateSelectedPosition(evt.GetX(), evt.GetY());
    if (m_cpysrc.first != -1 && m_selected[0].first != -1 && m_selected[0] != m_cpysrc)
    {
        m_map->SetHeight({ m_selected[0].first, m_selected[0].second}, m_map->GetHeight({ m_cpysrc.first, m_cpysrc.second}));
        m_map->SetCellProps({ m_selected[0].first, m_selected[0].second }, m_map->GetCellProps({ m_cpysrc.first, m_cpysrc.second }));
        m_map->SetCellType({ m_selected[0].first, m_selected[0].second }, m_map->GetCellType({ m_cpysrc.first, m_cpysrc.second }));
        ForceRedraw();
        FireEvent(EVT_HEIGHTMAP_UPDATE);
    }
}

void HeightmapEditorCtrl::OnLeftDblClick(wxMouseEvent& evt)
{
    evt.Skip();
}

void HeightmapEditorCtrl::OnRightClick(wxMouseEvent& evt)
{
    if (UpdateSelectedPosition(evt.GetX(), evt.GetY()))
    {
        m_cpysrc = m_selected[0];
        Refresh(false);
    }
    
    evt.Skip();
}

void HeightmapEditorCtrl::OnRightDblClick(wxMouseEvent& evt)
{
    evt.Skip();
}

int HeightmapEditorCtrl::GetFirstDoorRegion(const Coord& c)
{
    auto& [x, y] = c;
    for (std::size_t i = 0; i < m_doors.size(); ++i)
    {
        const Door& door = m_doors.at(i);
        if (door.x == x && door.y == y)
        {
            return i;
        }
    }
    return -1;
}

std::pair<int, bool> HeightmapEditorCtrl::GetFirstSwapRegion(const Coord& c)
{
    const auto& [x, y] = c;
    for (std::size_t i = 0; i < m_swaps.size(); ++i)
    {
        const TileSwap& swap = m_swaps.at(i);
        if ((x >= swap.heightmap.src_x && x < swap.heightmap.src_x + swap.heightmap.width) &&
            (y >= swap.heightmap.src_y && y < swap.heightmap.src_y + swap.heightmap.height))
        {
            return { i, true };
        }
        else if ((x >= swap.heightmap.dst_x && x < swap.heightmap.dst_x + swap.heightmap.width) &&
                 (y >= swap.heightmap.dst_y && y < swap.heightmap.dst_y + swap.heightmap.height))
        {
            return { i, false };
        }
    }
    return { -1, false };
}

std::tuple<uint8_t, uint8_t, uint8_t> HeightmapEditorCtrl::GetHeightmapCell(int x, int y)
{
    TileSwap* swap = nullptr;
    if (m_preview_swap && IsSwapSelected())
    {
        swap = &m_swaps.at(GetSelectedSwap() - 1);
    }
    if (swap && swap->IsHeightmapPointInSwap(x, y))
    {
        HMPoint2D point = { x - swap->heightmap.dst_x + swap->heightmap.src_x,
                            y - swap->heightmap.dst_y + swap->heightmap.src_y };
        auto restrictions = m_map->GetCellProps(point);
        auto z = m_map->GetHeight(point);
        auto type = m_map->GetCellType(point);
        return { restrictions, z, type };
    }
    else
    {
        auto restrictions = m_map->GetCellProps({ x, y });
        auto z = m_map->GetHeight({ x, y });
        auto type = m_map->GetCellType({ x, y });
        return { restrictions, z, type };
    }
}

uint8_t HeightmapEditorCtrl::GetHeightmapCellType(int x, int y)
{
    TileSwap* swap = nullptr;
    if (m_preview_swap && IsSwapSelected())
    {
        swap = &m_swaps.at(GetSelectedSwap() - 1);
    }
    if (swap && swap->IsHeightmapPointInSwap(x, y))
    {
        HMPoint2D point = { x - swap->heightmap.dst_x + swap->heightmap.src_x,
                            y - swap->heightmap.dst_y + swap->heightmap.src_y };
        return m_map->GetCellType(point);
    }
    else
    {
        return m_map->GetCellType({ x, y });
    }
}

uint8_t HeightmapEditorCtrl::GetHeightmapCellZ(int x, int y)
{
    TileSwap* swap = nullptr;
    if (m_preview_swap && IsSwapSelected())
    {
        swap = &m_swaps.at(GetSelectedSwap() - 1);
    }
    if (swap && swap->IsHeightmapPointInSwap(x, y))
    {
        HMPoint2D point = { x - swap->heightmap.dst_x + swap->heightmap.src_x,
                            y - swap->heightmap.dst_y + swap->heightmap.src_y };
        return m_map->GetHeight(point);
    }
    else
    {
        return m_map->GetHeight({ x, y });
    }
}

uint8_t HeightmapEditorCtrl::GetHeightmapCellRestrictions(int x, int y)
{
    TileSwap* swap = nullptr;
    if (m_preview_swap && IsSwapSelected())
    {
        swap = &m_swaps.at(GetSelectedSwap() - 1);
    }
    if (swap && swap->IsHeightmapPointInSwap(x, y))
    {
        HMPoint2D point = { x - swap->heightmap.dst_x + swap->heightmap.src_x,
                            y - swap->heightmap.dst_y + swap->heightmap.src_y };
        return m_map->GetCellProps(point);
    }
    else
    {
        return m_map->GetCellProps({ x, y });
    }
}

void HeightmapEditorCtrl::StartDrag()
{
    if (!IsSwapSelected() || m_dragging)
    {
        return;
    }
    m_dragged = m_hovered;
    m_dragging = true;
    CaptureMouse();
    if (m_selected_is_src)
    {
        m_dragged_orig_pos.first = m_swaps[GetSelectedSwap() - 1].map.src_x;
        m_dragged_orig_pos.second = m_swaps[GetSelectedSwap() - 1].map.src_y;
    }
    else
    {
        m_dragged_orig_pos.first = m_swaps[GetSelectedSwap() - 1].map.dst_x;
        m_dragged_orig_pos.second = m_swaps[GetSelectedSwap() - 1].map.dst_y;
    }
}

void HeightmapEditorCtrl::StopDrag(bool cancel)
{
    if (!m_dragging)
    {
        return;
    }
    ReleaseMouse();
    m_dragged = std::make_pair(0, 0);
    m_dragging = false;
    if (cancel)
    {
        if (IsSwapSelected())
        {
            if (m_selected_is_src)
            {
                m_swaps[GetSelectedSwap() - 1].map.src_x = m_dragged_orig_pos.first;
                m_swaps[GetSelectedSwap() - 1].map.src_y = m_dragged_orig_pos.second;
            }
            else
            {
                m_swaps[GetSelectedSwap() - 1].map.dst_x = m_dragged_orig_pos.first;
                m_swaps[GetSelectedSwap() - 1].map.dst_y = m_dragged_orig_pos.second;
            }
        }
        Refresh();
        if (m_g)
        {
            m_g->GetRoomData()->SetTileSwaps(m_roomnum, m_swaps);
            FireEvent(EVT_TILESWAP_UPDATE, GetSelectedSwap());
        }
    }
}

void HeightmapEditorCtrl::RefreshDrag()
{
    if (!m_dragging || !IsHoverValid())
    {
        return;
    }
    Coord relmv = { m_hovered.first - m_dragged.first, m_hovered.second - m_dragged.second };
    if (IsSwapSelected())
    {
        if (m_selected_is_src)
        {
            if (m_swaps[GetSelectedSwap() - 1].map.src_x != m_dragged_orig_pos.first + relmv.first ||
                m_swaps[GetSelectedSwap() - 1].map.src_y != m_dragged_orig_pos.second + relmv.second)
            {
                m_swaps[GetSelectedSwap() - 1].map.src_x = m_dragged_orig_pos.first + relmv.first;
                m_swaps[GetSelectedSwap() - 1].map.src_y = m_dragged_orig_pos.second + relmv.second;
            }
        }
        else
        {
            if (m_swaps[GetSelectedSwap() - 1].map.dst_x != m_dragged_orig_pos.first + relmv.first ||
                m_swaps[GetSelectedSwap() - 1].map.dst_y != m_dragged_orig_pos.second + relmv.second)
            {
                m_swaps[GetSelectedSwap() - 1].map.dst_x = m_dragged_orig_pos.first + relmv.first;
                m_swaps[GetSelectedSwap() - 1].map.dst_y = m_dragged_orig_pos.second + relmv.second;
            }
        }
        if (m_g)
        {
            m_g->GetRoomData()->SetTileSwaps(m_roomnum, m_swaps);
            FireEvent(EVT_TILESWAP_UPDATE, GetSelectedSwap());
        }
    }
}
