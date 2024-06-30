#include <Map3DEditor.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <RoomViewerFrame.h>
#include <ImageBuffer.h>

wxDEFINE_EVENT(EVT_MAPLAYER_UPDATE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(Map3DEditor, wxScrolledCanvas)
EVT_ERASE_BACKGROUND(Map3DEditor::OnEraseBackground)
EVT_PAINT(Map3DEditor::OnPaint)
EVT_SIZE(Map3DEditor::OnSize)
EVT_MOTION(Map3DEditor::OnMouseMove)
EVT_LEAVE_WINDOW(Map3DEditor::OnMouseLeave)
EVT_LEFT_DOWN(Map3DEditor::OnLeftDown)
EVT_RIGHT_DOWN(Map3DEditor::OnRightDown)
EVT_LEFT_UP(Map3DEditor::OnLeftUp)
EVT_RIGHT_UP(Map3DEditor::OnRightUp)
EVT_LEFT_DCLICK(Map3DEditor::OnLeftDClick)
EVT_RIGHT_DCLICK(Map3DEditor::OnRightDClick)
EVT_SHOW(Map3DEditor::OnShow)
wxEND_EVENT_TABLE()

Map3DEditor::Map3DEditor(wxWindow* parent, RoomViewerFrame* frame, Tilemap3D::Layer layer)
    : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
      m_g(nullptr),
      m_map(nullptr),
      m_map_disp(nullptr),
      m_layer_buf(std::make_unique<ImageBuffer>()),
      m_bg_buf(std::make_unique<ImageBuffer>()),
      m_tileset(nullptr),
      m_pal(nullptr),
      m_blockset(nullptr),
      m_frame(frame),
      m_roomnum(0),
      m_layer(layer),
      m_width(1),
      m_height(1),
      m_redraw(false),
      m_repaint(false),
      m_zoom(1.0),
      m_selected_block(0),
      m_selected(-1, -1),
      m_hovered(-1, -1),
      m_dragged(-1, -1),
      m_dragged_orig_pos(-1, -1),
      m_dragging(false),
      m_selected_region(-1),
      m_selected_is_src(false),
      m_preview_swap(false),
      m_bmp(std::make_unique<wxBitmap>()),
      m_show_blocknums(false),
      m_show_borders(true),
      m_show_priority(true),
      m_scroll_rate(SCROLL_RATE),
      m_cursorid(wxCURSOR_ARROW)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
    m_priority_pen = std::make_unique<wxPen>(*wxCYAN, 1, wxPENSTYLE_SHORT_DASH);
}

Map3DEditor::~Map3DEditor()
{
}

void Map3DEditor::SetGameData(std::shared_ptr<GameData> gd)
{
    m_g = gd;
    Refresh(true);
}

void Map3DEditor::ClearGameData()
{
    m_g = nullptr;
    Refresh(true);
}

void Map3DEditor::SetRoomNum(uint16_t roomnum)
{
    if (m_g)
    {
        m_map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
        m_map_disp = std::make_shared<Tilemap3D>(*m_map);
        m_pal = m_g->GetRoomData()->GetPaletteForRoom(roomnum)->GetData();
        m_tileset = m_g->GetRoomData()->GetTilesetForRoom(roomnum)->GetData();
        m_blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);
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
        m_preview_swap = false;
        UpdateSwaps();
        UpdateDoors();
        RecreateBuffer();
    }
}

void Map3DEditor::SetSelectedBlock(int block)
{
    if (m_blockset != nullptr && block >= 0 && block < static_cast<int>(m_blockset->size()))
    {
        m_selected_block = block;
    }
    else
    {
        m_selected_block = -1;
    }
}

int Map3DEditor::GetSelectedBlock() const
{
    return m_selected_block;
}

bool Map3DEditor::IsBlockSelected() const
{
    return (m_blockset != nullptr && m_selected_block >= 0 && m_selected_block < static_cast<int>(m_blockset->size()));
}

