#include "MainFrame.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <set>

#include <wx/wx.h>
#include <wx/aboutdlg.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/colour.h>
#include <wx/graphics.h>

#include "LZ77.h"
#include "BigTilesCmp.h"
#include "LSTilemapCmp.h"
#include "Rom.h"
#include "ImageBuffer.h"
#include "SpriteFrame.h"
#include "Utils.h"
#include "Tilemap2D.h"
#include "Blockmap2D.h"

MainFrame::MainFrame(wxWindow* parent, const std::string& filename)
    : MainFrameBaseClass(parent),
      m_gfxSize(0),
      m_scale(1),
      m_rpalidx(0),
      m_tsidx(0),
      m_bs1(0),
      m_bs2(0),
      m_roomnum(0),
      m_sprite_idx(0),
      m_sprite_anim(0),
      m_sprite_frame(0),
      m_mode(MODE_NONE),
      m_layer_controls_enabled(false)
{
    m_imgs = new ImgLst();
    if (!filename.empty())
    {
        OpenRomFile(filename.c_str());
    }
}

MainFrame::~MainFrame()
{
    delete m_imgs;
}

void MainFrame::OnExit(wxCommandEvent& event)
{
    wxUnusedVar(event);
    Close();
    event.Skip();
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxUnusedVar(event);
    wxAboutDialogInfo info;
    info.SetCopyright(_("Landstalker Graphics Viewer"));
    info.SetLicence(_("GPL v2 or later"));
    info.SetDescription(_("Github: www.github.com/thomasamendt/landstalker_gfx\nEmail: tgamendt@gmail.com"));
    ::wxAboutBox(info);
    event.Skip();
}

