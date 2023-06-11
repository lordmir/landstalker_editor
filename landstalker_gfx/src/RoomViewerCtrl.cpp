#include "RoomViewerCtrl.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include "EditorFrame.h"

wxBEGIN_EVENT_TABLE(RoomViewerCtrl, wxScrolledCanvas)
EVT_ERASE_BACKGROUND(RoomViewerCtrl::OnEraseBackground)
EVT_PAINT(RoomViewerCtrl::OnPaint)
EVT_SIZE(RoomViewerCtrl::OnSize)
EVT_MOTION(RoomViewerCtrl::OnMouseMove)
EVT_LEAVE_WINDOW(RoomViewerCtrl::OnMouseLeave)
EVT_LEFT_UP(RoomViewerCtrl::OnLeftClick)
wxEND_EVENT_TABLE()

RoomViewerCtrl::RoomViewerCtrl(wxWindow* parent)
	: wxScrolledCanvas(parent, wxID_ANY),
	  m_roomnum(0),
      m_bmp(new wxBitmap),
      m_zoom(1.0),
      m_scroll_rate(SCROLL_RATE),
      m_width(1),
      m_height(1),
      m_buffer_width(1),
      m_buffer_height(1),
      m_redraw(false),
      m_repaint(false)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxBLACK);
    m_layer_opacity = { {Layer::BACKGROUND1, 0xFF}, {Layer::BACKGROUND2, 0xFF}, {Layer::BG_SPRITES, 0xFF },
                        {Layer::FOREGROUND, 0xFF}, {Layer::FG_SPRITES, 0xFF}, {Layer::HEIGHTMAP, 0x80} };
    m_layer_bufs = { {Layer::BACKGROUND1, std::make_shared<ImageBuffer>()}, {Layer::BACKGROUND2, std::make_shared<ImageBuffer>()},
                     {Layer::BG_SPRITES,  std::make_shared<ImageBuffer>()}, {Layer::FOREGROUND,  std::make_shared<ImageBuffer>()},
                     {Layer::FG_SPRITES,  std::make_shared<ImageBuffer>()}};
}

RoomViewerCtrl::~RoomViewerCtrl()
{
    delete m_bmp;
}

void RoomViewerCtrl::SetGameData(std::shared_ptr<GameData> gd)
{
	m_g = gd;
	Refresh(true);
}

void RoomViewerCtrl::ClearGameData()
{
	m_g = nullptr;
	Refresh(true);
}

void RoomViewerCtrl::SetRoomNum(uint16_t roomnum, Mode mode)
{
	m_roomnum = roomnum;
    m_mode = mode;
    UpdateRoomDescText(m_roomnum);
    RefreshGraphics();
}

void RoomViewerCtrl::SetZoom(double zoom)
{
    m_zoom = zoom;
    if (m_g == nullptr)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
    UpdateBuffer();
    switch (m_mode)
    {
    case Mode::HEIGHTMAP:
        m_layers[Layer::HEIGHTMAP] = DrawHeightmapGrid(map);
        break;
    case Mode::WARPS:
        m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
        if (m_layer_opacity[Layer::HEIGHTMAP] > 0)
        {
            m_layers[Layer::HEIGHTMAP] = DrawHeightmapVisualisation(map, m_layer_opacity[Layer::HEIGHTMAP]);
        }
        break;
    case Mode::NORMAL:
        if (m_layer_opacity[Layer::HEIGHTMAP] > 0)
        {
            m_layers[Layer::HEIGHTMAP] = DrawHeightmapVisualisation(map, m_layer_opacity[Layer::HEIGHTMAP]);
        }
        break;
    }
    UpdateScroll();
    ForceRedraw();
}

void RoomViewerCtrl::SetLayerOpacity(Layer layer, uint8_t opacity)
{
    assert(m_layer_opacity.find(layer) != m_layer_opacity.cend());
    m_layer_opacity[layer] = opacity;
}

uint8_t RoomViewerCtrl::GetLayerOpacity(Layer layer) const
{
    assert(m_layer_opacity.find(layer) != m_layer_opacity.cend());
    return m_layer_opacity.find(layer)->second;
}

void RoomViewerCtrl::RedrawRoom()
{
    DrawRoom(m_roomnum);
    ForceRedraw();
}