void Map3DEditor::SetHovered(Coord sel)
{
    if (IsCoordValid(sel) && m_hovered != sel)
    {
        m_hovered = sel;
        Refresh();
    }
}

Map3DEditor::Coord Map3DEditor::GetHovered() const
{
    return m_hovered;
}

bool Map3DEditor::IsHoverValid() const
{
    return IsCoordValid(m_hovered);
}

void Map3DEditor::SetSelectedSwap(int swap)
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
        if (m_preview_swap)
        {
            if (!IsSwapSelected())
            {
                m_preview_swap = false;
                Refresh();
            }
            else
            {
                ForceRedraw();
            }
        }
        else
        {
            Refresh();
        }
        RefreshStatusbar();
    }
}

int Map3DEditor::GetSelectedSwap() const
{
    return IsSwapSelected() ? m_selected_region + 1 : -1;
}

bool Map3DEditor::IsSwapSelected() const
{
    return (m_selected_region >= 0 && m_selected_region < static_cast<int>(m_swaps.size()));
}

void Map3DEditor::SetSelectedDoor(int door)
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
        if (m_preview_swap)
        {
            if (!IsDoorSelected())
            {
                m_preview_swap = false;
                Refresh();
            }
            else
            {
                ForceRedraw();
            }
        }
        else
        {
            Refresh();
        }
        RefreshStatusbar();
    }
}

int Map3DEditor::GetSelectedDoor() const
{
    return IsDoorSelected() ? m_selected_region - 0xFF : -1;
}

bool Map3DEditor::IsDoorSelected() const
{
    return (m_selected_region >= 0x100 && m_selected_region < static_cast<int>(0x100 + m_doors.size()));
}

const std::vector<TileSwap>& Map3DEditor::GetTileswaps() const
{
    return m_swaps;
}

int Map3DEditor::GetTotalTileswaps() const
{
    return m_swaps.size();
}

void Map3DEditor::AddTileswap()
{
    m_swaps.push_back(TileSwap());
    SetSelectedSwap(m_swaps.size() - 1);
    Refresh();
}

void Map3DEditor::DeleteTileswap()
{
    if (m_swaps.size() > 0 && IsSwapSelected())
    {
        auto prev = GetSelectedSwap();
        m_swaps.erase(m_swaps.cbegin() + prev);
        SetSelectedSwap(prev);
        Refresh();
    }
}

const std::vector<Door>& Map3DEditor::GetDoors() const
{
    return m_doors;
}

int Map3DEditor::GetTotalDoors() const
{
    return m_doors.size();
}

void Map3DEditor::AddDoor()
{
    m_doors.push_back(Door());
    SetSelectedDoor(m_doors.size() - 1);
    Refresh();
}

void Map3DEditor::DeleteSelectedDoor()
{
    if (m_doors.size() > 0 && IsDoorSelected())
    {
        auto prev = GetSelectedDoor();
        m_doors.erase(m_doors.cbegin() + prev);
        SetSelectedDoor(prev);
        Refresh();
    }
}

void Map3DEditor::RefreshGraphics()
{
    UpdateScroll();
    ForceRedraw();
}

void Map3DEditor::UpdateSwaps()
{
    if (m_g)
    {
        auto old_swap_count = m_swaps.size();
        m_swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
        m_swap_regions.clear();
        if (m_swaps.size() != old_swap_count)
        {
            ResetPreview();
        }
    }
}

void Map3DEditor::UpdateDoors()
{
    if (m_g)
    {
        auto old_door_count = m_doors.size();
        m_doors = m_g->GetRoomData()->GetDoors(m_roomnum);
        m_door_regions.clear();
        if (m_doors.size() != old_door_count)
        {
            ResetPreview();
        }
    }
}

