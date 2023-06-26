#include "HeightmapEditorCtrl.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <EditorFrame.h>
#include <RoomViewerFrame.h>

wxDEFINE_EVENT(EVT_HEIGHTMAP_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(EVT_HEIGHTMAP_MOVE, wxCommandEvent);

wxBEGIN_EVENT_TABLE(HeightmapEditorCtrl, wxScrolledCanvas)
EVT_ERASE_BACKGROUND(HeightmapEditorCtrl::OnEraseBackground)
EVT_PAINT(HeightmapEditorCtrl::OnPaint)
EVT_SIZE(HeightmapEditorCtrl::OnSize)
EVT_MOTION(HeightmapEditorCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(HeightmapEditorCtrl::OnMouseLeave)
EVT_LEFT_UP(HeightmapEditorCtrl::OnLeftClick)
EVT_LEFT_DCLICK(HeightmapEditorCtrl::OnLeftDblClick)
EVT_RIGHT_UP(HeightmapEditorCtrl::OnRightClick)
EVT_RIGHT_DCLICK(HeightmapEditorCtrl::OnRightDblClick)
wxEND_EVENT_TABLE()

HeightmapEditorCtrl::HeightmapEditorCtrl(wxWindow* parent, RoomViewerFrame* frame)
    : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
    m_roomnum(0),
    m_frame(frame),
    m_bmp(new wxBitmap),
    m_zoom(1.0),
    m_scroll_rate(SCROLL_RATE),
    m_width(1),
    m_height(1),
    m_redraw(false),
    m_repaint(false),
    m_hovered(-1, -1),
    m_selected(-1, -1)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
}

HeightmapEditorCtrl::~HeightmapEditorCtrl()
{
    delete m_bmp;
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
        m_roomnum = roomnum;
        m_map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
        RecreateBuffer();

    }
}

wxString HeightmapEditorCtrl::GetStatusText() const
{
    return m_status_text;
}

void HeightmapEditorCtrl::SetZoom(double zoom)
{
    m_zoom = zoom;
    RefreshGraphics();
}

void HeightmapEditorCtrl::RefreshGraphics()
{
    UpdateScroll();
    ForceRedraw();
}

bool HeightmapEditorCtrl::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
    switch(key)
    {
    case WXK_ESCAPE:
        ClearSelection();
        return true;
    case WXK_UP:
    case 'w':
    case 'W':
        if (modifiers == wxMOD_CONTROL)
        {
            InsertColumnLeft();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapUp();
        }
        else
        {
            NudgeSelectionUp();
        }
        return true;
    case WXK_DOWN:
    case 's':
    case 'S':
        if (modifiers == wxMOD_CONTROL)
        {
            InsertColumnRight();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapDown();
        }
        else
        {
            NudgeSelectionDown();
        }
        return true;
    case WXK_LEFT:
    case 'a':
    case 'A':
        if (modifiers == wxMOD_CONTROL)
        {
            InsertRowBelow();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapLeft();
        }
        else
        {
            NudgeSelectionLeft();
        }
        return true;
    case WXK_RIGHT:
    case 'd':
    case 'D':
        if (modifiers == wxMOD_CONTROL)
        {
            InsertRowAbove();
        }
        else if (modifiers == wxMOD_ALT)
        {
            NudgeHeightmapRight();
        }
        else
        {
            NudgeSelectionRight();
        }
        return true;
    case WXK_PAGEUP:
        IncreaseSelectedHeight();
        return true;
    case WXK_PAGEDOWN:
        DecreaseSelectedHeight();
        return true;
    case WXK_SPACE:
        ToggleSelectedPlayerPassable();
        return true;
    case 'n':
    case 'N':
        ToggleSelectedNPCPassable();
        return true;
    case 'r':
    case 'R':
        ToggleSelectedRaftTrack();
        return true;
    case WXK_ADD:
    case '+':
        if (modifiers == wxMOD_CONTROL)
        {
            IncrementSelectedRestrictions();
        }
        else
        {
            IncrementSelectedType();
        }
        return true;
    case WXK_SUBTRACT:
    case '-':
        if (modifiers == wxMOD_CONTROL)
        {
            DecrementSelectedRestrictions();
        }
        else
        {
            DecrementSelectedType();
        }
        return true;
    case WXK_DELETE:
        if (modifiers == wxMOD_CONTROL)
        {
            DeleteRow();
        }
        else if (modifiers == wxMOD_ALT)
        {
            DeleteColumn();
        }
        else
        {
            ClearSelectedCell();
        }
        return true;
    }
	return false;
}