void RoomViewerCtrl::DrawRoom(uint16_t roomnum)
{
    if (m_g == nullptr)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    auto blocksets = m_g->GetRoomData()->GetBlocksetsForRoom(roomnum);
    auto palette = std::vector<std::shared_ptr<Palette>>{ m_g->GetRoomData()->GetPaletteForRoom(roomnum)->GetData() };
    auto tileset = m_g->GetRoomData()->GetTilesetForRoom(roomnum)->GetData();
    auto blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);

    m_warp_poly.clear();
    m_link_poly.clear();
    m_layers.clear();
    m_width = map->GetPixelWidth();
    m_height = map->GetPixelHeight();
    UpdateBuffer();

    if (m_layer_opacity[Layer::BACKGROUND1] > 0)
    {
        m_layer_bufs[Layer::BACKGROUND1]->Resize(m_width, m_height);
        m_layer_bufs[Layer::BACKGROUND1]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset);
        m_layers.insert({ Layer::BACKGROUND1, m_layer_bufs[Layer::BACKGROUND1]->MakeBitmap(palette,
            true, m_layer_opacity[Layer::BACKGROUND1]) });
    }
    if (m_layer_opacity[Layer::BACKGROUND2] > 0)
    {
        m_layer_bufs[Layer::BACKGROUND2]->Resize(m_width, m_height);
        m_layer_bufs[Layer::BACKGROUND2]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
        m_layers.insert({ Layer::BACKGROUND2, m_layer_bufs[Layer::BACKGROUND2]->MakeBitmap(palette,
            true, m_layer_opacity[Layer::BACKGROUND2], 0) });
    }
    if (m_layer_opacity[Layer::FOREGROUND] > 0)
    {
        m_layer_bufs[Layer::FOREGROUND]->Resize(m_width, m_height);
        m_layer_bufs[Layer::FOREGROUND]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
        m_layers.insert({ Layer::FOREGROUND, m_layer_bufs[Layer::FOREGROUND]->MakeBitmap(palette,
            true, 0, m_layer_opacity[Layer::FOREGROUND]) });
    }
    if (m_layer_opacity[Layer::HEIGHTMAP] > 0)
    {
        m_layers.insert({ Layer::HEIGHTMAP, DrawHeightmapVisualisation(map, m_layer_opacity[Layer::HEIGHTMAP]) });
    }
}

void RoomViewerCtrl::UpdateRoomDescText(uint16_t roomnum)
{
    if (m_g == nullptr)
    {
        return;
    }
    auto rd = m_g->GetRoomData()->GetRoom(roomnum);
    std::ostringstream ss;
    ss << "Room: " << std::dec << std::uppercase << std::setw(3) << std::setfill('0') << rd->index
        << " Tileset: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->tileset)
        << " Palette: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->room_palette)
        << " PriBlockset: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd->pri_blockset)
        << " SecBlockset: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd->sec_blockset)
        << " BGM: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd->bgm)
        << " Map: " << rd->map;
    m_status_text = ss.str();
    FireEvent(EVT_STATUSBAR_UPDATE);
}

std::shared_ptr<wxBitmap> RoomViewerCtrl::DrawRoomWarps(uint16_t roomnum)
{
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    auto warps = m_g->GetRoomData()->GetWarpsForRoom(roomnum);

    wxImage img(m_buffer_width, m_buffer_height);
    img.InitAlpha();
    SetOpacity(img, 0x00);
    wxGraphicsContext* gc = wxGraphicsContext::Create(img);
    gc->Scale(m_zoom, m_zoom);
    gc->SetPen(*wxWHITE_PEN);
    gc->SetBrush(*wxBLACK_BRUSH);
    for (const auto& warp : warps)
    {
        DrawWarp(*gc, warp, map, TILE_WIDTH, TILE_HEIGHT);
    }
    wxColour bkColor(*wxBLACK);
    wxColour textColor(*wxWHITE);
    int line = 0;
    if (m_g->GetRoomData()->HasClimbDestination(roomnum))
    {
        AddRoomLink(gc, "Climb Destination:", m_g->GetRoomData()->GetClimbDestination(roomnum), 5, 5 + line * 16);
        line++;
    }
    if (m_g->GetRoomData()->HasFallDestination(roomnum))
    {
        AddRoomLink(gc, "Fall Destination:", m_g->GetRoomData()->GetFallDestination(roomnum), 5, 5 + line * 16);
        line++;
    }
    auto txns = m_g->GetRoomData()->GetTransitions(roomnum);
    for (const auto& t : txns)
    {
        std::string label = StrPrintf("Transition when flag %04d is %s:", t.second, (t.first.first == roomnum) ? "SET" : "CLEAR");
        uint16_t dest = (t.first.first == roomnum) ? t.first.second : t.first.first;
        AddRoomLink(gc, label, dest, 5, 5 + line * 16);
        line++;
    }
    line++;
    auto entities = m_g->GetSpriteData()->GetRoomEntities(roomnum);

    wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    gc->SetFont(font, *wxWHITE);
    int count = 1;
    for (const auto& e : entities)
    {
        auto d = e.GetData();
        std::string label = StrPrintf("%01X: %s", count, e.ToString().c_str());
        gc->DrawText(label, 5, 5 + line * 20);
        line+=2;
        count++;
    }
    delete gc;
    return std::make_shared<wxBitmap>(img);
}