bool Map3DEditor::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
    RefreshCursor((modifiers & wxMOD_CONTROL) > 0);
    if (key == WXK_ESCAPE)
    {
        StopDrag(true);
        m_selected = { -1, -1 };
        m_hovered = { -1, -1 };
        SetSelectedSwap(-1);
        SetSelectedDoor(-1);
        FireEvent(EVT_TILESWAP_SELECT, -1);
        FireEvent(EVT_DOOR_SELECT, -1);
        return true;
    }
    if (!HandleRegionKeyDown(key, modifiers))
    {
        return HandleDrawKeyDown(key, modifiers);
    }
    return false;
}

bool Map3DEditor::HandleDrawKeyDown(unsigned int key, unsigned int modifiers)
{
    switch (key)
    {
    case WXK_LEFT:
    case 'a':
    case 'A':
        if (IsHoverValid())
        {
            SetHovered({ m_hovered.first - 1, m_hovered.second });
        }
        else
        {
            SetHovered({ 0,0 });
        }
        return true;
    case WXK_RIGHT:
    case 'd':
    case 'D':
        if (IsHoverValid())
        {
            SetHovered({ m_hovered.first + 1, m_hovered.second });
        }
        else
        {
            SetHovered({ 0,0 });
        }
        return true;
    case WXK_UP:
    case 'w':
    case 'W':
        if (IsHoverValid())
        {
            SetHovered({ m_hovered.first, m_hovered.second - 1 });
        }
        else
        {
            SetHovered({ 0,0 });
        }
        return true;
    case WXK_DOWN:
    case 's':
    case 'S':
        if (IsHoverValid())
        {
            SetHovered({ m_hovered.first, m_hovered.second + 1 });
        }
        else
        {
            SetHovered({ 0,0 });
        }
        return true;
    case WXK_SPACE:
        SetHoveredTile();
        return true;
    case 'c':
    case 'C':
        SelectHoveredTile();
        return true;
    }
    return false;
}

