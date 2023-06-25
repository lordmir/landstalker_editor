#include "HeightmapEditorCtrl.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <EditorFrame.h>

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

HeightmapEditorCtrl::HeightmapEditorCtrl(wxWindow* parent)
    : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS),
    m_roomnum(0),
    m_bmp(new wxBitmap),
    m_zoom(1.0),
    m_scroll_rate(SCROLL_RATE),
    m_width(1),
    m_height(1),
    m_redraw(false),
    m_repaint(false),
    m_selected(-1)
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
    m_roomnum = roomnum;
    RefreshGraphics();
}

void HeightmapEditorCtrl::SetZoom(double zoom)
{
    m_zoom = zoom;
    if (m_g == nullptr)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
    *m_bmp = *DrawHeightmapGrid(map);
    UpdateScroll();
    ForceRedraw();
}

void HeightmapEditorCtrl::RefreshGraphics()
{
}

bool HeightmapEditorCtrl::HandleKeyDown(unsigned int key, unsigned int modifiers)
{
	return false;
}

void HeightmapEditorCtrl::DrawRoomHeightmap(uint16_t roomnum)
{
    if (m_g == nullptr)
    {
        return;
    }

    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();

    m_width = (map->GetHeightmapWidth() + map->GetHeightmapHeight()) * TILE_WIDTH + 1;
    m_height = (map->GetHeightmapWidth() + map->GetHeightmapHeight()) * TILE_HEIGHT + 1;
    *m_bmp = *DrawHeightmapGrid(map);
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

std::shared_ptr<wxBitmap> HeightmapEditorCtrl::DrawHeightmapVisualisation(std::shared_ptr<Tilemap3D> map, uint8_t opacity)
{
    wxImage hm_img(m_width, m_height);
    hm_img.InitAlpha();
    SetOpacity(hm_img, 0x00);
    wxGraphicsContext* hm_gc = wxGraphicsContext::Create(hm_img);
    hm_gc->Scale(m_zoom, m_zoom);
    hm_gc->SetPen(*wxWHITE_PEN);
    hm_gc->SetBrush(*wxBLACK_BRUSH);
    for (int y = 0; y < map->GetHeightmapHeight(); ++y)
    {
        for (int x = 0; x < map->GetHeightmapWidth(); ++x)
        {
            // Only display cells that are not completely restricted
            if ((map->GetHeight({ x, y }) > 0 || (map->GetCellProps({ x, y }) != 0x04)))
            {
                int z = map->GetHeight({ x, y });
                auto xy(map->Iso3DToPixel({ x + 12, y + 12, z }));
                DrawHeightmapCell(*hm_gc, xy.x, xy.y, z, TILE_WIDTH, TILE_HEIGHT,
                    map->GetCellProps({ x,y }), map->GetCellType({ x,y }));
            }
        }
    }
    delete hm_gc;
    SetOpacity(hm_img, opacity);
    return std::make_shared<wxBitmap>(hm_img);
}

std::shared_ptr<wxBitmap> HeightmapEditorCtrl::DrawHeightmapGrid(std::shared_ptr<Tilemap3D> map)
{
    wxImage img(m_width, m_height);
    wxGraphicsContext* gc = wxGraphicsContext::Create(img);
    gc->SetBrush(*wxBLACK);
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, 0, m_width, m_height);
    gc->SetPen(*wxWHITE_PEN);
    gc->Scale(m_zoom, m_zoom);
    auto font = wxFont(TILE_HEIGHT / 2, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false);
    // Draw grid
    for (int y = 0; y < map->GetHeightmapHeight(); ++y)
        for (int x = 0; x < map->GetHeightmapWidth(); ++x)
        {
            int xx = (x - y + map->GetHeightmapHeight() - 1) * TILE_WIDTH;
            int xy = (x + y) * TILE_HEIGHT;
            DrawHeightmapCell(*gc, xx, xy, 0, TILE_WIDTH * 2, TILE_HEIGHT * 2, -1, -1, false, wxColor(0x444444));
            std::string lbl1 = StrPrintf("%02X", map->GetCellType({ x,y }));
            std::string lbl2 = StrPrintf("%X %X", map->GetCellType({ x,y }), map->GetCellProps({ x,y }));
        }

    // Draw cells
    for (int y = 0; y < map->GetHeightmapHeight(); ++y)
        for (int x = 0; x < map->GetHeightmapWidth(); ++x)
        {
            // Only display cells that are not completely restricted
            if ((map->GetHeight({ x, y }) > 0 || (map->GetCellProps({ x, y }) != 0x04)))
            {
                int xx = (x - y + map->GetHeightmapHeight() - 1) * TILE_WIDTH;
                int xy = (x + y) * TILE_HEIGHT;
                DrawHeightmapCell(*gc, xx, xy, map->GetHeight({ x,y }), TILE_WIDTH * 2, TILE_HEIGHT * 2, map->GetCellProps({ x,y }), -1, false);
                std::string lbl1 = StrPrintf("%02X", map->GetCellType({ x,y }));
                std::string lbl2 = StrPrintf("%X", map->GetHeight({ x,y }));
                std::string lbl3 = StrPrintf("%X", map->GetCellProps({ x,y }));
                double tw, th;
                gc->SetFont(font, map->GetCellType({ x,y }) == 0 ? *wxLIGHT_GREY : wxColor(0xFF00FF));
                gc->GetTextExtent(lbl1, &tw, &th);
                gc->DrawText(lbl1, xx + TILE_WIDTH - tw / 2, xy + TILE_HEIGHT / 2 - th / 2 + 2);
                gc->SetFont(font, *wxCYAN);
                gc->GetTextExtent(lbl2, &tw, &th);
                gc->DrawText(lbl2, xx + TILE_WIDTH - 5 * tw / 4, xy + 5 * TILE_HEIGHT / 4 - th / 2 + 2);
                gc->SetFont(font, map->GetCellProps({ x,y }) == 0 ? *wxLIGHT_GREY : *wxYELLOW);
                gc->GetTextExtent(lbl3, &tw, &th);
                gc->DrawText(lbl3, xx + TILE_WIDTH + 1 * tw / 4, xy + 5 * TILE_HEIGHT / 4 - th / 2 + 2);
            }
        }
    delete gc;

    return std::make_shared<wxBitmap>(img);
}