void HeightmapEditorCtrl::ClearSelection()
{
    if (m_selected.first != -1)
    {
        m_selected = { -1, -1 };
        Refresh(false);
    }
}

void HeightmapEditorCtrl::SetSelection(int ix, int iy)
{
    if (m_selected.first != ix || m_selected.second != iy)
    {
        if (ix >= 0 && ix < m_map->GetHeightmapWidth() && iy >= 0 && iy < m_map->GetHeightmapWidth())
        {
            m_selected = { ix, iy };
            Refresh(false);
        }
        else
        {
            ClearSelection();
        }
    }
}

std::pair<int, int> HeightmapEditorCtrl::GetSelection() const
{
    return m_selected;
}

bool HeightmapEditorCtrl::IsSelectionValid() const
{
    return m_selected.first != -1;
}

void HeightmapEditorCtrl::NudgeSelectionUp()
{
    if (m_selected.first == -1 && m_hovered.first == -1)
    {
        m_selected = { 0, 0 };
        Refresh(false);
    }
    else if (m_selected.first > 0)
    {
        if (m_selected.first == -1)
        {
            m_selected = m_hovered;
        }
        m_selected.first--;
        Refresh(false);
    }
}

void HeightmapEditorCtrl::NudgeSelectionDown()
{
    if (m_selected.first == -1 && m_hovered.first == -1)
    {
        m_selected = { 0, 0 };
        Refresh(false);
    }
    else if (m_selected.first < m_map->GetHeightmapWidth() - 1)
    {
        if (m_selected.first == -1)
        {
            m_selected = m_hovered;
        }
        m_selected.first++;
        Refresh(false);
    }
}

void HeightmapEditorCtrl::NudgeSelectionLeft()
{
    if (m_selected.first == -1 && m_hovered.first == -1)
    {
        m_selected = { 0, 0 };
        Refresh(false);
    }
    else if (m_selected.second < m_map->GetHeightmapHeight() - 1)
    {
        if (m_selected.first == -1)
        {
            m_selected = m_hovered;
        }
        m_selected.second++;
        Refresh(false);
    }
}

