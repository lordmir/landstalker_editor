#include "HeightmapEditorCtrl.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <EditorFrame.h>
#include <RoomViewerFrame.h>

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
        m_width = (m_map->GetHeightmapWidth() + m_map->GetHeightmapHeight()) * TILE_WIDTH / 2 + 1;
        m_height = (m_map->GetHeightmapWidth() + m_map->GetHeightmapHeight()) * TILE_HEIGHT / 2 + 1;
        RefreshGraphics();
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
	return false;
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
        dc.SetPen(wxPen(*wxWHITE));
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
    if (m_hovered.first != -1)
    {
        int x = (m_map->GetHeightmapHeight() + m_hovered.first - m_hovered.second - 1) * TILE_WIDTH / 2;
        int y = (m_hovered.first + m_hovered.second) * TILE_HEIGHT / 2;
        auto lines = GetTilePoly(0, 0);
        dc.SetPen(*wxYELLOW_PEN);
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
    auto xy = GetAbsoluteCoordinates(evt.GetX(), evt.GetY());
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
    if(m_hovered.first != ix || m_hovered.second != iy)
    {
        if (ix == -1)
        {
            m_status_text = "";
        }
        else
        {
            uint8_t z = 0, r = 0, t = 0;
            if (m_map != nullptr)
            {
                z = m_map->GetHeight({ ix, iy });
                r = m_map->GetCellProps({ ix, iy });
                t = m_map->GetCellType({ ix, iy });
            }
            m_status_text = StrPrintf("(%04d, %04d) : Z:%02d R:%01X T:%02X", ix, iy, z, r, t);
        }
        m_hovered.first = ix;
        m_hovered.second = iy;
        FireEvent(EVT_STATUSBAR_UPDATE);
        ForceRedraw();
    }
    evt.Skip();
}

void HeightmapEditorCtrl::OnMouseLeave(wxMouseEvent& evt)
{
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

void HeightmapEditorCtrl::OnLeftClick(wxMouseEvent& evt)
{
    evt.Skip();
}

void HeightmapEditorCtrl::OnLeftDblClick(wxMouseEvent& evt)
{
    evt.Skip();
}

void HeightmapEditorCtrl::OnRightClick(wxMouseEvent& evt)
{
    evt.Skip();
}

void HeightmapEditorCtrl::OnRightDblClick(wxMouseEvent& evt)
{
    evt.Skip();
}