void RoomViewerCtrl::DrawRoomHeightmap(uint16_t roomnum)
{
    if (m_g == nullptr)
    {
        return;
    }

    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();

    m_warp_poly.clear();
    m_link_poly.clear();

    m_layers.clear();
    m_width = (map->GetHeightmapWidth() + map->GetHeightmapHeight()) * TILE_WIDTH + 1;
    m_height = (map->GetHeightmapWidth() + map->GetHeightmapHeight()) * TILE_HEIGHT + 1;
    UpdateBuffer();
    m_layers.insert({ Layer::HEIGHTMAP, DrawHeightmapGrid(map) });
}

std::shared_ptr<wxBitmap> RoomViewerCtrl::DrawHeightmapVisualisation(std::shared_ptr<Tilemap3D> map, uint8_t opacity)
{
    wxImage hm_img(m_buffer_width, m_buffer_height);
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

std::shared_ptr<wxBitmap> RoomViewerCtrl::DrawHeightmapGrid(std::shared_ptr<Tilemap3D> map)
{

    wxImage img(m_buffer_width, m_buffer_height);
    wxGraphicsContext* gc = wxGraphicsContext::Create(img);
    gc->SetBrush(*wxBLACK);
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, 0, m_buffer_width, m_buffer_height);
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
                DrawHeightmapCell(*gc, xx, xy, map->GetHeight({x,y}), TILE_WIDTH * 2, TILE_HEIGHT * 2, map->GetCellProps({x,y}), -1, false);
                std::string lbl1 = StrPrintf("%02X", map->GetCellType({ x,y }));
                std::string lbl2 = StrPrintf("%X", map->GetHeight({ x,y }));
                std::string lbl3 = StrPrintf("%X", map->GetCellProps({x,y}));
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

void RoomViewerCtrl::DrawHeightmapCell(wxGraphicsContext& gc, int x, int y, int zz, int width, int height, int restrictions, int classification, bool draw_walls, wxColor border_colour)
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
    switch(restrictions)
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

void RoomViewerCtrl::DrawWarp(wxGraphicsContext& gc, const WarpList::Warp& warp, std::shared_ptr<Tilemap3D> map, int tile_width, int tile_height)
{
    int x = 0;
    int y = 0;
    if (warp.room1 == m_roomnum)
    {
        x = warp.x1;
        y = warp.y1;
    }
    else if (warp.room2 == m_roomnum)
    {
        x = warp.x2;
        y = warp.y2;
    }
    int z = map->GetHeight({ x - 12,y - 12 });
    auto xy = map->Iso3DToPixel({ x,y,z });
    int width = tile_width;
    int height = tile_height;
    std::vector<wxPoint2DDouble> tile_points = {
        wxPoint2DDouble(xy.x + tile_width / 2, xy.y),
        wxPoint2DDouble(xy.x + (warp.x_size + 1) * tile_width / 2, xy.y + warp.x_size * tile_height / 2),
        wxPoint2DDouble(xy.x + (warp.x_size - warp.y_size + 1) * tile_width / 2, xy.y + (warp.x_size + warp.y_size) * tile_height / 2),
        wxPoint2DDouble(xy.x + (1 - warp.y_size) * tile_width / 2, xy.y + warp.y_size * tile_height / 2),
        wxPoint2DDouble(xy.x + tile_width / 2, xy.y)
    };
    switch (warp.type)
    {
    case WarpList::Warp::Type::NORMAL:
        gc.SetPen(*wxYELLOW_PEN);
        break;
    case WarpList::Warp::Type::STAIR_SE:
        gc.SetPen(*wxGREEN_PEN);
        break;
    case WarpList::Warp::Type::STAIR_SW:
        gc.SetPen(*wxCYAN_PEN);
        break;
    default:
        gc.SetPen(*wxRED_PEN);
        break;
    }
    gc.SetBrush(*wxTRANSPARENT_BRUSH);
    gc.DrawLines(tile_points.size(), &tile_points[0]);
    m_warp_poly.push_back({ warp, tile_points });
}



void RoomViewerCtrl::AddRoomLink(wxGraphicsContext* gc, const std::string& label, uint16_t room, int x, int y)
{
    wxFont font(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    double w, h;
    gc->SetFont(font, *wxWHITE);
    gc->GetTextExtent(label, &w, &h);
    const double LINK_X = x + std::max<double>(145, w + 20);
    const double LINK_Y = y;
    gc->DrawText(label, x, y);
    gc->SetFont(font, *wxYELLOW);
    auto roomlabel = StrPrintf("Room %03d", room);
    gc->DrawText(roomlabel, LINK_X, LINK_Y);
    gc->GetTextExtent(roomlabel, &w, &h);
    m_link_poly.push_back({ room, {
        {LINK_X, LINK_Y},
        {LINK_X + w, LINK_Y},
        {LINK_X + w, LINK_Y + h},
        {LINK_X, LINK_Y + h},
        {LINK_X, LINK_Y}
    } });
}

void RoomViewerCtrl::SetOpacity(wxImage& image, uint8_t opacity)
{
    uint8_t* alpha = image.GetAlpha();
    for (int i = 0; i < (image.GetHeight() * image.GetWidth()); i++)
    {
        *alpha = (*alpha < opacity) ? *alpha : opacity;
        alpha++;
    }
}

void RoomViewerCtrl::ForceRedraw()
{
    m_redraw = true;
    m_repaint = true;
    Refresh(true);
}

void RoomViewerCtrl::ForceRepaint()
{
    m_repaint = true;
    Refresh(true);
}

void RoomViewerCtrl::UpdateScroll()
{
    m_scroll_rate = std::ceil(SCROLL_RATE * m_zoom);
    SetVirtualSize(std::ceil(m_zoom * m_width), std::ceil(m_zoom * m_height));
    SetScrollRate(m_scroll_rate, m_scroll_rate);
    AdjustScrollbars();
}

void RoomViewerCtrl::UpdateBuffer()
{
    m_buffer_width = std::ceil(m_width * m_zoom);
    m_buffer_height = std::ceil(m_height * m_zoom);
    m_bmp->Create(m_buffer_width, m_buffer_height);
}

void RoomViewerCtrl::RefreshGraphics()
{
    if (m_g == nullptr)
    {
        return;
    }
    switch (m_mode)
    {
    case Mode::HEIGHTMAP:
        DrawRoomHeightmap(m_roomnum);
        break;
    case Mode::WARPS:
        DrawRoom(m_roomnum);
        m_layers[Layer::WARPS] = DrawRoomWarps(m_roomnum);
        break;
    case Mode::NORMAL:
        DrawRoom(m_roomnum);
        break;
    }
    UpdateScroll();
    ForceRedraw();
}

const std::string& RoomViewerCtrl::GetStatusText() const
{
    return m_status_text;
}


bool RoomViewerCtrl::Pnpoly(const std::vector<wxPoint2DDouble>& poly, int x, int y)
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

void RoomViewerCtrl::GoToRoom(uint16_t room)
{
    auto name = m_g->GetRoomData()->GetRoom(room)->name;
    FireEvent(EVT_GO_TO_NAV_ITEM, "Rooms/" + name + "/Warps");
}

void RoomViewerCtrl::FireEvent(const wxEventType& e, long userdata)
{
    wxCommandEvent evt(e);
    evt.SetExtraLong(userdata);
    evt.SetClientData(this->GetParent());
    wxPostEvent(this->GetParent(), evt);
}

void RoomViewerCtrl::FireEvent(const wxEventType& e, const std::string& userdata)
{
    wxCommandEvent evt(e);
    evt.SetString(userdata);
    evt.SetClientData(this->GetParent());
    wxPostEvent(this->GetParent(), evt);
}

void RoomViewerCtrl::FireEvent(const wxEventType& e)
{
    wxCommandEvent evt(e);
    evt.SetClientData(this->GetParent());
    wxPostEvent(this->GetParent(), evt);
}

void RoomViewerCtrl::OnDraw(wxDC& dc)
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
        mdc.SetBackground(*wxBLACK_BRUSH);
        mdc.Clear();
        for (const auto& layer : m_layers)
        {
            if (layer.first == Layer::HEIGHTMAP || layer.first == Layer::WARPS)
            {
                mdc.SetUserScale(1.0, 1.0);
            }
            else
            {
                mdc.SetUserScale(m_zoom, m_zoom);
            }
            mdc.DrawBitmap(*layer.second, 0, 0, true);
        }
        m_redraw = false;
    }
    mdc.SetUserScale(1.0, 1.0);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    dc.Blit(sx, sy, sw, sh, &mdc, sx, sy, wxCOPY);
    mdc.SelectObject(wxNullBitmap);
}