void MainFrame::OpenRomFile(const wxString& path)
{
    try
    {
        m_rom.load_from_file(static_cast<std::string>(path));

        m_tilesetOffsets = m_rom.read_array<uint32_t>(0x44070, 31);
        m_browser->DeleteAllItems();
        m_browser->SetImageList(m_imgs);
        wxTreeItemId nodeRoot = m_browser->AddRoot("");
        wxTreeItemId nodeTs = m_browser->AppendItem(nodeRoot, "Tilesets", 1, 1, new TreeNodeData());
        wxTreeItemId nodeATs = m_browser->AppendItem(nodeRoot, "Animated Tilesets", 1, 1, new TreeNodeData());
        wxTreeItemId nodeBTs = m_browser->AppendItem(nodeRoot, "Big Tilesets", 3, 3, new TreeNodeData());
        wxTreeItemId nodeRPal = m_browser->AppendItem(nodeRoot, "Room Palettes", 2, 2, new TreeNodeData());
        wxTreeItemId nodeRm = m_browser->AppendItem(nodeRoot, "Rooms", 0, 0, new TreeNodeData());
        wxTreeItemId nodeSprites = m_browser->AppendItem(nodeRoot, "Sprites", 4, 4, new TreeNodeData());

        const uint32_t start_of_sprite_graphics = 0x120000;
        const uint32_t start_of_sprite_table = start_of_sprite_graphics + 4;
        const uint32_t start_of_anim_table = m_rom.read<uint32_t>(start_of_sprite_graphics);
        const uint32_t start_of_frame_table = m_rom.read<uint32_t>(start_of_anim_table);
        const uint32_t start_of_frames = m_rom.read<uint32_t>(start_of_frame_table);

        std::set<uint32_t> frame_offsets;
        for (uint32_t frame_offset = start_of_frame_table; frame_offset < start_of_frames; frame_offset += 4)
        {
            frame_offsets.insert(m_rom.read<uint32_t>(frame_offset));
        }

        std::vector<uint32_t> frames(frame_offsets.cbegin(), frame_offsets.cend());
        std::map<uint32_t, uint32_t> frame_offset_to_frame_num;
        for (size_t i = 0; i < frames.size(); ++i)
        {
            frame_offset_to_frame_num[frames[i]] = i;
            m_spriteFrames.emplace_back(SpriteFrame(m_rom.data(frames[i])));
        }

        std::vector<uint32_t> sprite_frames;
        size_t i = 0;
        for (uint32_t soffset = start_of_sprite_table; soffset < start_of_anim_table; soffset += 4)
        {
            m_spriteGraphics.emplace_back(i++);
            uint32_t start_anim_offset = m_rom.read<uint16_t>(soffset) * 4 + start_of_anim_table;
            uint32_t end_anim_offset;
            if (soffset + 4 >= start_of_anim_table)
            {
                end_anim_offset = start_of_frame_table;
            }
            else
            {
                end_anim_offset = m_rom.read<uint16_t>(soffset + 4) * 4 + start_of_anim_table;
            }
            for (uint32_t aoffset = start_anim_offset; aoffset < end_anim_offset; aoffset += 4)
            {
                uint32_t start_frame_offset = m_rom.read<uint32_t>(aoffset);
                uint32_t end_frame_offset;
                if (aoffset + 4 >= start_of_frames)
                {
                    end_frame_offset = start_of_frames;
                }
                else
                {
                    end_frame_offset = m_rom.read<uint32_t>(aoffset + 4);
                }
                std::vector<uint32_t> sframes = m_rom.read_array<uint32_t>(start_frame_offset, (end_frame_offset - start_frame_offset)/4);
                for(auto& frame : sframes)
                {
                    frame = frame_offset_to_frame_num[frame];
                }
                m_spriteGraphics.back().AddAnimation(sframes);
            }
        }

        for (size_t i = 0; i < (236 * 2); i+=2)
        {
            uint8_t sprite_idx = m_rom.read<uint8_t>(0x1ABF2 + i + 1);
            uint8_t sprite_gfx = m_rom.read<uint8_t>(0x1ABF2 + i);
            m_sprites.emplace(sprite_idx, Sprite(sprite_gfx));
        }

        for (size_t offset = 0x1A453A; m_rom.read<uint8_t>(offset) != 0xFF; offset += 2)
        {
            if ((m_rom.read<uint8_t>(offset + 1) & 0x80) > 0)
            {
                m_sprites[m_rom.read<uint8_t>(offset)].SetHighPalette(m_rom.read<uint8_t>(offset + 1) & 0x7F);
            }
            else
            {
                m_sprites[m_rom.read<uint8_t>(offset)].SetLowPalette(m_rom.read<uint8_t>(offset + 1));
            }
        }

        for (const auto& sprite : m_sprites)
        {
            const auto& sg = m_spriteGraphics[sprite.second.GetGraphicsIdx()];
            size_t default_anim = sg.GetAnimationCount() > 1 ? 1 : 0;
            auto spr = m_browser->AppendItem(nodeSprites, Hex(sprite.first), 4, 4, new TreeNodeData(TreeNodeData::NODE_SPRITE, default_anim << 16 | sprite.first));

            for (size_t a = 0; a != sg.GetAnimationCount(); ++a)
            {
                std::ostringstream ss;
                ss.str(std::string());
                ss << "ANIM" << a;
                wxTreeItemId anim = m_browser->AppendItem(spr, ss.str(), 4, 4, new TreeNodeData(TreeNodeData::NODE_SPRITE, a << 16 | sprite.first));
                for (size_t f = 0; f != sg.GetFrameCount(a); ++f)
                {
                    ss.str(std::string());
                    ss << "FRAME" << f;
                    m_browser->AppendItem(anim, ss.str(), 4, 4, new TreeNodeData(TreeNodeData::NODE_SPRITE, a << 16 | f << 8 | sprite.first));
                }
            }
        }

        for (size_t i = 0; i < m_tilesetOffsets.size(); ++i)
        {
            m_browser->AppendItem(nodeTs, Hex(m_tilesetOffsets[i]), 1, 1, new TreeNodeData(TreeNodeData::NODE_TILESET, i));
        }
        auto bt = m_rom.read_array<uint32_t>(m_rom.read<uint32_t>(0x1AF800), 64);
        for (size_t i = 0; i < 64; ++i)
        {
            m_bigTileOffsets.push_back(m_rom.read_array<uint32_t>(bt[i], 9));
            wxTreeItemId curTn = m_browser->AppendItem(nodeBTs, Hex(bt[i]), 3, 3, new TreeNodeData(TreeNodeData::NODE_BIG_TILES, i << 16));
            for (size_t j = 0; j < 9; ++j)
            {
                m_browser->AppendItem(curTn, Hex(m_bigTileOffsets[i][j]), 3, 3, new TreeNodeData(TreeNodeData::NODE_BIG_TILES, i << 16 | j));
            }
        }
        const uint8_t* rm = m_rom.data(m_rom.read<uint32_t>(0xA0A00));
        for (size_t i = 0; i < 816; i++)
        {
            std::ostringstream ss;
            m_rooms.push_back(RoomData(rm));
            rm += 8;
            ss << i;
            wxTreeItemId cRm = m_browser->AppendItem(nodeRm, ss.str(), 0, 0, new TreeNodeData(TreeNodeData::NODE_ROOM, i));
            m_browser->AppendItem(cRm, "Heightmap", 0, 0, new TreeNodeData(TreeNodeData::NODE_ROOM_HEIGHTMAP, i));
        }
        InitPals(nodeRPal);
    }
    catch(const std::runtime_error& e)
    {
        m_browser->DeleteAllItems();
        wxMessageBox(e.what());
    }
    SetMode(MODE_NONE);
}

