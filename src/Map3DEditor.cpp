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
EVT_LEFT_UP(Map3DEditor::OnLeftClick)
EVT_RIGHT_UP(Map3DEditor::OnRightClick)
EVT_SHOW(Map3DEditor::OnShow)
wxEND_EVENT_TABLE()

Map3DEditor::Map3DEditor(wxWindow* parent, RoomViewerFrame* frame, Tilemap3D::Layer layer)
    : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
    m_roomnum(0),
    m_layer(layer),
    m_frame(frame),
    m_bmp(new wxBitmap),
    m_zoom(1.0),
    m_scroll_rate(SCROLL_RATE),
    m_width(1),
    m_height(1),
    m_redraw(false),
    m_repaint(false),
    m_hovered(-1, -1),
    m_selected(-1, -1),
    m_layer_buf(std::make_unique<ImageBuffer>()),
    m_bg_buf(std::make_unique<ImageBuffer>()),
    m_show_blocknums(false),
    m_show_borders(true),
    m_show_priority(true),
    m_selected_region(-1),
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
        m_roomnum = roomnum;
        m_map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
        m_pal = m_g->GetRoomData()->GetPaletteForRoom(m_roomnum)->GetData();
        m_tileset = m_g->GetRoomData()->GetTilesetForRoom(m_roomnum)->GetData();
        m_blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(m_roomnum);
        SetSelectedSwap(-1);
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
    if (swap >= 0 && swap < static_cast<int>(m_swaps.size()))
    {
        new_region = swap;
    }
    if (new_region != m_selected_region)
    {
        m_selected_region = new_region;
        Refresh();
    }
}

int Map3DEditor::GetSelectedSwap() const
{
    return IsSwapSelected() ? m_selected_region : - 1;
}

bool Map3DEditor::IsSwapSelected() const
{
    return (m_selected_region >= 0 && m_selected_region < static_cast<int>(m_swaps.size()));
}

void Map3DEditor::SetSelectedDoor(int door)
{
    int new_region = -1;
    if (door >= 0 && door < static_cast<int>(m_doors.size()))
    {
        new_region = 0x100 + door;
    }
    if (new_region != m_selected_region)
    {
        m_selected_region = new_region;
        Refresh();
    }
}

int Map3DEditor::GetSelectedDoor() const
{
    return IsDoorSelected() ? m_selected_region - 0x100 : -1;
}

bool Map3DEditor::IsDoorSelected() const
{
    return (m_selected_region >= 0x100 && m_selected_region < static_cast<int>(0x100 + m_doors.size()));
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
        m_swaps = m_g->GetRoomData()->GetTileSwaps(m_roomnum);
        m_swap_regions.clear();
    }
}

void Map3DEditor::UpdateDoors()
{
    if (m_g)
    {
        m_doors = m_g->GetRoomData()->GetDoors(m_roomnum);
        m_door_regions.clear();
    }
}

bool Map3DEditor::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
    if (key == WXK_ESCAPE)
    {
        m_selected = { -1, -1 };
        m_hovered = { -1, -1 };
        SetSelectedSwap(-1);
        return true;
    }
    if (!HandleRegionKeyDown(key, modifiers))
    {
        return HandleDrawKeyDown(key, modifiers);
    }
    return true;
}

