#include "MainFrame.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>

#include <wx/wx.h>
#include <wx/aboutdlg.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/colour.h>

#include "LZ77.h"
#include "BigTilesCmp.h"
#include "LSTilemapCmp.h"
#include "Rom.h"

MainFrame::MainFrame(wxWindow* parent)
    : MainFrameBaseClass(parent),
      m_gfxSize(0),
      m_scale(1),
      m_rpalidx(0),
      m_tsidx(0),
      m_roomnum(0)
{
    m_imgs = new ImgLst();
    bmp.Create(1,1);
}

MainFrame::~MainFrame()
{
}

void MainFrame::OnExit(wxCommandEvent& event)
{
    wxUnusedVar(event);
    Close();
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxUnusedVar(event);
    wxAboutDialogInfo info;
    info.SetCopyright(_("My MainFrame"));
    info.SetLicence(_("GPL v2 or later"));
    info.SetDescription(_("Short description goes here"));
    ::wxAboutBox(info);
}


void MainFrame::OnButton41ButtonClicked(wxCommandEvent& event)
{
}

void MainFrame::OpenRomFile(const wxString& path)
{
    try
    {
        m_rom.load_from_file(static_cast<std::string>(path));

        m_tilesetOffsets = m_rom.read_array<uint32_t>(0x44070, 31);
        m_treeCtrl101->DeleteAllItems();
        m_treeCtrl101->SetImageList(m_imgs);
        wxTreeItemId nodeRoot = m_treeCtrl101->AddRoot("");
        wxTreeItemId nodeTs = m_treeCtrl101->AppendItem(nodeRoot, "Tilesets", 1, 1, new TreeNodeData());
        wxTreeItemId nodeATs = m_treeCtrl101->AppendItem(nodeRoot, "Animated Tilesets", 1, 1, new TreeNodeData());
        wxTreeItemId nodeBTs = m_treeCtrl101->AppendItem(nodeRoot, "Big Tilesets", 3, 3, new TreeNodeData());
        wxTreeItemId nodeRPal = m_treeCtrl101->AppendItem(nodeRoot, "Room Palettes", 2, 2, new TreeNodeData());
        wxTreeItemId nodeRm = m_treeCtrl101->AppendItem(nodeRoot, "Rooms", 0, 0, new TreeNodeData());

        for (const auto& offset : m_tilesetOffsets)
        {
            m_treeCtrl101->AppendItem(nodeTs, Hex(offset), 1, 1, new TreeNodeData(TreeNodeData::NODE_TILESET, offset));
        }
        auto bt = m_rom.read_array<uint32_t>(m_rom.read<uint32_t>(0x1AF800), 64);
        for (size_t i = 0; i < 64; ++i)
        {
            m_bigTileOffsets.push_back(m_rom.read_array<uint32_t>(bt[i], 9));
            wxTreeItemId curTn = m_treeCtrl101->AppendItem(nodeBTs, Hex(bt[i]), 3, 3, new TreeNodeData(TreeNodeData::NODE_BIG_TILES, 0));
            for (size_t j = 0; j < 9; ++j)
            {
                m_treeCtrl101->AppendItem(curTn, Hex(m_bigTileOffsets[i][j]), 3, 3, new TreeNodeData(TreeNodeData::NODE_BIG_TILES, i << 16 | j));
            }
        }
        const uint8_t* rm = m_rom.data(m_rom.read<uint32_t>(0xA0A00));
        for (size_t i = 0; i < 816; i++)
        {
            std::ostringstream ss;
            m_rooms.push_back(RoomData(rm));
            rm += 8;
            ss << i;
            wxTreeItemId cRm = m_treeCtrl101->AppendItem(nodeRm, ss.str(), 0, 0, new TreeNodeData(TreeNodeData::NODE_ROOM, i));
            m_treeCtrl101->AppendItem(cRm, "Heightmap", 0, 0, new TreeNodeData(TreeNodeData::NODE_ROOM_HEIGHTMAP, i));
        }
        InitPals(nodeRPal);
    }
    catch(const std::runtime_error& e)
    {
        m_treeCtrl101->DeleteAllItems();
        wxMessageBox(e.what());
    }
}