void MainFrame::DrawBigTiles(size_t row_width, size_t scale, uint8_t pal)
{
    const size_t ROW_WIDTH = std::min<size_t>(16U, m_bigTiles.size());
    const size_t ROW_HEIGHT = std::min<size_t>(128U, m_bigTiles.size() / ROW_WIDTH + (m_bigTiles.size() % ROW_WIDTH != 0));
    Blockmap2D map(ROW_WIDTH, ROW_HEIGHT, 0, 0, 0);
    m_imgbuf.Resize(map.GetBitmapWidth(), map.GetBitmapHeight());
    map.SetTileset(std::make_shared<Tileset>(m_tilebmps));
    map.SetBlockset(std::make_shared<std::vector<BigTile>>(m_bigTiles));
    map.Fill(0, 1);
    map.Draw(m_imgbuf);
    m_scale = scale;
    bmp = m_imgbuf.MakeBitmap(m_palette);
    ForceRepaint();
}

void DrawTile(wxGraphicsContext& gc, int x, int y, int z, int width, int height, int restrictions, int classification)
{
    wxPoint2DDouble tile_points[] = {
        wxPoint2DDouble(x + width / 2, y             ),
        wxPoint2DDouble(x + width    , y + height / 2),
        wxPoint2DDouble(x + width / 2, y + height    ),
        wxPoint2DDouble(x            , y + height / 2),
        wxPoint2DDouble(x + width / 2, y             )
    };
    wxPoint2DDouble left_wall[] = {
        wxPoint2DDouble(x            , y + height / 2             ),
        wxPoint2DDouble(x + width / 2, y + height                 ),
        wxPoint2DDouble(x + width / 2, y + height     + z * height),
        wxPoint2DDouble(x            , y + height / 2 + z * height),
        wxPoint2DDouble(x            , y + height / 2             )
    };
    wxPoint2DDouble right_wall[] = {
        wxPoint2DDouble(x + width / 2, y + height                 ),
        wxPoint2DDouble(x + width    , y + height / 2             ),
        wxPoint2DDouble(x + width    , y + height / 2 + z * height),
        wxPoint2DDouble(x + width / 2, y + height     + z * height),
        wxPoint2DDouble(x + width / 2, y + height                 )
    };
    wxColor bg;
    if (restrictions == 0)
    {
        bg.Set(z * 3, 48 + z * 8, z*3);
    }
    else if (restrictions == 0x4)
    {
        bg.Set(48 + z * 8, z*3, z*3);
    }
    else if (restrictions == 0x2)
    {
        bg.Set(32 + z * 3, 32 + z * 3, 48 + z * 12);
    }
    else if (restrictions == 0x6)
    {
        bg.Set(48 + z * 8, 32 + z * 3, 48 + z * 8);
    }
    else
    {
        bg.Set(48 + z * 8, 48 + z * 8, z * 3);
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
        gc.SetFont(wxFont(height/2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
           wxFONTWEIGHT_NORMAL, false),
           wxColour(255, 255, 255, 255));
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << classification;
        double textwidth, textheight;
        gc.GetTextExtent(ss.str(), &textwidth, &textheight);
        gc.DrawText(ss.str(), x + (width - textwidth)/2, y + (height - textheight)/2 );
    }
}

void SetOpacity(wxImage& image, uint8_t opacity)
{
    uint8_t* alpha = image.GetAlpha();
    for (int i = 0; i < (image.GetHeight() * image.GetWidth()); i++)
    {
        *alpha = (*alpha < opacity) ? *alpha : opacity;
        alpha++;
    }
}