void HeightmapEditorCtrl::NudgeSelectionRight()
{
    if (m_selected.first == -1 && m_hovered.first == -1)
    {
        m_selected = { 0, 0 };
        Refresh(false);
    }
    else if (m_selected.second > 0)
    {
        if (m_selected.first == -1)
        {
            m_selected = m_hovered;
        }
        m_selected.second--;
        Refresh(false);
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
    if (m_selected.second != -1 && m_map->GetHeightmapHeight() < 64)
    {
        m_map->InsertHeightmapRow(m_selected.second);
        m_selected.second++;
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::InsertRowBelow()
{
    if (m_selected.first != -1 && m_map->GetHeightmapHeight() < 64)
    {
        m_map->InsertHeightmapRow(m_selected.second);
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::DeleteRow()
{
    if (m_selected.first != -1 && m_map->GetHeightmapHeight() > 1)
    {
        m_map->DeleteHeightmapRow(m_selected.second);
        if (m_selected.second >= m_map->GetHeightmapHeight())
        {
            m_selected.second = m_map->GetHeightmapHeight() - 1;
        }
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::InsertColumnLeft()
{
    if (m_selected.first != -1 && m_map->GetHeightmapWidth() < 64)
    {
        m_map->InsertHeightmapColumn(m_selected.first);
        m_selected.first++;
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::InsertColumnRight()
{
    if (m_selected.first != -1 && m_map->GetHeightmapWidth() < 64)
    {
        m_map->InsertHeightmapColumn(m_selected.first);
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

void HeightmapEditorCtrl::DeleteColumn()
{
    if (m_selected.first != -1 && m_map->GetHeightmapWidth() > 1)
    {
        m_map->DeleteHeightmapColumn(m_selected.first);
        if (m_selected.first >= m_map->GetHeightmapWidth())
        {
            m_selected.first = m_map->GetHeightmapWidth() - 1;
        }
        RecreateBuffer();
        FireEvent(EVT_HEIGHTMAP_UPDATE);
        FireEvent(EVT_PROPERTIES_UPDATE);
    }
}

uint8_t HeightmapEditorCtrl::GetSelectedHeight() const
{
    return m_map->GetHeight({ m_selected.first, m_selected.second });
}

void HeightmapEditorCtrl::SetSelectedHeight(uint8_t height)
{
    m_map->SetHeight({ m_selected.first, m_selected.second }, height);
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

void HeightmapEditorCtrl::IncreaseSelectedHeight()
{
    uint8_t height = GetSelectedHeight();
    if (height < 15)
    {
        ++height;
    }
    SetSelectedHeight(height);
}

void HeightmapEditorCtrl::DecreaseSelectedHeight()
{
    uint8_t height = GetSelectedHeight();
    if (height > 0)
    {
        --height;
    }
    SetSelectedHeight(height);
}

void HeightmapEditorCtrl::ClearSelectedCell()
{
    m_map->SetHeight({ m_selected.first, m_selected.second }, 0);
    m_map->SetCellProps({ m_selected.first, m_selected.second }, 4);
    m_map->SetCellType({ m_selected.first, m_selected.second }, 0);
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

uint8_t HeightmapEditorCtrl::GetSelectedRestrictions() const
{
    return m_map->GetCellProps({ m_selected.first, m_selected.second });
}

void HeightmapEditorCtrl::SetSelectedRestrictions(uint8_t restrictions)
{
    m_map->SetCellProps({ m_selected.first, m_selected.second }, restrictions);
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

bool HeightmapEditorCtrl::IsSelectedPlayerPassable() const
{
    return (GetSelectedRestrictions() & 0x04) > 0;
}

void HeightmapEditorCtrl::ToggleSelectedPlayerPassable()
{
    SetSelectedRestrictions(GetSelectedRestrictions() ^ 0x04);
}

bool HeightmapEditorCtrl::IsSelectedNPCPassable() const
{
    return (GetSelectedRestrictions() & 0x02) == 0;
}

void HeightmapEditorCtrl::ToggleSelectedNPCPassable()
{
    SetSelectedRestrictions(GetSelectedRestrictions() ^ 0x02);
}

bool HeightmapEditorCtrl::IsSelectedRaftTrack() const
{
    return (GetSelectedRestrictions() & 0x01) > 0;
}

void HeightmapEditorCtrl::ToggleSelectedRaftTrack()
{
    SetSelectedRestrictions(GetSelectedRestrictions() ^ 0x01);
}

void HeightmapEditorCtrl::IncrementSelectedRestrictions()
{
    SetSelectedRestrictions((GetSelectedRestrictions() + 1) & 0x0F);
}

void HeightmapEditorCtrl::DecrementSelectedRestrictions()
{
    SetSelectedRestrictions((GetSelectedRestrictions() - 1) & 0x0F);
}

uint8_t HeightmapEditorCtrl::GetSelectedType() const
{
    return m_map->GetCellType({m_selected.first, m_selected.second});
}

void HeightmapEditorCtrl::SetSelectedType(uint8_t type)
{
    m_map->SetCellType({ m_selected.first, m_selected.second }, type);
    ForceRedraw();
    FireEvent(EVT_HEIGHTMAP_UPDATE);
}

void HeightmapEditorCtrl::IncrementSelectedType()
{
    SetSelectedType((GetSelectedType() + 1) & 0xFF);
}

void HeightmapEditorCtrl::DecrementSelectedType()
{
    SetSelectedType((GetSelectedType() - 1) & 0xFF);
}

void HeightmapEditorCtrl::DrawRoomHeightmap()
{
    m_bmp->Create(m_width, m_height);
    wxMemoryDC dc(*m_bmp);

    dc.SetUserScale(m_zoom, m_zoom);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
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
                auto restrictions = m_map->GetCellProps({ ix, iy });
                auto z = m_map->GetHeight({ ix, iy });
                auto type = m_map->GetCellType({ ix, iy });
                dc.SetBrush(wxBrush(GetCellBackground(restrictions, type, z)));
                dc.DrawPolygon(lines.size(), lines.data(), x, y);
            }
        }
        // Do stuff
        auto font = wxFont(wxSize(0, TILE_HEIGHT / 2), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false);
        dc.SetFont(font);
        for (int iy = 0; iy < m_map->GetHeightmapHeight(); ++iy)
        {
            for (int ix = 0; ix < m_map->GetHeightmapWidth(); ++ix)
            {
                int x = (m_map->GetHeightmapHeight() + ix - iy - 1) * TILE_WIDTH / 2;
                int y = (ix + iy) * TILE_HEIGHT / 2;
                auto restrictions = m_map->GetCellProps({ ix, iy });
                auto z = m_map->GetHeight({ ix, iy });
                auto type = m_map->GetCellType({ ix, iy });
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
        ///
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColor(128,128,128)));
        for (int iy = 0; iy < m_map->GetHeightmapHeight(); ++iy)
        {
            for (int ix = 0; ix < m_map->GetHeightmapWidth(); ++ix)
            {
                int x = (m_map->GetHeightmapHeight() + ix - iy - 1) * TILE_WIDTH / 2;
                int y = (ix + iy) * TILE_HEIGHT / 2;
                auto restrictions = m_map->GetCellProps({ ix, iy });
                auto z = m_map->GetHeight({ ix, iy });
                auto type = m_map->GetCellType({ ix, iy });
                if (IsCellHidden(restrictions, type, z))
                {
                    dc.DrawPolygon(lines.size(), lines.data(), x, y);
                }
            }
        }
        dc.SetPen(wxPen(wxColor(200,200,200)));
        for (int iy = 0; iy < m_map->GetHeightmapHeight(); ++iy)
        {
            for (int ix = 0; ix < m_map->GetHeightmapWidth(); ++ix)
            {
                int x = (m_map->GetHeightmapHeight() + ix - iy - 1) * TILE_WIDTH / 2;
                int y = (ix + iy) * TILE_HEIGHT / 2;
                auto restrictions = m_map->GetCellProps({ ix, iy });
                auto z = m_map->GetHeight({ ix, iy });
                auto type = m_map->GetCellType({ ix, iy });
                if (!IsCellHidden(restrictions, type, z))
                {
                    dc.DrawPolygon(lines.size(), lines.data(), x, y);
                }
            }
        }
    }

    dc.SelectObject(wxNullBitmap);
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
    if (m_selected.first != i.first || m_selected.second != i.second)
    {
        m_selected = i;
        return true;
    }
    return false;
}

std::vector<wxPoint> HeightmapEditorCtrl::GetTilePoly(int x, int y, int width, int height) const
{
    return {
        wxPoint(x + width / 2, y),
        wxPoint(x + width    , y + height / 2),
        wxPoint(x + width / 2, y + height),
        wxPoint(x            , y + height / 2),
        wxPoint(x + width / 2, y)
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

void HeightmapEditorCtrl::UpdateScroll()
{
    m_scroll_rate = std::ceil(SCROLL_RATE * m_zoom);
    SetVirtualSize(std::ceil(m_zoom * m_width), std::ceil(m_zoom * m_height));
    SetScrollRate(m_scroll_rate, m_scroll_rate);
    AdjustScrollbars();
}

bool HeightmapEditorCtrl::Pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y)
{
    int i, j;
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
    auto name = m_g->GetRoomData()->GetRoom(room)->name;
    FireEvent(EVT_GO_TO_NAV_ITEM, "Rooms/" + name);
}

bool HeightmapEditorCtrl::IsCellHidden(uint8_t restrictions, uint8_t type, uint8_t z)
{
    return (restrictions == 0x4 && type == 0 && z == 0);
}

void HeightmapEditorCtrl::FireEvent(const wxEventType& e, long userdata)
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
    if (m_redraw)
    {
        DrawRoomHeightmap();
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
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    dc.Blit(sx, sy, sw, sh, &mdc, sx, sy, wxCOPY);
    if (m_hovered.first != -1 && m_hovered != m_selected)
    {
        int x = (m_map->GetHeightmapHeight() + m_hovered.first - m_hovered.second - 1) * TILE_WIDTH / 2;
        int y = (m_hovered.first + m_hovered.second) * TILE_HEIGHT / 2;
        auto lines = GetTilePoly(0, 0);
        dc.SetPen(*wxWHITE_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawPolygon(lines.size(), lines.data(), x, y);
    }
    if (m_selected.first != -1 && m_hovered != m_selected)
    {
        int x = (m_map->GetHeightmapHeight() + m_selected.first - m_selected.second - 1) * TILE_WIDTH / 2;
        int y = (m_selected.first + m_selected.second) * TILE_HEIGHT / 2;
        auto lines = GetTilePoly(0, 0);
        dc.SetPen(*wxYELLOW_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawPolygon(lines.size(), lines.data(), x, y);
    }
    if (m_selected.first != -1 && m_hovered == m_selected)
    {
        int x = (m_map->GetHeightmapHeight() + m_hovered.first - m_hovered.second - 1) * TILE_WIDTH / 2;
        int y = (m_hovered.first + m_hovered.second) * TILE_HEIGHT / 2;
        auto lines = GetTilePoly(0, 0);
        dc.SetPen(wxColor(255, 255, 64));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawPolygon(lines.size(), lines.data(), x, y);
    }
    mdc.SelectObject(wxNullBitmap);
}

void HeightmapEditorCtrl::OnPaint(wxPaintEvent& evt)
{
    wxBufferedPaintDC dc(this);
    this->PrepareDC(dc);
    this->OnDraw(dc);
}

void HeightmapEditorCtrl::OnEraseBackground(wxEraseEvent& evt)
{
}

void HeightmapEditorCtrl::OnSize(wxSizeEvent& evt)
{
    Refresh(false);
    evt.Skip();
}

void HeightmapEditorCtrl::OnMouseMove(wxMouseEvent& evt)
{
    if(UpdateHoveredPosition(evt.GetX(), evt.GetY()))
    {
        if (m_hovered.first == -1)
        {
            m_status_text = "";
        }
        else
        {
            uint8_t z = 0, r = 0, t = 0;
            if (m_map != nullptr)
            {
                z = m_map->GetHeight({ m_hovered.first, m_hovered.second });
                r = m_map->GetCellProps({ m_hovered.first, m_hovered.second });
                t = m_map->GetCellType({ m_hovered.first, m_hovered.second });
            }
            m_status_text = StrPrintf("(%04d, %04d) : Z:%02d R:%01X T:%02X", m_hovered.first, m_hovered.second, z, r, t);
        }
        FireEvent(EVT_STATUSBAR_UPDATE);
        Refresh(false);
    }
    evt.Skip();
}

void HeightmapEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
    ClearSelection();
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
    if (UpdateSelectedPosition(evt.GetX(), evt.GetY()))
    {
        Refresh(false);
    }
    else
    {
        IncreaseSelectedHeight();
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
        Refresh(false);
    }
    else
    {
        DecreaseSelectedHeight();
    }
    
    evt.Skip();
}

void HeightmapEditorCtrl::OnRightDblClick(wxMouseEvent& evt)
{
    evt.Skip();
}