bool Map3DEditor::HandleRegionKeyDown(unsigned int key, unsigned int modifiers)
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
                if ((position < 0)  || (position >= static_cast<int>(0x100 + m_doors.size())))
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
                else if(IsSwapSelected())
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
                DECR(m_swaps[GetSelectedSwap() - 1].map.dst_x);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECSZ(m_swaps[GetSelectedSwap() - 1].map.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedSwap() - 1].map.src_x);
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
                INCR(m_swaps[GetSelectedSwap() - 1].map.dst_x);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCSZ(m_swaps[GetSelectedSwap() - 1].map.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedSwap() - 1].map.src_x);
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
                DECR(m_swaps[GetSelectedSwap() - 1].map.dst_y);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECSZ(m_swaps[GetSelectedSwap() - 1].map.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedSwap() - 1].map.src_y);
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
                INCR(m_swaps[GetSelectedSwap() - 1].map.dst_y);
                upd = true;
            }
            else if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                INCSZ(m_swaps[GetSelectedSwap() - 1].map.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedSwap() - 1].map.src_y);
                upd = true;
            }
        }
        break;
    case 'T':
    case 't':
        if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))
            {
                DECDSZ(m_swaps[GetSelectedSwap() - 1].mode, 3);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCDSZ(m_swaps[GetSelectedSwap() - 1].mode, 3);
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
        if (!m_preview_swap && (IsDoorSelected() || IsSwapSelected()))
        {
            m_preview_swap = true;
            GeneratePreview();
        }
        else if (m_preview_swap)
        {
            m_preview_swap = false;
            ResetPreview();
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
            if (m_preview_swap)
            {
                ForceRedraw();
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

bool Map3DEditor::HandleKeyUp(unsigned int key, unsigned int modifiers)
{
    switch (key)
    {
    case 'P':
    case 'p':
        if (m_preview_swap)
        {
            m_preview_swap = false;
            for (int y = 0; y < m_map_disp->GetHeight(); ++y)
            {
                for (int x = 0; x < m_map_disp->GetWidth(); ++x)
                {
                    m_map_disp->SetBlock({ m_map->GetBlock({x, y}, m_layer), {x, y}}, m_layer);
                }
            }
            Refresh();
        }
        break;
    }
    return false;
}

bool Map3DEditor::HandleMouse(MouseEventType type, bool left_down, bool right_down, unsigned int modifiers, int x, int y)
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

bool Map3DEditor::HandleLeftDown(unsigned int modifiers)
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
    else if (IsBlockSelected() && m_hovered.first != -1)
    {
        SetHoveredTile();
    }
    return false;
}

bool Map3DEditor::HandleLeftDClick(unsigned int modifiers)
{
    auto sw = GetFirstSwapRegion(m_hovered);
    auto dw = GetFirstDoorRegion(m_hovered);
    if (sw.first != -1)
    {
        FireEvent(EVT_TILESWAP_OPEN_PROPERTIES, sw.first + 1);
    }
    else if (dw != -1)
    {
        FireEvent(EVT_DOOR_OPEN_PROPERTIES, dw + 1);
    }
    return false;
}

bool Map3DEditor::HandleRightDown(unsigned int modifiers)
{
    if ((modifiers & wxMOD_CONTROL) == 0)
    {
        SelectHoveredTile();
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
        if (!m_preview_swap && (IsSwapSelected() || IsDoorSelected()))
        {
            m_preview_swap = true;
            GeneratePreview();
        }
        else if (m_preview_swap)
        {
            m_preview_swap = false;
            ResetPreview();
        }
        RefreshStatusbar();
        ForceRedraw();
    }
    return false;
}

void Map3DEditor::SetHoveredTile()
{
    if (m_hovered.first != -1)
    {
        m_map->SetBlock({ static_cast<uint16_t>(m_selected_block), {m_hovered.first, m_hovered.second} }, m_layer);
        m_map_disp->SetBlock({ static_cast<uint16_t>(m_selected_block), {m_hovered.first, m_hovered.second} }, m_layer);
        FireMapEvent(EVT_MAPLAYER_UPDATE);
        ForceRedraw();
    }
}

void Map3DEditor::SelectHoveredTile()
{
    if (m_hovered.first != -1)
    {
        m_selected_block = m_map->GetBlock({ m_hovered.first, m_hovered.second }, m_layer);
        FireEvent(EVT_BLOCK_SELECT, m_selected_block);
        RefreshStatusbar();
    }
}

void Map3DEditor::RefreshStatusbar()
{
    std::string hovmsg = "";
    std::string selmsg = "";
    std::string swpmsg = "";
    if (m_hovered.first != -1)
    {
        uint16_t i = 0;
        if (m_map_disp != nullptr)
        {
            i = m_map_disp->GetBlock({ m_hovered.first, m_hovered.second }, m_layer);
        }
        hovmsg = StrPrintf("Cursor (%04d, %04d) : Block %d", m_hovered.first, m_hovered.second, i);
    }
    if (IsBlockSelected())
    {
        selmsg = StrPrintf("Selected Block %d", m_selected_block);
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
    FireUpdateStatusEvent(hovmsg, 0);
    FireUpdateStatusEvent(selmsg, 1);
    FireUpdateStatusEvent(swpmsg, 2);
}

void Map3DEditor::ForceRedraw()
{
    m_redraw = true;
    m_repaint = true;
    Refresh(true);
}

void Map3DEditor::RecreateBuffer()
{
    m_width = (m_map->GetWidth() + m_map->GetHeight()) * TILE_WIDTH + 1;
    m_height = (m_map->GetWidth() + m_map->GetHeight()) * TILE_HEIGHT / 2 + 1;
    m_bg_buf->Resize(m_width, m_height);
    m_layer_buf->Resize(m_width, m_height);
    m_width *= m_zoom;
    m_height *= m_zoom;
    if (!IsCoordValid(m_hovered))
    {
        m_hovered = { -1, -1 };
    }
    if (!IsCoordValid(m_selected))
    {
        m_selected = { -1, -1 };
    }
    RefreshGraphics();
}

void Map3DEditor::UpdateScroll()
{
    m_scroll_rate = std::ceil(SCROLL_RATE * m_zoom);
    SetVirtualSize(std::ceil(m_zoom * m_width), std::ceil(m_zoom * m_height));
    SetScrollRate(m_scroll_rate, m_scroll_rate);
    AdjustScrollbars();
}

void Map3DEditor::DrawMap(wxDC& dc)
{
    dc.SetUserScale(m_zoom, m_zoom);
    if (m_map_disp)
    {
        dc.SetPen(wxColor(128,128,128));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        for (int y = 0; y < m_map_disp->GetHeight(); ++y)
        {
            for (int x = 0; x < m_map_disp->GetWidth(); ++x)
            {
                int xp = (m_map_disp->GetHeight() - 1 + x - y) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::BG ? TILE_WIDTH : 0);
                int yp = (x + y) * TILE_HEIGHT / 2;
                if (m_show_borders)
                {
                    dc.DrawRectangle(xp, yp, TILE_WIDTH + 1, TILE_HEIGHT + 1);
                }
                uint16_t blk = m_map_disp->GetBlock({ x,y }, m_layer);
                if (m_show_priority)
                {
                    bool pri = false;
                    for (std::size_t i = 0; i < MapBlock::GetBlockSize(); ++i)
                    {
                        pri = pri || m_blockset->at(blk).GetTile(i).Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
                    }
                    if (pri)
                    {
                        dc.SetPen(*m_priority_pen);
                        dc.DrawRectangle({ xp + 1, yp + 1, TILE_WIDTH - 1, TILE_HEIGHT - 1 });
                        dc.SetPen(wxColor(128, 128, 128));
                    }
                }
                if (m_show_blocknums && blk > 0)
                {
                    wxString t = StrPrintf("%03X", blk);
                    auto extent = dc.GetTextExtent(t);
                    dc.SetTextForeground(*wxWHITE);
                    int tx = xp + (TILE_WIDTH - extent.GetWidth()) / 2;
                    int ty = yp + (TILE_HEIGHT - extent.GetHeight()) / 2;
                    dc.DrawText(t, tx + 1, ty);
                    dc.DrawText(t, tx - 1, ty);
                    dc.DrawText(t, tx, ty + 1);
                    dc.DrawText(t, tx, ty - 1);
                    dc.SetTextForeground(*wxBLACK);
                    dc.DrawText(t, tx, ty);
                }
            }
        }
    }
}

void Map3DEditor::DrawTiles()
{
    if (m_redraw)
    {
        wxMemoryDC dc(*m_bmp);
        m_pal = m_g->GetRoomData()->GetPaletteForRoom(m_roomnum)->GetData();
        m_tileset = m_g->GetRoomData()->GetTilesetForRoom(m_roomnum)->GetData();
        m_blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(m_roomnum);
        m_layer_buf->Clear();
        m_layer_buf->Insert3DMapLayer(0, 0, 0, m_layer, m_map_disp, m_tileset, m_blockset, false, m_preview_swaps, m_preview_doors);
        if (m_layer == Tilemap3D::Layer::FG)
        {
            m_bg_buf->Clear();
            m_bg_buf->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, m_map_disp, m_tileset, m_blockset, false);
        }
        dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
        dc.Clear();
        dc.SetUserScale(m_zoom * 2.0, m_zoom * 2.0);
        if (m_layer == Tilemap3D::Layer::FG)
        {
            dc.DrawBitmap(*m_bg_buf->MakeBitmap({ m_pal }, true, 0x40, 0x40), 0, 0, false);
        }
        dc.DrawBitmap(*m_layer_buf->MakeBitmap({ m_pal }, true), 0, 0, true);
        m_redraw = false;
        dc.SelectObject(wxNullBitmap);
    }
}

void Map3DEditor::DrawTile(int /*tile*/)
{
}

void Map3DEditor::DrawCell(wxDC& dc, const std::pair<int, int>& pos, const wxPen& pen, const wxBrush& brush)
{
    auto p = GetScreenPosition(pos);
    dc.SetPen(pen);
    dc.SetBrush(brush);
    dc.DrawRectangle(p.first, p.second, TILE_WIDTH + 1, TILE_HEIGHT + 1);
}

void Map3DEditor::DrawTileSwaps(wxDC& dc)
{
    m_swap_regions.clear();
    int si = 0;
    for (const auto& s : m_swaps)
    {
        std::pair<int, int> src, dst;
        auto lines = s.GetMapRegionPoly(TileSwap::Region::UNDEFINED, TILE_WIDTH, TILE_HEIGHT);
        std::vector<wxPoint> wxpoints = ToWxPoints(lines);
        auto sp = GetScreenPosition(s.GetTileOffset(TileSwap::Region::SOURCE, m_map, m_layer));
        auto dp = GetScreenPosition(s.GetTileOffset(TileSwap::Region::DESTINATION, m_map, m_layer));
        auto [hx, hy] = GetScreenPosition(m_hovered);
        m_swap_regions.emplace_back(
                ToWxPoints(TileSwap::OffsetRegionPoly(lines, sp)),
                ToWxPoints(TileSwap::OffsetRegionPoly(lines, dp)));
        bool hovered = false;
        if (!m_dragging)
        {
            hovered = Pnpoly(m_swap_regions.back().first, hx, hy) || Pnpoly(m_swap_regions.back().second, hx, hy);
        }
        else
        {
            hovered = m_selected_region == si;
        }
        dc.SetPen(wxPen(hovered ? wxColor(128, 128, 255) : *wxBLUE, (m_selected_region == si) ? 3 : 1));
        dc.DrawPolygon(wxpoints.size(), wxpoints.data(), sp.first, sp.second);
        dc.SetPen(wxPen(hovered ? wxColor(255, 128, 128) : *wxRED, (m_selected_region == si) ? 3 : 1));
        dc.DrawPolygon(wxpoints.size(), wxpoints.data(), dp.first, dp.second);
        ++si;
    }
}

void Map3DEditor::DrawDoors(wxDC& dc)
{
    int di = 0x100;
    m_door_regions.clear();
    for (const auto& d : m_doors)
    {
        if (m_map_disp && d.x < m_map_disp->GetHeightmapWidth() && d.y < m_map_disp->GetHeightmapHeight())
        {
            auto lines = d.GetMapRegionPoly(m_map_disp, TILE_WIDTH, TILE_HEIGHT);
            auto wxpoints = ToWxPoints(lines);
            auto pos = GetScreenPosition(d.GetTileOffset(m_map_disp, m_layer));
            auto [hx, hy] = GetScreenPosition(m_hovered);
            m_door_regions.push_back(ToWxPoints(Door::OffsetRegionPoly(m_map_disp, lines, pos)));
            dc.SetPen(wxPen(Pnpoly(m_door_regions.back(), hx, hy) ? wxColor(255, 128, 255) : wxColor(255, 0, 255),
                di == m_selected_region ? 3 : 1));
            dc.DrawPolygon(wxpoints.size(), wxpoints.data(), pos.first, pos.second);
            ++di;
        }
    }
}

void Map3DEditor::OnDraw(wxDC& dc)
{
    if (m_redraw)
    {
        m_bmp->Create(m_width, m_height);
        DrawTiles();
        m_redraw = false;
    }
    int sx, sy;
    GetViewStart(&sx, &sy);
    sx *= m_scroll_rate;
    sy *= m_scroll_rate;
    int sw, sh;
    GetClientSize(&sw, &sh);
    wxMemoryDC mdc(*m_bmp);
    mdc.SetUserScale(1.0, 1.0);
    dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE)));
    dc.Clear();
    dc.Blit(sx, sy, sw, sh, &mdc, sx, sy, wxCOPY);
    dc.SetUserScale(m_zoom, m_zoom);
    mdc.SelectObject(wxNullBitmap);
    DrawMap(dc);
    DrawDoors(dc);
    DrawTileSwaps(dc);
    if (m_hovered.first != -1 && m_hovered != m_selected)
    {
        DrawCell(dc, m_hovered, *wxWHITE_PEN, *wxTRANSPARENT_BRUSH);
    }
    if (m_selected.first != -1 && m_hovered != m_selected)
    {
        DrawCell(dc, m_selected, *wxYELLOW_PEN, *wxWHITE_BRUSH);
    }
    if (m_selected.first != -1 && m_hovered == m_selected)
    {
        DrawCell(dc, m_selected, wxColor(255, 255, 128), *wxWHITE_BRUSH);
    }
}