void RoomViewerCtrl::OnPaint(wxPaintEvent& evt)
{
	wxBufferedPaintDC dc(this);
	this->PrepareDC(dc);
	this->OnDraw(dc);
}

void RoomViewerCtrl::OnEraseBackground(wxEraseEvent& evt)
{
    //evt.Skip();
}

void RoomViewerCtrl::OnSize(wxSizeEvent& evt)
{
	Refresh(false);
	evt.Skip();
}

void RoomViewerCtrl::OnMouseMove(wxMouseEvent& evt)
{
    if (m_g != nullptr && m_mode == Mode::WARPS)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            int x, y;
            GetViewStart(&x, &y);
            x *= m_scroll_rate;
            y *= m_scroll_rate;
            x += evt.GetX();
            y += evt.GetY();
            x = std::ceil(x / m_zoom);
            y = std::ceil(y / m_zoom);
            auto r = map->PixelToHMPoint({ x,y });
            auto status_text = StrPrintf("(%d,%d) [%d,%d]", x, y, r.x, r.y);
            SetCursor(wxCURSOR_ARROW);
            for (const auto& wp : m_warp_poly)
            {
                if (Pnpoly(wp.second, x, y))
                {
                    uint16_t room = (wp.first.room1 == m_roomnum) ? wp.first.room2 : wp.first.room1;
                    uint8_t wx = (wp.first.room1 == m_roomnum) ? wp.first.x2 : wp.first.x1;
                    uint8_t wy = (wp.first.room1 == m_roomnum) ? wp.first.y2 : wp.first.y1;
                    status_text += StrPrintf(" - Warp to room %03d (%d,%d)", room, wx, wy);
                    SetCursor(wxCURSOR_HAND);
                    break;
                }
            }
            for (const auto& lp : m_link_poly)
            {
                if (Pnpoly(lp.second, x, y))
                {
                    SetCursor(wxCURSOR_HAND);
                    break;
                }
            }
            if (status_text != m_status_text)
            {
                m_status_text = status_text;
                FireEvent(EVT_STATUSBAR_UPDATE);
            }
        }
    }
    evt.Skip();
}

void RoomViewerCtrl::OnMouseLeave(wxMouseEvent& evt)
{
    SetCursor(wxCURSOR_ARROW);
    evt.Skip();
}

void RoomViewerCtrl::OnLeftClick(wxMouseEvent& evt)
{
    SetCursor(wxCURSOR_ARROW);
    if (m_g != nullptr && m_mode == Mode::WARPS)
    {
        auto map = m_g->GetRoomData()->GetMapForRoom(m_roomnum)->GetData();
        if (map)
        {
            int x, y;
            GetViewStart(&x, &y);
            x *= m_scroll_rate;
            y *= m_scroll_rate;
            x += evt.GetX();
            y += evt.GetY();
            x = std::ceil(x / m_zoom);
            y = std::ceil(y / m_zoom);
            for (const auto& wp : m_warp_poly)
            {
                if (Pnpoly(wp.second, x, y))
                {
                    uint16_t room = (wp.first.room1 == m_roomnum) ? wp.first.room2 : wp.first.room1;
                    GoToRoom(room);
                    break;
                }
            }
            for (const auto& lp : m_link_poly)
            {
                if (Pnpoly(lp.second, x, y))
                {
                    GoToRoom(lp.first);
                    break;
                }
            }
        }
    }
    evt.Skip();
}