void HeightmapEditorCtrl::DrawHeightmapCell(wxGraphicsContext& gc, int x, int y, int zz, int width, int height, int restrictions, int classification, bool draw_walls, wxColor border_colour)
{
    int z = draw_walls ? zz : 0;
    wxPoint2DDouble tile_points[] = {
        wxPoint2DDouble(x + width / 2, y),
        wxPoint2DDouble(x + width    , y + height / 2),
        wxPoint2DDouble(x + width / 2, y + height),
        wxPoint2DDouble(x            , y + height / 2),
        wxPoint2DDouble(x + width / 2, y)
    };
    wxPoint2DDouble left_wall[] = {
        wxPoint2DDouble(x            , y + height / 2),
        wxPoint2DDouble(x + width / 2, y + height),
        wxPoint2DDouble(x + width / 2, y + height + z * height),
        wxPoint2DDouble(x            , y + height / 2 + z * height),
        wxPoint2DDouble(x            , y + height / 2)
    };
    wxPoint2DDouble right_wall[] = {
        wxPoint2DDouble(x + width / 2, y + height),
        wxPoint2DDouble(x + width    , y + height / 2),
        wxPoint2DDouble(x + width    , y + height / 2 + z * height),
        wxPoint2DDouble(x + width / 2, y + height + z * height),
        wxPoint2DDouble(x + width / 2, y + height)
    };
    wxColor bg;
    switch (restrictions)
    {
    case -1:
        bg = *wxBLACK;
        break;
    case 0x0:
        bg.Set(zz * 3, 48 + zz * 8, zz * 3);
        break;
    case 0x4:
        bg.Set(48 + zz * 8, zz * 3, zz * 3);
        break;
    case 0x2:
        bg.Set(32 + zz * 3, 32 + zz * 3, 48 + zz * 12);
        break;
    case 0x6:
        bg.Set(48 + zz * 8, 32 + zz * 3, 48 + zz * 8);
        break;
    default:
        bg.Set(48 + zz * 8, 48 + zz * 8, zz * 3);
        break;
    }
    gc.SetBrush(wxBrush(bg));
    gc.SetPen(border_colour);
    //gc.SetTextForeground(*wxWHITE);
    gc.DrawLines(sizeof(tile_points) / sizeof(tile_points[0]), tile_points);
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(left_wall) / sizeof(left_wall[0]), left_wall);
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(right_wall) / sizeof(right_wall[0]), right_wall);
    if (classification > 0)
    {
        // Set font and transparent colour
        gc.SetFont(wxFont(height / 2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false),
            wxColour(255, 255, 255, 255));
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << classification;
        double textwidth, textheight;
        gc.GetTextExtent(ss.str(), &textwidth, &textheight);
        gc.DrawText(ss.str(), x + (width - textwidth) / 2, y + (height - textheight) / 2);
    }
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

void HeightmapEditorCtrl::FireEvent(const wxEventType& e, long userdata)
{
    wxCommandEvent evt(e);
    evt.SetExtraLong(userdata);
    evt.SetClientData(this->GetParent());
    wxPostEvent(this->GetParent(), evt);
}

void HeightmapEditorCtrl::FireEvent(const wxEventType& e, const std::string& userdata)
{
    wxCommandEvent evt(e);
    evt.SetString(userdata);
    evt.SetClientData(this->GetParent());
    wxPostEvent(this->GetParent(), evt);
}

void HeightmapEditorCtrl::FireEvent(const wxEventType& e)
{
    wxCommandEvent evt(e);
    evt.SetClientData(this->GetParent());
    wxPostEvent(this->GetParent(), evt);
}

void HeightmapEditorCtrl::OnDraw(wxDC& dc)
{
    int sx, sy;
    GetViewStart(&sx, &sy);
    sx *= m_scroll_rate;
    sy *= m_scroll_rate;
    int sw, sh;
    GetClientSize(&sw, &sh);
    wxMemoryDC mdc(*m_bmp);
    if (m_redraw)
    {
        mdc.SetBackground(*wxGREEN_BRUSH);
        mdc.Clear();
        mdc.SetUserScale(1.0, 1.0);
    }
    mdc.SetUserScale(1.0, 1.0);
    dc.SetBackground(*wxBLUE_BRUSH);
    dc.Clear();
    dc.Blit(sx, sy, sw, sh, &mdc, sx, sy, wxCOPY);
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