void MainFrame::DrawBigTiles(size_t row_width, size_t scale, uint8_t pal)
{
    const size_t TILE_WIDTH = 16;
    const size_t TILE_HEIGHT = 16;
    const size_t MAX_WIDTH = 128;
    const size_t MAX_HEIGHT = 256;
    const size_t ROW_WIDTH = std::min(std::min(MAX_WIDTH, m_bigTiles.size()), row_width);
    const size_t ROW_HEIGHT = std::min(MAX_HEIGHT, (m_bigTiles.size() + ROW_WIDTH - 1) / ROW_WIDTH);
    const size_t BMP_WIDTH = TILE_WIDTH * ROW_WIDTH;
    const size_t BMP_HEIGHT = TILE_HEIGHT * ROW_HEIGHT;

    size_t x = 0;
    size_t y = 0;
    bmp.Create(BMP_WIDTH, BMP_HEIGHT);
    memDc.SelectObject(bmp);
    memDc.SetBackground(*wxBLACK_BRUSH);
    memDc.Clear();
    memDc.SetPen(*wxWHITE_PEN);
    memDc.SetBrush(*wxBLACK_BRUSH);
    memDc.SetTextBackground(*wxBLACK);
    memDc.SetTextForeground(*wxWHITE);
    m_tilebmps.setPalette(m_pal2[pal]);

    for(auto& b : m_bigTiles)
    {
        for(int i = 0; i < 4; i++)
        {
            const Tile& t = b.getTile(i);
            size_t xoff = (i & 1) ? 1 : 0;
            size_t yoff = (i & 2) ? 1 : 0;
            m_tilebmps.draw(memDc, t.getIndex(), x + xoff, y + yoff, t.attributes());
        }
        x+=2;
        if(x == ROW_WIDTH*2)
        {
            x = 0;
            y+=2;
        }
    }
    memDc.SelectObject(wxNullBitmap);

    m_scale = scale;
    m_scrollWin27->SetScrollbars(scale,scale,BMP_WIDTH,BMP_HEIGHT,0,0);
    wxClientDC dc(m_scrollWin27);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, scale);
}

void MainFrame::DrawTilemap(size_t scale, uint8_t pal)
{
    const size_t TILE_WIDTH = 16;
    const size_t TILE_HEIGHT = 16;
    const size_t ROW_WIDTH = m_tilemap.width;
    const size_t ROW_HEIGHT = m_tilemap.height;
    const size_t BMP_WIDTH = (ROW_WIDTH + ROW_HEIGHT) * TILE_WIDTH;
    const size_t BMP_HEIGHT = (ROW_HEIGHT + ROW_WIDTH + 1) * TILE_HEIGHT / 2;

    size_t x = 0;
    size_t y = 0;
    bmp.Create(BMP_WIDTH, BMP_HEIGHT);
    memDc.SelectObject(bmp);
    memDc.SetBackground(*wxBLACK_BRUSH);
    memDc.Clear();
    m_tilebmps.setPalette(m_pal2[pal]);

    for(size_t ti = 0; ti < m_tilemap.background.size(); ++ti)
    {
        for(int i = 0; i < 4; i++)
        {
            size_t xoff = (i & 1) ? 1 : 0;
            size_t yoff = (i & 2) ? 1 : 0;
            const Tile* t = &m_bigTiles[m_tilemap.background[ti]].getTile(i);
            m_tilebmps.draw(memDc, t->getIndex(), x + (ROW_HEIGHT - y/2 - 1)*2 + xoff + 2, y/2 + x/2 + yoff, t->attributes());
        }
        x+=2;
        if(x == ROW_WIDTH*2)
        {
            x = 0;
            y+=2;
        }
    }
    x=0;
    y=0;
    for(size_t ti = 0; ti < m_tilemap.foreground.size(); ++ti)
    {
        for(int i = 0; i < 4; i++)
        {
            size_t xoff = (i & 1) ? 1 : 0;
            size_t yoff = (i & 2) ? 1 : 0;
            const Tile* t = &m_bigTiles[m_tilemap.foreground[ti]].getTile(i);
            m_tilebmps.draw(memDc, t->getIndex(), x + (ROW_HEIGHT - y/2 - 1)*2 + xoff, y/2 + x/2 + yoff, t->attributes());
        }
        x+=2;
        if(x == ROW_WIDTH*2)
        {
            x = 0;
            y+=2;
        }
    }
    memDc.SelectObject(wxNullBitmap);

    m_scale = scale;
    m_scrollWin27->SetScrollbars(scale,scale,BMP_WIDTH,BMP_HEIGHT,0,0);
    wxClientDC dc(m_scrollWin27);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, scale);
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
    bmp.Create(BMP_WIDTH, BMP_HEIGHT);
    memDc.SelectObject(bmp);
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
            memDc.DrawRectangle(x*TILE_WIDTH, y*TILE_HEIGHT, TILE_WIDTH+1, TILE_HEIGHT+1);
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
    m_scrollWin27->SetScrollbars(scale,scale,BMP_WIDTH,BMP_HEIGHT,0,0);
    wxClientDC dc(m_scrollWin27);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, scale);
}