void MainFrame::DrawTilemap(size_t scale, uint8_t pal)
{
    const size_t TILE_WIDTH = 32;
    const size_t TILE_HEIGHT = 16;

    uint8_t bg_opacity  = m_checkBgVisible->GetValue() ? m_sliderBgOpacity->GetValue() : 0;
    uint8_t fg1_opacity = m_checkFg1Visible->GetValue() ? m_sliderFg1Opacity->GetValue() : 0;
    uint8_t fg2_opacity = m_checkFg2Visible->GetValue() ? m_sliderFg2Opacity->GetValue() : 0;
    uint8_t hm_opacity  = m_checkHeightmapVisible->GetValue() ? m_sliderHeightmapOpacity->GetValue() : 0;
    uint8_t spr_opacity = m_checkSpritesVisible->GetValue() ? m_sliderSpritesOpacity->GetValue() : 0;

    m_imgbuf.Resize(m_tilemap.background.GetBitmapWidth(), m_tilemap.background.GetBitmapHeight());
    ImageBuffer fg(m_tilemap.background.GetBitmapWidth(), m_tilemap.background.GetBitmapHeight());
    m_tilemap.background.SetTileset(std::make_shared<Tileset>(m_tilebmps));
    m_tilemap.foreground.SetTileset(std::make_shared<Tileset>(m_tilebmps));
    m_tilemap.background.SetBlockset(std::make_shared<std::vector<BigTile>>(m_bigTiles));
    m_tilemap.foreground.SetBlockset(std::make_shared<std::vector<BigTile>>(m_bigTiles));
    m_tilemap.background.Draw(m_imgbuf);
    m_tilemap.foreground.Draw(fg);
    m_scale = scale;
    std::shared_ptr<wxBitmap> bg_bmp(m_imgbuf.MakeBitmap(m_palette, true, bg_opacity));
    std::shared_ptr<wxBitmap> fg_bmp(fg.MakeBitmap(m_palette, true, fg1_opacity, fg2_opacity));
    wxImage hm_img(fg.GetWidth(), fg.GetHeight());
    wxImage disp_img(fg.GetWidth(), fg.GetHeight());
    hm_img.InitAlpha();
    SetOpacity(hm_img, 0x00);
    wxGraphicsContext* hm_gc = wxGraphicsContext::Create(hm_img);
    hm_gc->SetPen(*wxWHITE_PEN);
    hm_gc->SetBrush(*wxBLACK_BRUSH);
    size_t p = 0;
    for (size_t y = 0; y < m_tilemap.hmheight; ++y)
        for (size_t x = 0; x < m_tilemap.hmwidth; ++x)
        {
            // Only display cells that are not completely restricted
            if ((m_tilemap.heightmap[p].height > 0) || (m_tilemap.heightmap[p].restrictions != 0x04))
            {
                size_t xx = x - m_tilemap.GetLeft() + 12;
                size_t yy = y - m_tilemap.GetTop() + 12;
                size_t zz = m_tilemap.heightmap[p].height;
                wxPoint xy(m_tilemap.foreground.ToXYPoint3D(TilePoint3D{ xx, yy, zz }));
                DrawTile(*hm_gc, xy.x, xy.y, zz, TILE_WIDTH, TILE_HEIGHT, m_tilemap.heightmap[p].restrictions, m_tilemap.heightmap[p].classification);
            }
            p++;
        }
    delete hm_gc;
    SetOpacity(hm_img, hm_opacity);
    wxBitmap hm_bmp(hm_img);
    //hm_bmp.UseAlpha();
    wxGraphicsContext* disp_gc = wxGraphicsContext::Create(disp_img);
    disp_gc->DrawBitmap(*bg_bmp, 0, 0, bg_bmp->GetWidth(), bg_bmp->GetHeight());
    disp_gc->DrawBitmap(*fg_bmp, 0, 0, fg_bmp->GetWidth(), fg_bmp->GetHeight());
    disp_gc->DrawBitmap(hm_bmp, 0, 0, hm_bmp.GetWidth(), hm_bmp.GetHeight());
    delete disp_gc;
    bmp = std::make_shared<wxBitmap>(disp_img);
    memDc.SelectObject(*bmp);
    ForceRepaint();
}

