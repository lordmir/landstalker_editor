#include "RoomViewerCtrl.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

wxBEGIN_EVENT_TABLE(RoomViewerCtrl, wxScrolledCanvas)
EVT_ERASE_BACKGROUND(RoomViewerCtrl::OnEraseBackground)
EVT_PAINT(RoomViewerCtrl::OnPaint)
EVT_SIZE(RoomViewerCtrl::OnSize)
wxEND_EVENT_TABLE()

RoomViewerCtrl::RoomViewerCtrl(wxWindow* parent)
	: wxScrolledCanvas(parent, wxID_ANY),
	  m_roomnum(0),
      m_bmp(new wxBitmap)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
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
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    delete m_bmp;
    DrawRoom(m_roomnum);
    m_bmp = new wxBitmap(map->GetPixelWidth(), map->GetPixelHeight());
    SetScrollbars(8, 8, map->GetPixelWidth()/8, map->GetPixelHeight()/8, 0, 0);
    ForceRepaint();
}

void RoomViewerCtrl::DrawRoom(uint16_t roomnum)
{
    if (m_g == nullptr)
    {
        return;
    }
    auto map = m_g->GetRoomData()->GetMapForRoom(roomnum)->GetData();
    auto blocksets = m_g->GetRoomData()->GetBlocksetsForRoom(roomnum);
    auto palette = m_g->GetRoomData()->GetPaletteForRoom(roomnum)->GetData();
    auto tileset = m_g->GetRoomData()->GetTilesetForRoom(roomnum)->GetData();
    auto blockset = m_g->GetRoomData()->GetCombinedBlocksetForRoom(roomnum);

    m_warp_poly.clear();
    m_link_poly.clear();
    m_layers.clear();

    ImageBuffer m_imgbuf(map->GetPixelWidth(), map->GetPixelHeight());
    if (m_layer_opacity[Layer::BACKGROUND1] > 0)
    {
        m_layer_bufs[Layer::BACKGROUND1]->Resize(map->GetPixelWidth(), map->GetPixelHeight());
        m_layer_bufs[Layer::BACKGROUND1]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::BG, map, tileset, blockset);
        m_layers.insert({ Layer::BACKGROUND1, m_layer_bufs[Layer::BACKGROUND1]->MakeBitmap({ palette },
            true, m_layer_opacity[Layer::BACKGROUND1]) });
        m_imgbuf.Clear();
    }
    if (m_layer_opacity[Layer::BACKGROUND2] > 0)
    {
        m_layer_bufs[Layer::BACKGROUND2]->Resize(map->GetPixelWidth(), map->GetPixelHeight());
        m_layer_bufs[Layer::BACKGROUND2]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
        m_layers.insert({ Layer::BACKGROUND2, m_layer_bufs[Layer::BACKGROUND2]->MakeBitmap({ palette },
            true, 0, m_layer_opacity[Layer::BACKGROUND2]) });
        m_imgbuf.Clear();
    }
    if (m_layer_opacity[Layer::FOREGROUND] > 0)
    {
        m_layer_bufs[Layer::FOREGROUND]->Resize(map->GetPixelWidth(), map->GetPixelHeight());
        m_layer_bufs[Layer::FOREGROUND]->Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
        m_imgbuf.Insert3DMapLayer(0, 0, 0, Tilemap3D::Layer::FG, map, tileset, blockset);
        m_layers.insert({ Layer::FOREGROUND, m_layer_bufs[Layer::FOREGROUND]->MakeBitmap({ palette },
            true, m_layer_opacity[Layer::FOREGROUND], 0) });
        m_imgbuf.Clear();
    }
    if (m_layer_opacity[Layer::HEIGHTMAP] > 0)
    {
        m_layers.insert({ Layer::HEIGHTMAP, DrawHeightmapVisualisation(map, m_layer_opacity[Layer::HEIGHTMAP]) });
    }
}

std::shared_ptr<wxBitmap> RoomViewerCtrl::DrawHeightmapVisualisation(std::shared_ptr<Tilemap3D> map, uint8_t opacity)
{
    wxImage hm_img(map->GetPixelWidth(), map->GetPixelHeight());
    hm_img.InitAlpha();
    SetOpacity(hm_img, 0x00);
    wxGraphicsContext* hm_gc = wxGraphicsContext::Create(hm_img);
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

void RoomViewerCtrl::DrawHeightmapCell(wxGraphicsContext& gc, int x, int y, int z, int width, int height, int restrictions, int classification)
{
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
    case 0x0:
        bg.Set(z * 3, 48 + z * 8, z * 3);
        break;
    case 0x4:
        bg.Set(48 + z * 8, z * 3, z * 3);
        break;
    case 0x2:
        bg.Set(32 + z * 3, 32 + z * 3, 48 + z * 12);
        break;
    case 0x6:
        bg.Set(48 + z * 8, 32 + z * 3, 48 + z * 8);
        break;
    default:
        bg.Set(48 + z * 8, 48 + z * 8, z * 3);
        break;
    }
    gc.SetBrush(wxBrush(bg));
    gc.SetPen(*wxTRANSPARENT_PEN);
    //gc.SetTextForeground(*wxWHITE);
    gc.DrawLines(sizeof(tile_points) / sizeof(tile_points[0]), tile_points);
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(left_wall) / sizeof(left_wall[0]), left_wall);
    bg = bg.ChangeLightness(70);
    gc.SetBrush(wxBrush(bg));
    gc.DrawLines(sizeof(right_wall) / sizeof(right_wall[0]), right_wall);
    if (classification != 0)
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

void RoomViewerCtrl::ForceRepaint()
{
    m_redraw = true;
    Refresh(true);
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

void RoomViewerCtrl::OnDraw(wxDC& dc)
{
    int sx, sy;
    GetViewStart(&sx, &sy);
    sx *= SCROLL_RATE;
    sy *= SCROLL_RATE;
    int sw, sh;
    GetClientSize(&sw, &sh);
    wxMemoryDC mdc(*m_bmp);
    if (m_redraw)
    {
        mdc.SetBackground(*wxBLACK_BRUSH);
        mdc.Clear();
        for (const auto& layer : m_layers)
        {
            mdc.DrawBitmap(*layer.second, 0, 0, true);
        }
        dc.SetBackground(*wxBLACK_BRUSH);
        dc.Clear();
        dc.Blit(sx, sy, sw, sh, &mdc, sx, sy, wxCOPY);
        m_redraw = false;
    }
    else
    {
        int x, y, w, h;                 // Dimensions of client area in pixels
        wxRegionIterator upd(GetUpdateRegion()); // get the update rect list
        while (upd)
        {
            x = upd.GetX();
            y = upd.GetY();
            w = upd.GetW();
            h = upd.GetH();
            dc.Blit(x + sx, y + sy, w, h, &mdc, x + sx, y + sy);
            upd++;
        }
    }
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
}

void RoomViewerCtrl::OnSize(wxSizeEvent& evt)
{
	Refresh(false);
	evt.Skip();
}