void MainFrame::DrawTiles(size_t row_width, size_t scale, uint8_t pal)
{
    const size_t TILE_WIDTH = 8;
    const size_t TILE_HEIGHT = 8;
    const size_t MAX_WIDTH = 128;
    const size_t MAX_HEIGHT = 256;
    const size_t ROW_WIDTH = std::min(std::min(MAX_WIDTH, m_tilebmps.size()), row_width);
    const size_t ROW_HEIGHT = std::min(MAX_HEIGHT, (m_tilebmps.size() + ROW_WIDTH - 1) / ROW_WIDTH);
    const size_t BMP_WIDTH = TILE_WIDTH * ROW_WIDTH;
    const size_t BMP_HEIGHT = TILE_HEIGHT * ROW_HEIGHT;

    size_t x = 0;
    size_t y = 0;
    bmp.Create(BMP_WIDTH, BMP_HEIGHT);
    TileAttributes attr;
    memDc.SelectObject(bmp);
    memDc.SetBackground(*wxBLACK_BRUSH);
    memDc.Clear();
    m_tilebmps.setPalette(m_pal2[pal]);
    for(size_t i = 0; i < m_tilebmps.size(); ++i)
    {
        m_tilebmps.draw(memDc, i, x, y, attr);

        x++;
        if(x == ROW_WIDTH)
        {
            x = 0;
            y++;
        }
    }
    memDc.SelectObject(wxNullBitmap);

    m_scale = scale;
    m_scrollWin27->SetScrollbars(scale,scale,BMP_WIDTH,BMP_HEIGHT,0,0);
    wxClientDC dc(m_scrollWin27);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    PaintNow(dc, scale);
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
}

void MainFrame::PaintNow(wxDC& dc, size_t scale)
{
        int x, y, w, h;
        m_scrollWin27->GetViewStart(&x, &y);
        m_scrollWin27->GetClientSize(&w, &h);
        double dscale = static_cast<double>(scale);
        memDc.SelectObject(wxNullBitmap);
        memDc.SelectObject(bmp);
        dc.SetUserScale(dscale, dscale);
        dc.Blit(0, 0, w/dscale+1, h/dscale+1, &memDc, x, y, wxCOPY, true);
        memDc.SelectObject(wxNullBitmap);
}

void MainFrame::OnScrollwin27Paint(wxPaintEvent& event)
{
    wxPaintDC dc(m_scrollWin27);
    PaintNow(dc, m_scale);
}
void MainFrame::OnButton51ButtonClicked(wxCommandEvent& event)
{
    uint8_t buffer[131072];
    uint8_t ebuffer[131072];
    std::memset(buffer, 0x00, sizeof(buffer));
    std::memset(ebuffer, 0x00, sizeof(ebuffer));
    size_t elen = 0;
    size_t len = LZ77::Decode(m_rom.data(m_tilesetOffsets[0]), 0, buffer, elen);
    std::ostringstream ss;
    ss << "Extracted " << len << " bytes. Encoded len " << elen << " bytes.";
    wxMessageBox(ss.str());
    size_t enclen = LZ77::Encode(buffer, len, ebuffer);
    double ratio = static_cast<double>(enclen) / static_cast<double>(len) * 100.0;
    ss.str(std::string());
    ss << "Encoded to " << enclen << " bytes. Compression ratio: " << ratio << "%";
    wxMessageBox(ss.str());
    std::memset(buffer, 0x00, sizeof(buffer));
    LZ77::Decode(ebuffer, 0, buffer, elen);
    DrawTiles(16, 2, m_rpalidx);
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
        m_pal2.push_back(Palette(pal));

        std::ostringstream ss;
        ss << std::dec << std::setw(2) << std::setfill('0') << i;
        m_treeCtrl101->AppendItem(node, ss.str(), 2, 2, new TreeNodeData(TreeNodeData::NODE_ROOM_PAL, i));

        pal += 26;
    }
}

void MainFrame::OnMenuitem109MenuSelected(wxCommandEvent& event)
{
    wxFileDialog    fdlog(this);
    if(fdlog.ShowModal() != wxID_OK)
    {
        return;
    }
    OpenRomFile(fdlog.GetPath());
}
void MainFrame::OnAuimgr127Paint(wxPaintEvent& event)
{
    wxPaintDC dc(m_scrollWin27);
    PaintNow(dc, m_scale);
}