void MainFrame::DrawHeightmap(size_t scale, uint16_t room)
{
    const size_t TILE_WIDTH = 32;
    const size_t TILE_HEIGHT = 32;
    const size_t ROW_WIDTH = m_tilemap.hmwidth;
    const size_t ROW_HEIGHT = m_tilemap.hmheight;
    const size_t BMP_WIDTH = ROW_WIDTH * TILE_WIDTH + 1;
    const size_t BMP_HEIGHT = ROW_HEIGHT * TILE_WIDTH + 1;

    //size_t x = 0;
    //size_t y = 0;
    bmp = std::make_shared<wxBitmap>(BMP_WIDTH, BMP_HEIGHT);
    memDc.SelectObject(*bmp);
    memDc.SetBackground(*wxBLACK_BRUSH);
    memDc.Clear();
    memDc.SetPen(*wxWHITE_PEN);
    memDc.SetBrush(*wxBLACK_BRUSH);
    memDc.SetTextBackground(*wxBLACK);
    memDc.SetTextForeground(*wxWHITE);

    size_t p = 0;
    for(size_t y = 0; y < ROW_HEIGHT; ++y)
    for(size_t x = 0; x < ROW_WIDTH; ++x)
    {
        // Only display cells that are not completely restricted
        if((m_tilemap.heightmap[p].height > 0) || (m_tilemap.heightmap[p].restrictions != 0x04))
        {
            wxPoint xy(m_tilemap.foreground.ToXYPoint(TilePoint{ x, y }));
            memDc.DrawRectangle(x * TILE_WIDTH, y*TILE_HEIGHT, TILE_WIDTH+1, TILE_HEIGHT+1);
            std::stringstream ss;
            ss << std::hex << std::uppercase << std::setfill('0') << std::setw(1) << static_cast<unsigned>(m_tilemap.heightmap[p].height) << ","
            << std::setfill('0') << std::setw(1) << static_cast<unsigned>(m_tilemap.heightmap[p].restrictions) << "\n"
            << std::setfill('0') << std::setw(2) << static_cast<unsigned>(m_tilemap.heightmap[p].classification);
            memDc.DrawText(ss.str(),x*TILE_WIDTH+2, y*TILE_HEIGHT + 1);
        }
        p++;
    }
    memDc.SelectObject(wxNullBitmap);

    m_scale = scale;
    m_scrollwindow->SetScrollbars(scale,scale,BMP_WIDTH,BMP_HEIGHT,0,0);
    wxClientDC dc(m_scrollwindow);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, scale);
}

void MainFrame::DrawTiles(size_t row_width, size_t scale, uint8_t pal)
{
    const size_t ROW_WIDTH = std::min<size_t>(16UL, m_tilebmps.size());
    const size_t ROW_HEIGHT = std::min<size_t>(128UL, m_tilebmps.size() / ROW_WIDTH + (m_tilebmps.size() % ROW_WIDTH != 0));
    Tilemap2D map(ROW_WIDTH, ROW_HEIGHT, 0, 0, 0);
    m_imgbuf.Resize(map.GetBitmapWidth(), map.GetBitmapHeight());
    map.SetTileset(std::make_shared<Tileset>(m_tilebmps));
    map.Fill(0, 1);
    map.Draw(m_imgbuf);
    m_scale = scale;
    bmp = m_imgbuf.MakeBitmap(m_palette);
    ForceRepaint();
}

void MainFrame::DrawSprite(const SpriteFrame& sprite, uint8_t pal_idx, size_t scale)
{
    size_t top = 0xFFFF;
    size_t left = 0xFFFF;
    size_t bottom = 0;
    size_t right = 0;

    for (const auto& subs : sprite.m_subsprites)
    {
        left   = std::min(left,    (subs.x + 0x80) & 0xFF);
        top    = std::min(top,     (subs.y + 0x80) & 0xFF);
        right  = std::max(right,  ((subs.x + 0x80) & 0xFF) + subs.w * 8);
        bottom = std::max(bottom, ((subs.y + 0x80) & 0xFF) + subs.h * 8);
    }

    m_imgbuf.Resize(right - left, bottom - top);

    for (const auto& subs : sprite.m_subsprites)
    {
        size_t index = subs.tile_idx;
        for (size_t x = 0; x < subs.w; ++x)
        for (size_t y = 0; y < subs.h; ++y)
        {
            size_t xx = ((subs.x + 0x80) & 0xFF) - left + x * 8;
            size_t yy = ((subs.y + 0x80) & 0xFF) - top + y * 8;
            m_imgbuf.InsertTile(xx, yy, pal_idx, Tile(index++), sprite.m_sprite_gfx);
        }
    }
    m_scale = scale;
    bmp = m_imgbuf.MakeBitmap(m_palette);
    ForceRepaint();
}