bool Map3DEditor::HandleDrawKeyDown(unsigned int key, unsigned int /*modifiers*/)
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
    case WXK_LEFT:
    case 'a':
    case 'A':
        if (IsDoorSelected())
        {
            if (modifiers == wxMOD_SHIFT)
            {
                DECDSZ(m_doors[GetSelectedDoor()].size, 8);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_doors[GetSelectedDoor()].x);
                upd = true;
            }
        }
        else if(IsSwapSelected())
        {
            if (modifiers == (wxMOD_ALT | wxMOD_CONTROL))
            {
                DECR(m_swaps[GetSelectedSwap()].map.dst_x);
                upd = true;
            }
            else if (modifiers == wxMOD_SHIFT)
            {
                DECSZ(m_swaps[GetSelectedSwap()].map.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedSwap()].map.src_x);
                upd = true;
            }
        }
        break;
    case WXK_RIGHT:
    case 'd':
    case 'D':
        if (IsDoorSelected())
        {
            if (modifiers == wxMOD_SHIFT)
            {
                INCDSZ(m_doors[GetSelectedDoor()].size, 8);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_doors[GetSelectedDoor()].x);
                upd = true;
            }
        }
        else if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_ALT | wxMOD_CONTROL))
            {
                INCR(m_swaps[GetSelectedSwap()].map.dst_x);
                upd = true;
            }
            else if (modifiers == wxMOD_SHIFT)
            {
                INCSZ(m_swaps[GetSelectedSwap()].map.width);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedSwap()].map.src_x);
                upd = true;
            }
        }
        break;
    case WXK_UP:
    case 'w':
    case 'W':
        if (IsDoorSelected())
        {
            if (modifiers == wxMOD_SHIFT)
            {
                DECDSZ(m_doors[GetSelectedDoor()].size, 8);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_doors[GetSelectedDoor()].y);
                upd = true;
            }
        }
        else if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_ALT | wxMOD_CONTROL))
            {
                DECR(m_swaps[GetSelectedSwap()].map.dst_y);
                upd = true;
            }
            else if (modifiers == wxMOD_SHIFT)
            {
                DECSZ(m_swaps[GetSelectedSwap()].map.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                DECR(m_swaps[GetSelectedSwap()].map.src_y);
                upd = true;
            }
        }
        break;
    case WXK_DOWN:
    case 's':
    case 'S':
        if (IsDoorSelected())
        {
            if (modifiers == wxMOD_SHIFT)
            {
                INCDSZ(m_doors[GetSelectedDoor()].size, 8);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_doors[GetSelectedDoor()].y);
                upd = true;
            }
        }
        else if (IsSwapSelected())
        {
            if (modifiers == (wxMOD_ALT | wxMOD_CONTROL))
            {
                INCR(m_swaps[GetSelectedSwap()].map.dst_y);
                upd = true;
            }
            else if (modifiers == wxMOD_SHIFT)
            {
                INCSZ(m_swaps[GetSelectedSwap()].map.height);
                upd = true;
            }
            else if (modifiers == wxMOD_CONTROL)
            {
                INCR(m_swaps[GetSelectedSwap()].map.src_y);
                upd = true;
            }
        }
        break;
    case 'T':
    case 't':
        if (IsSwapSelected())
        {
            if ((modifiers & wxMOD_SHIFT) == wxMOD_SHIFT)
            {
                DECDSZ(m_swaps[GetSelectedSwap()].mode, 3);
                upd = true;
            }
            else
            {
                INCDSZ(m_swaps[GetSelectedSwap()].mode, 3);
                upd = true;
            }
        }
        break;
    }

    if (upd)
    {
        if (m_g)
        {
            m_g->GetRoomData()->SetDoors(m_roomnum, m_doors);
            m_g->GetRoomData()->SetTileSwaps(m_roomnum, m_swaps);
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

bool Map3DEditor::HandleMouse(unsigned int buttons, unsigned int modifiers, int x, int y)
{
    if (UpdateHoveredPosition(x, y))
    {
        RefreshStatusbar();
        Refresh(false);
    }
    if ((modifiers & wxMOD_CONTROL) > 0)
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
    if (buttons == wxMOUSE_BTN_LEFT)
    {
        return HandleLeftDown(modifiers);
    }
    else if (buttons == wxMOUSE_BTN_RIGHT)
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
            SetSelectedSwap(sw.first);
        }
        else if (dw != -1)
        {
            SetSelectedDoor(dw);
        }
        else
        {
            SetSelectedSwap(-1);
        }
    }
    else if (IsBlockSelected() && m_hovered.first != -1)
    {
        SetHoveredTile();
    }
    return false;
}

bool Map3DEditor::HandleRightDown(unsigned int modifiers)
{
    if ((modifiers & wxMOD_CONTROL) == 0)
    {
        SelectHoveredTile();
    }
    return false;
}