void MainFrame::InitRoom(uint16_t room)
{
    m_roomnum = room;
    const RoomData& rd = m_rooms[m_roomnum];
    m_rpalidx = rd.roomPalette;
    m_tsidx = rd.tileset;
    LoadTileset(m_tilesetOffsets[m_tsidx]);
    m_bigTiles.clear();
    LoadBigTiles(m_bigTileOffsets[rd.bigTilesetIdx][0]);
    LoadBigTiles(m_bigTileOffsets[rd.bigTilesetIdx][1 + rd.secBigTileset]);
    LoadTilemap(rd.offset);
}

void MainFrame::PopulateRoomProperties(uint16_t room, const Tilemap& tm)
{
    m_pgMgr146->GetGrid()->Clear();
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
    m_pgMgr146->Append(new wxStringProperty("Room Number", "RN", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.tileset);
    m_pgMgr146->Append(new wxStringProperty("Tileset", "TS", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.roomPalette);
    m_pgMgr146->Append(new wxStringProperty("Room Palette", "RP", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.priBigTileset);
    m_pgMgr146->Append(new wxStringProperty("Primary Big Tiles", "PBT", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.secBigTileset);
    m_pgMgr146->Append(new wxStringProperty("Secondary Big Tiles", "SBT", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.backgroundMusic);
    m_pgMgr146->Append(new wxStringProperty("BGM", "BGM", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << static_cast<unsigned>(rd.offset);
    m_pgMgr146->Append(new wxStringProperty("Map Offset", "MO", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.unknownParam1);
    m_pgMgr146->Append(new wxStringProperty("Unknown Parameter 1", "UP1", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(rd.unknownParam2);
    m_pgMgr146->Append(new wxStringProperty("Unknown Parameter 2", "UP2", ss.str()));
    ss.str(std::string());
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(rd.unknownParam3);
    m_pgMgr146->Append(new wxStringProperty("Unknown Parameter 3", "UP3", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.left);
    m_pgMgr146->Append(new wxStringProperty("Tilemap Left Offset", "TLO", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.top);
    m_pgMgr146->Append(new wxStringProperty("Tilemap Top Offset", "TTO", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.width);
    m_pgMgr146->Append(new wxStringProperty("Tilemap Width", "TW", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.height);
    m_pgMgr146->Append(new wxStringProperty("Tilemap Height", "TH", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.hmwidth);
    m_pgMgr146->Append(new wxStringProperty("Heightmap Width", "HW", ss.str()));
    ss.str(std::string());
    ss << std::dec << static_cast<unsigned>(tm.hmheight);
    m_pgMgr146->Append(new wxStringProperty("Heightmap Height", "HH", ss.str()));
}

void MainFrame::OnTreectrl101TreeItemActivated(wxTreeEvent& event)
{
    TreeNodeData* itemData = static_cast<TreeNodeData*>(m_treeCtrl101->GetItemData(event.GetItem()));
    m_pgMgr146->GetGrid()->Clear();
    switch(itemData->GetNodeType())
    {
        case TreeNodeData::NODE_TILESET:
            LoadTileset(itemData->GetValue());
            DrawTiles(16, 2, m_rpalidx);
            break;
        case TreeNodeData::NODE_BIG_TILES:
        {
            size_t sel = itemData->GetValue();
            size_t bt1 = sel >> 16;
            size_t bt2 = sel & 0xFFFF;
            m_bigTiles.clear();
            LoadTileset(m_tilesetOffsets[m_tsidx]);
            LoadBigTiles(m_bigTileOffsets[bt1][0]);
            if (bt2 > 0)
            {
                LoadBigTiles(m_bigTileOffsets[bt1][bt2]);
            }
            DrawBigTiles(16, 1, m_rpalidx);
            break;
        }
        case TreeNodeData::NODE_ROOM_PAL:
            m_rpalidx = itemData->GetValue();
            LoadTileset(m_tilesetOffsets[m_tsidx]);
            DrawTiles(16, 2, m_rpalidx);
            break;
        case TreeNodeData::NODE_ROOM:
            InitRoom(itemData->GetValue());
            PopulateRoomProperties(m_roomnum, m_tilemap);
            DrawTilemap(1, m_rpalidx);
            break;
        case TreeNodeData::NODE_ROOM_HEIGHTMAP:
            InitRoom(itemData->GetValue());
            PopulateRoomProperties(m_roomnum, m_tilemap);
            //DrawTilemap(1, n_rpalidx);
            DrawHeightmap(1, m_roomnum);
        default:
            // do nothing
            break;
    }
}