void MainFrame::ForceRepaint()
{
    m_scrollwindow->SetScrollbars(m_scale, m_scale, m_imgbuf.GetWidth(), m_imgbuf.GetHeight(), 0, 0);
    wxClientDC dc(m_scrollwindow);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, m_scale);
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
    event.Skip();
}

void MainFrame::PaintNow(wxDC& dc, size_t scale)
{
    int x, y, w, h;
    m_scrollwindow->GetViewStart(&x, &y);
    m_scrollwindow->GetClientSize(&w, &h);
    double dscale = static_cast<double>(scale);
    memDc.SelectObject(wxNullBitmap);
    if (bmp != nullptr)
    {
        memDc.SelectObject(*bmp);
    }
    dc.SetUserScale(dscale, dscale);
    dc.Blit(0, 0, w/dscale+1, h/dscale+1, &memDc, x, y, wxCOPY, true);
    memDc.SelectObject(wxNullBitmap);
}

void MainFrame::OnScrollWindowPaint(wxPaintEvent& event)
{
    wxPaintDC dc(m_scrollwindow);
    PaintNow(dc, m_scale);
    event.Skip();
}

void MainFrame::LoadTileset(size_t offset)
{
    std::memset(m_gfxBuffer, 0x00, sizeof(m_gfxBuffer));
    size_t elen = 0;
    m_gfxSize = LZ77::Decode(m_rom.data(offset), sizeof(m_gfxBuffer), m_gfxBuffer, elen);
    m_tilebmps.setBits(m_gfxBuffer, 0x400);
}

void MainFrame::LoadBigTiles(size_t offset)
{
    BigTilesCmp::Decode(m_rom.data(offset), m_bigTiles);
}

void MainFrame::LoadTilemap(size_t offset)
{
    LSTilemapCmp::Decode(m_rom.data(offset), m_tilemap);
}