void Map3DEditor::OnPaint(wxPaintEvent& /*evt*/)
{
    wxBufferedPaintDC dc(this);
    this->PrepareDC(dc);
    this->OnDraw(dc);
}

void Map3DEditor::OnEraseBackground(wxEraseEvent& /*evt*/)
{
}

void Map3DEditor::OnSize(wxSizeEvent& evt)
{
    Refresh(false);
    evt.Skip();
}

void Map3DEditor::OnMouseMove(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::MOVE, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnMouseLeave(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEAVE, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnLeftDown(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEFT_DOWN, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnLeftDClick(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEFT_DCLICK, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnRightDown(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::RIGHT_DOWN, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnLeftUp(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::LEFT_UP, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnRightUp(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::RIGHT_UP, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnRightDClick(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(MouseEventType::RIGHT_DCLICK, evt.LeftIsDown(), evt.RightIsDown(), evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnShow(wxShowEvent& evt)
{
    RefreshStatusbar();
    evt.Skip();
}

std::vector<wxPoint> Map3DEditor::ToWxPoints(const std::vector<std::pair<int, int>>& points)
{
    std::vector<wxPoint> wxpoints;
    std::transform(points.cbegin(), points.cend(),
        std::back_inserter(wxpoints),
        [](const auto& p) {return wxPoint(p.first, p.second); });
    return wxpoints;
}

bool Map3DEditor::Pnpoly(const std::vector<wxPoint>& poly, int x, int y)
{
    std::size_t i, j;
    bool c = false;
    for (i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].y > y) != (poly[j].y > y)) &&
            (x < (poly[j].x - poly[i].x) * (y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
            c = !c;
    }
    return c;
}

Map3DEditor::Coord Map3DEditor::GetAbsoluteCoordinates(int screenx, int screeny) const
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

Map3DEditor::Coord Map3DEditor::GetCellPosition(int screenx, int screeny) const
{
    auto xy = GetAbsoluteCoordinates(screenx, screeny);
    int ix = 0, iy = 0;
    int offsetx = (m_map->GetHeight() - 1) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::BG ? TILE_WIDTH : 0);
    int relx = xy.first - offsetx;
    int column = relx / static_cast<int>(TILE_WIDTH) - (relx < 0 ? 1 : 0);

    int row = 0;
    if (column % 2 == 0)
    {
        row = xy.second / TILE_HEIGHT;
    }
    else
    {
        row = (xy.second - TILE_HEIGHT / 2) / TILE_HEIGHT;
        column++;
        iy = 1;
    }

    ix += row + column / 2;
    iy += row - column / 2;

    if (IsCoordValid({ ix, iy }))
    {
        return { ix, iy };
    }
    else
    {
        return { -1, -1 };
    }
}

Map3DEditor::Coord Map3DEditor::GetScreenPosition(const Map3DEditor::Coord& pos) const
{
    int xp = (m_map->GetHeight() - 1 + pos.first - pos.second) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::BG ? TILE_WIDTH : 0);
    int yp = (pos.first + pos.second) * TILE_HEIGHT / 2;
    return { xp, yp };
}

bool Map3DEditor::IsCoordValid(const Map3DEditor::Coord& c) const
{
    return (m_map != nullptr &&
        (c.first >= 0 && c.first < m_map->GetWidth() &&
            c.second >= 0 && c.second < m_map->GetHeight()));
}

bool Map3DEditor::UpdateHoveredPosition(int screenx, int screeny)
{
    auto i = GetCellPosition(screenx, screeny);
    if (m_hovered.first != i.first || m_hovered.second != i.second)
    {
        m_hovered = i;
        return true;
    }
    return false;
}

bool Map3DEditor::UpdateSelectedPosition(int screenx, int screeny)
{
    auto i = GetCellPosition(screenx, screeny);
    if (m_selected.first != i.first || m_selected.second != i.second)
    {
        m_selected = i;
        return true;
    }
    return false;
}

int Map3DEditor::GetFirstDoorRegion(const Coord& c)
{
    auto [x, y] = GetScreenPosition(c);
    for (std::size_t i = 0; i < m_door_regions.size(); ++i)
    {
        if (Pnpoly(m_door_regions[i], x, y) == true)
        {
            return i;
        }
    }
    return -1;
}

std::pair<int, bool> Map3DEditor::GetFirstSwapRegion(const Coord& c)
{
    auto [x, y] = GetScreenPosition(c);
    for (std::size_t i = 0; i < m_swap_regions.size(); ++i)
    {
        if (Pnpoly(m_swap_regions[i].first, x, y) == true)
        {
            return { i, true };
        }
        if (Pnpoly(m_swap_regions[i].second, x, y) == true)
        {
            return { i, false };
        }
    }
    return {-1, false};
}

void Map3DEditor::RefreshCursor(bool ctrl_down)
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

void Map3DEditor::UpdateCursor(wxStockCursor cursor)
{
    if (cursor != m_cursorid)
    {
        SetCursor(cursor);
        m_cursorid = cursor;
    }
}

void Map3DEditor::FireUpdateStatusEvent(const std::string& data, int pane)
{
    wxCommandEvent evt(EVT_STATUSBAR_UPDATE);
    evt.SetString(data);
    evt.SetInt(pane);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void Map3DEditor::FireEvent(const wxEventType& e, int userdata)
{
    wxCommandEvent evt(e);
    evt.SetInt(userdata);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void Map3DEditor::FireEvent(const wxEventType& e, const std::string& userdata)
{
    wxCommandEvent evt(e);
    evt.SetString(userdata);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void Map3DEditor::FireEvent(const wxEventType& e)
{
    wxCommandEvent evt(e);
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void Map3DEditor::FireMapEvent(const wxEventType& e)
{
    wxCommandEvent evt(e);
    evt.SetInt(static_cast<int>(m_layer));
    evt.SetClientData(m_frame);
    wxPostEvent(m_frame, evt);
}

void Map3DEditor::StartDrag()
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

void Map3DEditor::StopDrag(bool cancel)
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

void Map3DEditor::RefreshDrag()
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

void Map3DEditor::GeneratePreview()
{
    if (IsSwapSelected())
    {
        *m_map_disp = *m_map;
        if (m_layer == Tilemap3D::Layer::FG)
        {
            m_swaps.at(GetSelectedSwap() - 1).DrawSwap(*m_map_disp, Tilemap3D::Layer::FG);
        }
        m_swaps.at(GetSelectedSwap() - 1).DrawSwap(*m_map_disp, Tilemap3D::Layer::BG);
    }
    else if (IsDoorSelected())
    {
        *m_map_disp = *m_map;
        if (m_layer == Tilemap3D::Layer::FG)
        {
            m_doors.at(GetSelectedDoor() - 1).DrawDoor(*m_map_disp, Tilemap3D::Layer::FG);
        }
    }
    else
    {
        ResetPreview();
    }
}

void Map3DEditor::ResetPreview()
{
    *m_map_disp = *m_map;
}