void Map3DEditor::SetHoveredTile()
{
    if (m_hovered.first != -1)
    {
        m_map->SetBlock({ static_cast<uint16_t>(m_selected_block), {m_hovered.first, m_hovered.second} }, m_layer);
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
    if (m_hovered.first != -1)
    {
        uint16_t i = 0;
        if (m_map != nullptr)
        {
            i = m_map->GetBlock({ m_hovered.first, m_hovered.second }, m_layer);
        }
        hovmsg = StrPrintf("(%04d, %04d) : 0x%04X", m_hovered.first, m_hovered.second, i);
    }
    if (IsBlockSelected())
    {
        selmsg = StrPrintf("SELECTED 0x%04X", m_selected_block);
    }
    FireUpdateStatusEvent(hovmsg, 0);
    FireUpdateStatusEvent(selmsg, 1);
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
    if (m_map)
    {
        dc.SetPen(wxColor(128,128,128));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        for (int y = 0; y < m_map->GetHeight(); ++y)
        {
            for (int x = 0; x < m_map->GetWidth(); ++x)
            {
                int xp = (m_map->GetHeight() - 1 + x - y) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::BG ? TILE_WIDTH : 0);
                int yp = (x + y) * TILE_HEIGHT / 2;
                if (m_show_borders)
                {
                    dc.DrawRectangle(xp, yp, TILE_WIDTH + 1, TILE_HEIGHT + 1);
                }
                uint16_t blk = m_map->GetBlock({ x,y }, m_layer);
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
        m_layer_buf->Insert3DMapLayer(0, 0, 0, m_layer, m_map, m_tileset, m_blockset, false);
        if (m_layer == Tilemap3D::Layer::FG)
        {
            m_bg_buf->Clear();
            m_bg_buf->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, m_map, m_tileset, m_blockset, false);
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
    int offsetx = 0, offsety = 0;
    m_swap_regions.clear();
    if (m_map)
    {
        offsetx = m_map->GetLeft();
        offsety = m_map->GetTop();
    }
    int si = 0;
    for (const auto& s : m_swaps)
    {
        std::pair<int, int> src, dst;
        int tsoffsetx = 0, tsoffsety = 0;
        if (s.mode == TileSwap::Mode::WALL_NE && m_layer == Tilemap3D::Layer::FG)
        {
            tsoffsetx = 1;
        }
        else if (s.mode == TileSwap::Mode::WALL_NW && m_layer == Tilemap3D::Layer::BG)
        {
            tsoffsety = 1;
        }
        auto lines = GetRegionPoly(0, 0, s.map.width, s.map.height, s.mode);
        src = { s.map.src_x - offsetx + tsoffsetx, s.map.src_y - offsety + tsoffsety };
        dst = { s.map.dst_x - offsetx + tsoffsetx, s.map.dst_y - offsety + tsoffsety };
        auto sp = GetScreenPosition(src);
        auto dp = GetScreenPosition(dst);
        auto [hx, hy] = GetScreenPosition(m_hovered);
        m_swap_regions.push_back({OffsetRegionPoly(lines, sp), OffsetRegionPoly(lines, dp)});
        bool hovered = Pnpoly(m_swap_regions.back().first, hx, hy) || Pnpoly(m_swap_regions.back().second, hx, hy);
        dc.SetPen(wxPen(hovered ? wxColor(128, 128, 255) : *wxBLUE, m_selected_region == si ? 3 : 1));
        dc.DrawPolygon(lines.size(), lines.data(), sp.first, sp.second);
        dc.SetPen(wxPen(hovered ? wxColor(255, 128, 128) : *wxRED, m_selected_region == si ? 3 : 1));
        dc.DrawPolygon(lines.size(), lines.data(), dp.first, dp.second);
        ++si;
    }
}

void Map3DEditor::DrawDoors(wxDC& dc)
{
    int offsetx = 0, offsety = 0, di = 0x100;
    m_door_regions.clear();
    if (m_map)
    {
        offsetx = m_map->GetLeft();
        offsety = m_map->GetTop();
    }
    for (const auto& d : m_doors)
    {
        if (m_map && d.x >= 0 && d.x < m_map->GetHeightmapWidth() && d.y >= 0 && d.y < m_map->GetHeightmapHeight())
        {
            int z = m_map->GetHeight({ d.x, d.y });
            auto type =  static_cast<Tilemap3D::FloorType>(m_map->GetCellType({ d.x, d.y }));
            if (type != Tilemap3D::FloorType::DOOR_NE && type != Tilemap3D::FloorType::DOOR_NW)
            {
                continue;
            }
            int doorxoffset = type == Tilemap3D::FloorType::DOOR_NE ? 1 : 0;
            int dooryoffset = 0;
            if (m_layer == Tilemap3D::Layer::BG)
            {
                --doorxoffset;
            }
            if (m_layer != Tilemap3D::Layer::FG || type != Tilemap3D::FloorType::DOOR_NE)
            {
                ++doorxoffset;
                ++dooryoffset;
            }
            doorxoffset -= Door::SIZES.at(d.size).second;
            dooryoffset -= Door::SIZES.at(d.size).second;
            auto lines = GetRegionPoly(0, 0, Door::SIZES.at(d.size).first, Door::SIZES.at(d.size).second,
                type == Tilemap3D::FloorType::DOOR_NE ? TileSwap::Mode::WALL_NE : TileSwap::Mode::WALL_NW);
            std::pair<int, int> loc{ d.x + 12 - offsetx - z + doorxoffset,
                                     d.y + 12 - offsety - z + dooryoffset };
            auto pos = GetScreenPosition(loc);
            auto [hx, hy] = GetScreenPosition(m_hovered);
            m_door_regions.push_back(OffsetRegionPoly(lines, pos));
            dc.SetPen(wxPen(Pnpoly(m_door_regions.back(), hx, hy) ? wxColor(255, 128, 255) : wxColor(255, 0, 255),
                di == m_selected_region ? 3 : 1));
            dc.DrawPolygon(lines.size(), lines.data(), pos.first, pos.second);
            di++;
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
    DrawTileSwaps(dc);
    DrawDoors(dc);
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
    evt.Skip(!HandleMouse(0, evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnMouseLeave(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(0, evt.GetModifiers(), -10000, -10000));
    evt.Skip();
}

void Map3DEditor::OnLeftClick(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(wxMOUSE_BTN_LEFT, evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnRightClick(wxMouseEvent& evt)
{
    evt.Skip(!HandleMouse(wxMOUSE_BTN_RIGHT, evt.GetModifiers(), evt.GetX(), evt.GetY()));
}

void Map3DEditor::OnShow(wxShowEvent& evt)
{
    RefreshStatusbar();
    evt.Skip();
}

std::vector<wxPoint> Map3DEditor::GetRegionPoly(int x, int y, int w, int h, TileSwap::Mode mode)
{
    std::vector<wxPoint> points;
    int xstep = x;
    int ystep = y;

    if (mode == TileSwap::Mode::WALL_NW)
    {
        xstep += TILE_WIDTH;
        for (int i = 0; i < w; ++i)
        {
            points.push_back({ xstep, ystep });
            xstep -= TILE_WIDTH;
            points.push_back({ xstep, ystep });
            ystep += TILE_HEIGHT / 2;
        }
        ystep -= TILE_HEIGHT / 2;
    }
    else
    {
        for (int i = 0; i < w; ++i)
        {
            points.push_back({ xstep, ystep });
            xstep += TILE_WIDTH;
            points.push_back({ xstep, ystep });
            ystep += TILE_HEIGHT / 2;
        }
    }
    if (mode == TileSwap::Mode::FLOOR)
    {
        for (int i = 0; i < h; ++i)
        {
            ystep += TILE_HEIGHT / 2;
            points.push_back({ xstep, ystep });
            xstep -= TILE_WIDTH;
            points.push_back({ xstep, ystep });
        }
    }
    else
    {
        ystep += TILE_HEIGHT * h;
    }
    if (mode == TileSwap::Mode::WALL_NW)
    {
        for (int i = 1; i < w; ++i)
        {
            points.push_back({ xstep, ystep });
            xstep += TILE_WIDTH;
            points.push_back({ xstep, ystep });
            ystep -= TILE_HEIGHT / 2;
        }
        //ystep += TILE_HEIGHT / 2;
    }
    else
    {
        ystep -= TILE_HEIGHT / 2;
        for (int i = 1; i < w; ++i)
        {
            points.push_back({ xstep, ystep });
            xstep -= TILE_WIDTH;
            points.push_back({ xstep, ystep });
            ystep -= TILE_HEIGHT / 2;
        }
    }
    if (mode == TileSwap::Mode::FLOOR)
    {
        for (int i = 1; i < h; ++i)
        {
            ystep -= TILE_HEIGHT / 2;
            points.push_back({ xstep, ystep });
            xstep += TILE_WIDTH;
            points.push_back({ xstep, ystep });
        }
    }
    else
    {
        points.push_back({ xstep, ystep });
        xstep += mode == TileSwap::Mode::WALL_NW ? static_cast<int>(TILE_WIDTH) : -static_cast<int>(TILE_WIDTH);
        points.push_back({ xstep, ystep });
        ystep -= TILE_HEIGHT * h;
        points.push_back({ xstep, ystep });
    }

    return points;
}

std::vector<wxPoint> Map3DEditor::OffsetRegionPoly(std::vector<wxPoint> points, const Coord& offset)
{
    std::transform(points.begin(), points.end(), points.begin(), [&](const auto& pt)
        {
            return wxPoint{ pt.x + offset.first, pt.y + offset.second };
        });
    return points;
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
            return { i, false };
        }
        if (Pnpoly(m_swap_regions[i].second, x, y) == true)
        {
            return { i, true };
        }
    }
    return {-1, false};
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