void MainFrame::InitPals(const wxTreeItemId& node)
{
    const uint8_t* const base_pal = m_rom.data(m_rom.read<uint32_t>(0xA0A04));
    const uint8_t* pal = base_pal;
    for(size_t i = 0; i < 54; ++i)
    {
        m_pal2.push_back(Palette(pal, i, Palette::ROOM_PALETTE));

        std::ostringstream ss;
        ss << std::dec << std::setw(2) << std::setfill('0') << i;
        m_browser->AppendItem(node, ss.str(), 2, 2, new TreeNodeData(TreeNodeData::NODE_ROOM_PAL, i));
    }

    m_palette.clear();
    m_palette.emplace_back(m_pal2[0]);
    m_palette.emplace_back();
    m_palette.emplace_back();
    m_palette.emplace_back();
}
void MainFrame::OnExport(wxCommandEvent& event)
{
    if (m_imgbuf.GetWidth() > 0)
    {
        wxFileDialog    fdlog(this, _("Export to PNG"), "", "",
            "PNG file (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (fdlog.ShowModal() == wxID_OK)
        {
            m_imgbuf.WritePNG(std::string(fdlog.GetPath()), m_palette);
        }
    }
    event.Skip();
}
void MainFrame::OnOpen(wxCommandEvent& event)
{
    wxFileDialog fdlog(this, "Open Landstalker ROM file", "", "",
        "ROM files (*.bin; *.md)|*.bin;*.md", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (fdlog.ShowModal() == wxID_OK)
    {
        OpenRomFile(fdlog.GetPath());
    }
    event.Skip();
}

void MainFrame::InitRoom(uint16_t room)
{
    m_roomnum = room;
    const RoomData& rd = m_rooms[m_roomnum];
    m_rpalidx = rd.roomPalette;
    m_palette[0] = m_pal2[m_rpalidx];
    m_tsidx = rd.tileset;
    LoadTileset(m_tilesetOffsets[m_tsidx]);
    m_bigTiles.clear();
    LoadBigTiles(m_bigTileOffsets[rd.bigTilesetIdx][0]);
    LoadBigTiles(m_bigTileOffsets[rd.bigTilesetIdx][1 + rd.secBigTileset]);
    LoadTilemap(rd.offset);
}

void MainFrame::PopulateRoomProperties(uint16_t room, const RoomTilemap& tm)
{
    m_properties->GetGrid()->Clear();
    std::ostringstream ss;
    ss.str(std::string());
    const RoomData& rd = m_rooms[room];

    ss << "Room: " << std::dec << std::uppercase << std::setw(3) << std::setfill('0') << room
        << " Tileset: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.tileset)
        << " Palette: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.roomPalette)
        << " PriBigTiles: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.priBigTileset)
        << " SecBigTiles: 0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.secBigTileset)
        << " BGM: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.backgroundMusic)
        << " Map Offset: 0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << static_cast<unsigned>(rd.offset);
    SetStatusText(ss.str());
    ss.str(std::string());
    ss << std::dec << std::uppercase << std::setw(3) << std::setfill('0') << room;
    m_properties->Append(new wxStringProperty("Room Number", "RN", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.tileset);
    m_properties->Append(new wxStringProperty("Tileset", "TS", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.roomPalette);
    m_properties->Append(new wxStringProperty("Room Palette", "RP", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.priBigTileset);
    m_properties->Append(new wxStringProperty("Primary Big Tiles", "PBT", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.secBigTileset);
    m_properties->Append(new wxStringProperty("Secondary Big Tiles", "SBT", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.backgroundMusic);
    m_properties->Append(new wxStringProperty("BGM", "BGM", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << static_cast<unsigned>(rd.offset);
    m_properties->Append(new wxStringProperty("Map Offset", "MO", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.unknownParam1);
    m_properties->Append(new wxStringProperty("Unknown Parameter 1", "UP1", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.unknownParam2);
    m_properties->Append(new wxStringProperty("Unknown Parameter 2", "UP2", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.unknownParam3);
    m_properties->Append(new wxStringProperty("Unknown Parameter 3", "UP3", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.GetLeft());
    m_properties->Append(new wxStringProperty("Tilemap Left Offset", "TLO", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.GetTop());
    m_properties->Append(new wxStringProperty("Tilemap Top Offset", "TTO", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.GetWidth());
    m_properties->Append(new wxStringProperty("Tilemap Width", "TW", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.GetHeight());
    m_properties->Append(new wxStringProperty("Tilemap Height", "TH", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.hmwidth);
    m_properties->Append(new wxStringProperty("Heightmap Width", "HW", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.hmheight);
    m_properties->Append(new wxStringProperty("Heightmap Height", "HH", ss.str()));
}

void MainFrame::EnableLayerControls(bool state)
{
    if (state != m_layer_controls_enabled)
    {
        m_optBgSelect->Enable(state);
        m_optFg1Select->Enable(state);
        m_optFg2Select->Enable(state);
        m_optHeightmapSelect->Enable(state);
        m_optSpritesSelect->Enable(state);
        m_checkBgVisible->Enable(state);
        m_checkFg1Visible->Enable(state);
        m_checkFg2Visible->Enable(state);
        m_checkHeightmapVisible->Enable(state);
        m_checkSpritesVisible->Enable(state);
        m_sliderBgOpacity->Enable(state);
        m_sliderFg1Opacity->Enable(state);
        m_sliderFg2Opacity->Enable(state);
        m_sliderHeightmapOpacity->Enable(state);
        m_sliderSpritesOpacity->Enable(state);
        
        if (state == true)
        {
            // Reset to default state
            m_optBgSelect->SetValue(true);
            m_checkBgVisible->SetValue(true);
            m_checkFg1Visible->SetValue(true);
            m_checkFg2Visible->SetValue(true);
            m_checkHeightmapVisible->SetValue(false);
            m_checkSpritesVisible->SetValue(true);
            m_sliderBgOpacity->SetValue(255);
            m_sliderFg1Opacity->SetValue(255);
            m_sliderFg2Opacity->SetValue(255);
            m_sliderHeightmapOpacity->SetValue(192);
            m_sliderSpritesOpacity->SetValue(255);
        }
        m_layer_controls_enabled = state;
    }
}

void MainFrame::SetMode(const Mode& mode)
{
    m_mode = mode;
    Refresh();
}

void MainFrame::Refresh()
{
    switch (m_mode)
    {
    case MODE_TILESET:
        // Display tileset
        EnableLayerControls(false);
        LoadTileset(m_tilesetOffsets[m_tsidx]);
        DrawTiles(16, 2, m_rpalidx);
        break;
    case MODE_BLOCKSET:
        EnableLayerControls(false);
        LoadTileset(m_tilesetOffsets[m_tsidx]);
        LoadBigTiles(m_bigTileOffsets[m_bs2][0]);
        if (m_bs2 > 0)
        {
            LoadBigTiles(m_bigTileOffsets[m_bs1][m_bs2]);
        }
        DrawBigTiles(16, 1, m_rpalidx);
        // Display blockset
        break;
    case MODE_PALETTE:
        // Display palettes
        EnableLayerControls(false);
        break;
    case MODE_ROOMMAP:
        // Display room map
        EnableLayerControls(true);
        InitRoom(m_roomnum);
        PopulateRoomProperties(m_roomnum, m_tilemap);
        DrawTilemap(m_scale, m_rpalidx);
        break;
    case MODE_SPRITE:
    {
        // Display sprite
        EnableLayerControls(false);
        const auto& sprite = m_sprites[m_sprite_idx];
        const auto& sprite_gfx = m_spriteGraphics[sprite.GetGraphicsIdx()];
        uint32_t frame = sprite_gfx.RetrieveFrameIdx(m_sprite_anim, m_sprite_frame);
        m_palette[1] = sprite.GetPalette(m_rom.data(0x1A4BA0), m_rom.data(0x1A47E0));
        DrawSprite(m_spriteFrames[frame], 1, 4);
        break;
    }
    case MODE_NONE:
    default:
        // Clear Screen
        EnableLayerControls(false);
        bmp = std::make_shared<wxBitmap>(1, 1);
        memDc.SelectObject(*bmp);
        memDc.SetBackground(*wxBLACK_BRUSH);
        memDc.Clear();
        memDc.SelectObject(wxNullBitmap);
        ForceRepaint();
        break;
    }
}

void MainFrame::OnBrowserSelect(wxTreeEvent& event)
{
    TreeNodeData* itemData = static_cast<TreeNodeData*>(m_browser->GetItemData(event.GetItem()));
    m_properties->GetGrid()->Clear();
    switch (itemData->GetNodeType())
    {
    case TreeNodeData::NODE_TILESET:
        m_tsidx = itemData->GetValue();
        SetMode(MODE_TILESET);
        break;
    case TreeNodeData::NODE_BIG_TILES:
    {
        size_t sel = itemData->GetValue();
        m_bs1 = sel >> 16;
        m_bs2 = sel & 0xFFFF;
        m_tsidx = m_bs1 & 0x1F;
        SetMode(MODE_BLOCKSET);
        break;
    }
    case TreeNodeData::NODE_ROOM_PAL:
    {
        m_rpalidx = itemData->GetValue();
        m_palette[0] = m_pal2[m_rpalidx];
        Refresh();
        break;
    }
    case TreeNodeData::NODE_ROOM:
        m_roomnum = itemData->GetValue();
        SetMode(MODE_ROOMMAP);
        break;
    case TreeNodeData::NODE_ROOM_HEIGHTMAP:
        InitRoom(itemData->GetValue());
        PopulateRoomProperties(m_roomnum, m_tilemap);
        DrawHeightmap(1, m_roomnum);
        break;
    case TreeNodeData::NODE_SPRITE:
    {
        Palette pal;
        uint32_t data = itemData->GetValue();
        m_sprite_idx = data & 0xFF;
        m_sprite_anim = (data >> 16) & 0xFF;
        m_sprite_frame = (data >> 8) & 0xFF;
        SetMode(MODE_SPRITE);
        break;
    }
    default:
        // do nothing
        break;
    }

    event.Skip();
}
void MainFrame::OnButton1(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton2(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton3(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton4(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton5(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton6(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton7(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton8(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton9(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnButton10(wxCommandEvent& event)
{
    event.Skip();
}
void MainFrame::OnKeyDown(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnKeyUp(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnMousewheel(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowKeyDown(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowKeyUp(wxKeyEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowLeftDown(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowLeftUp(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowMousewheel(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowMouseMove(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowRightDown(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnScrollWindowRightUp(wxMouseEvent& event)
{
    event.Skip();
}
void MainFrame::OnLayerOpacityChange(wxScrollEvent& event)
{
    DrawTilemap(m_scale, m_rpalidx);
    event.Skip();
}
void MainFrame::OnLayerSelect(wxCommandEvent& event)
{
    DrawTilemap(m_scale, m_rpalidx);
    event.Skip();
}
void MainFrame::OnLayerVisibilityChange(wxCommandEvent& event)
{
    DrawTilemap(m_scale, m_rpalidx);
    event.Skip();
}
