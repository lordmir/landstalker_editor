#include <Map3DEditor.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <RoomViewerFrame.h>

wxBEGIN_EVENT_TABLE(Map3DEditor, wxScrolledCanvas)
EVT_ERASE_BACKGROUND(Map3DEditor::OnEraseBackground)
EVT_PAINT(Map3DEditor::OnPaint)
EVT_SIZE(Map3DEditor::OnSize)
EVT_MOTION(Map3DEditor::OnMouseMove)
EVT_LEAVE_WINDOW(Map3DEditor::OnMouseLeave)
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
    m_selected(-1, -1)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
}

Map3DEditor::~Map3DEditor()
{
    delete m_bmp;
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
        RecreateBuffer();
    }
}

wxString Map3DEditor::GetStatusText() const
{
    return m_status_text;
}

void Map3DEditor::RefreshGraphics()
{
    UpdateScroll();
    ForceRedraw();
}

void Map3DEditor::ForceRedraw()
{
    m_redraw = true;
    m_repaint = true;
    Refresh(true);
}

void Map3DEditor::RecreateBuffer()
{
    m_width = (m_map->GetWidth() + m_map->GetHeight() - 1) * TILE_WIDTH + 1;
    m_height = (m_map->GetWidth() + m_map->GetHeight()) * TILE_HEIGHT / 2 + 1;
    if (m_layer == Tilemap3D::Layer::FG)
    {
        m_width += TILE_WIDTH / 2;
    }
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

void Map3DEditor::DrawMap()
{
    m_bmp->Create(m_width, m_height);
    wxMemoryDC dc(*m_bmp);
    dc.SetUserScale(m_zoom, m_zoom);
    dc.SetBackground(m_layer == Tilemap3D::Layer::FG ? *wxGREEN_BRUSH : *wxBLUE_BRUSH);
    dc.Clear();
    if (m_map)
    {
        dc.SetPen(wxColor(128,128,128));
        dc.SetBrush(wxColor(0,0,96));
        for (int y = 0; y < m_map->GetHeight(); ++y)
        {
            for (int x = 0; x < m_map->GetWidth(); ++x)
            {
                int xp = (m_map->GetHeight() - 1 + x - y) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::FG ? TILE_WIDTH / 2 : 0);
                int yp = (x + y) * TILE_HEIGHT / 2;
                dc.DrawRectangle(xp, yp, TILE_WIDTH + 1, TILE_HEIGHT + 1);
                wxString t = StrPrintf("%03X", m_map->GetBlock({x,y}, m_layer));
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
    dc.SelectObject(wxNullBitmap);
}

void Map3DEditor::OnDraw(wxDC& dc)
{
    if (m_redraw)
    {
        DrawMap();
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
    dc.SetUserScale(m_zoom, m_zoom);
    mdc.SelectObject(wxNullBitmap);
    if (m_hovered.first != -1 && m_hovered != m_selected)
    {
        int xp = (m_map->GetHeight() - 1 + m_hovered.first - m_hovered.second) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::FG ? TILE_WIDTH / 2 : 0);
        int yp = (m_hovered.first + m_hovered.second) * TILE_HEIGHT / 2;
        dc.SetPen(*wxWHITE_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(xp, yp, TILE_WIDTH + 1, TILE_HEIGHT + 1);
    }
    if (m_selected.first != -1 && m_hovered != m_selected)
    {
        int xp = (m_map->GetHeight() - 1 + m_selected.first - m_selected.second) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::FG ? TILE_WIDTH / 2 : 0);
        int yp = (m_selected.first + m_selected.second) * TILE_HEIGHT / 2;
        dc.SetPen(*wxYELLOW_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(xp, yp, TILE_WIDTH + 1, TILE_HEIGHT + 1);
    }
    if (m_selected.first != -1 && m_hovered == m_selected)
    {
        int xp = (m_map->GetHeight() - 1 + m_hovered.first - m_hovered.second) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::FG ? TILE_WIDTH / 2 : 0);
        int yp = (m_hovered.first + m_hovered.second) * TILE_HEIGHT / 2;
        dc.SetPen(wxColor(255, 255, 128));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(xp, yp, TILE_WIDTH + 1, TILE_HEIGHT + 1);
    }
}

void Map3DEditor::OnPaint(wxPaintEvent& evt)
{
    wxBufferedPaintDC dc(this);
    this->PrepareDC(dc);
    this->OnDraw(dc);
}

void Map3DEditor::OnEraseBackground(wxEraseEvent& evt)
{
}

void Map3DEditor::OnSize(wxSizeEvent& evt)
{
    Refresh(false);
    evt.Skip();
}

void Map3DEditor::OnMouseMove(wxMouseEvent& evt)
{
    if (UpdateHoveredPosition(evt.GetX(), evt.GetY()))
    {
        if (m_hovered.first == -1)
        {
            m_status_text = "";
        }
        else
        {
            uint16_t i = 0;
            if (m_map != nullptr)
            {
                i = m_map->GetBlock({m_hovered.first, m_hovered.second}, m_layer);
            }
            m_status_text = StrPrintf("(%04d, %04d) : 0x%04X", m_hovered.first, m_hovered.second, i);
        }
        FireEvent(EVT_STATUSBAR_UPDATE);
        Refresh(false);
    }
    evt.Skip();
}

void Map3DEditor::OnMouseLeave(wxMouseEvent& evt)
{
    if (UpdateHoveredPosition(-10000, -10000))
    {
        m_status_text = "";
        FireEvent(EVT_STATUSBAR_UPDATE);
        Refresh(false);
    }

    evt.Skip();
}

std::pair<int, int> Map3DEditor::GetAbsoluteCoordinates(int screenx, int screeny)
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

std::pair<int, int> Map3DEditor::GetCellPosition(int screenx, int screeny)
{
    auto xy = GetAbsoluteCoordinates(screenx, screeny);
    int ix = 0, iy = 0;
    int offsetx = (m_map->GetHeight() - 1) * TILE_WIDTH + (m_layer == Tilemap3D::Layer::FG ? TILE_WIDTH / 2 : 0);
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

bool Map3DEditor::IsCoordValid(std::pair<int, int> c)
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

void Map3DEditor::FireEvent(const wxEventType& e, long userdata)
{
    wxCommandEvent evt(e);
    evt.SetExtraLong(userdata);
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
