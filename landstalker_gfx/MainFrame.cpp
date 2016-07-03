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

MainFrame::MainFrame(wxWindow* parent)
    : MainFrameBaseClass(parent),
      m_gfxSize(0),
      m_scale(1),
      m_rpalidx(0),
      m_tsidx(0),
      m_roomnum(0)
{
    m_imgs = new ImgLst();
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
    std::ifstream inFile;
    size_t sz = 0;
    inFile.open(path, std::ios::in | std::ios::binary | std::ios::ate);
    if(!inFile.is_open())
    {
        std::ostringstream ss;
        ss << "Unable to open ROM file " << path << ".";
        wxMessageBox(ss.str());
    }
    else
    {
        inFile.seekg (0, std::ios::end);
        sz = inFile.tellg();
        inFile.seekg (0, std::ios::beg);
        if(sz != sizeof(m_rom))
        {
            std::ostringstream ss;
            ss << "ROM file " << path << ": Bad ROM size! Expected " << std::dec << sizeof(m_rom) << " bytes, read " << sz << " bytes.";
            wxMessageBox(ss.str());
        }
        else
        {
            uint32_t* offsets = reinterpret_cast<uint32_t*>(m_rom + 0x44070);
            inFile.read(reinterpret_cast<char*>(m_rom), sz);
            std::ostringstream ss;
            m_treeCtrl101->DeleteAllItems();
            m_treeCtrl101->SetImageList(m_imgs);
            wxTreeItemId nodeRoot = m_treeCtrl101->AddRoot("");            
            wxTreeItemId nodeTs = m_treeCtrl101->AppendItem(nodeRoot, "Tilesets", 1, 1, new TreeNodeData());
            wxTreeItemId nodeATs = m_treeCtrl101->AppendItem(nodeRoot, "Animated Tilesets", 1, 1, new TreeNodeData());
            wxTreeItemId nodeBTs = m_treeCtrl101->AppendItem(nodeRoot, "Big Tilesets", 3, 3, new TreeNodeData());
            wxTreeItemId nodeRPal = m_treeCtrl101->AppendItem(nodeRoot, "Room Palettes", 2, 2, new TreeNodeData());
            wxTreeItemId nodeRm = m_treeCtrl101->AppendItem(nodeRoot, "Rooms", 0, 0, new TreeNodeData());
            for(size_t i = 0; i < 31; ++i)
            {
                ss.str(std::string());
                uint32_t offset = ntohl(*offsets++);
                ss << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << offset;
                m_treeCtrl101->AppendItem(nodeTs, ss.str(), 1, 1, new TreeNodeData(TreeNodeData::NODE_TILESET, offset));
                m_tilesetOffsets.push_back(offset);
            }
            const uint32_t* bt_offset_ptr = reinterpret_cast<uint32_t*>(m_rom + 0x1AF800);
            const uint32_t* bt_offset_list = reinterpret_cast<const uint32_t*>(m_rom + ntohl(*bt_offset_ptr));
            for(size_t i = 0; i < 64; ++i)
            {
                m_bigTileOffsets.push_back(std::vector<uint32_t>(8,0));
                const uint32_t* bt_offsets = reinterpret_cast<uint32_t*>(m_rom + ntohl(*bt_offset_list++));
                ss.str(std::string());
                ss << std::dec << std::uppercase << std::setw(2) << std::setfill('0') << i;
                wxTreeItemId curTn = m_treeCtrl101->AppendItem(nodeBTs, ss.str(), 3, 3, new TreeNodeData(TreeNodeData::NODE_BIG_TILES, 0));
                for(size_t j = 0; j < 8; ++j)
                {
                    uint32_t offset = ntohl(*bt_offsets++);
                    m_bigTileOffsets[i][j] = offset;
                    ss.str(std::string());
                    ss << std::dec << std::uppercase << std::setw(1) << std::setfill('0') << j;
                    m_treeCtrl101->AppendItem(curTn, ss.str(), 3, 3, new TreeNodeData(TreeNodeData::NODE_BIG_TILES, i << 16 | j));
                }
            }
            uint32_t rm_ptr = ntohl(*reinterpret_cast<uint32_t*>(m_rom + 0xA0A00));
            const uint8_t* rm = m_rom + rm_ptr;
            for(size_t i = 0; i < 816; i++)
            {
                ss.str(std::string());
                m_rooms.push_back(RoomData(rm));
                rm += 8;
                ss << i;
                m_treeCtrl101->AppendItem(nodeRm, ss.str(), 0, 0, new TreeNodeData(TreeNodeData::NODE_ROOM, i));
            }
            InitPals(nodeRPal);
            //DrawTest(m_rom, 65536);
        }
        inFile.close();
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
    size_t len = LZ77::Decode(m_rom + m_tilesetOffsets[0], 0, buffer, elen);
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
    m_gfxSize = LZ77::Decode(m_rom + offset, sizeof(m_gfxBuffer), m_gfxBuffer, elen);
    m_tilebmps.setBits(m_gfxBuffer, 0x400);
}

void MainFrame::LoadBigTiles(size_t offset)
{
    BigTilesCmp::Decode(m_rom + offset, m_bigTiles);
}

void MainFrame::LoadTilemap(size_t offset)
{
    LSTilemapCmp::Decode(m_rom + offset, m_tilemap);
}

void MainFrame::InitPals(const wxTreeItemId& node)
{
    uint32_t pal_ptr = ntohl(*reinterpret_cast<uint32_t*>(m_rom + 0xA0A04));
    const uint8_t* const base_pal = m_rom + pal_ptr;
    const uint8_t* pal = base_pal;
    for(size_t i = 0; i < 54; ++i)
    {
        m_pal2.push_back(Palette(pal));
        
        std::ostringstream ss;
        ss << "0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0')
           << (pal - m_rom);
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
void MainFrame::OnTreectrl101TreeItemActivated(wxTreeEvent& event)
{
    TreeNodeData* itemData = static_cast<TreeNodeData*>(m_treeCtrl101->GetItemData(event.GetItem()));
    std::ostringstream ss;
    m_pgMgr146->GetGrid()->Clear();
    switch(itemData->GetNodeType())
    {
        case TreeNodeData::NODE_TILESET:
            LoadTileset(itemData->GetValue());
            DrawTiles(16, 2, m_rpalidx);    
            break;
        case TreeNodeData::NODE_ROOM_PAL:
            m_rpalidx = itemData->GetValue();
            DrawTiles(16, 2, m_rpalidx);    
            break;
        case TreeNodeData::NODE_ROOM:
            m_roomnum = itemData->GetValue();
            m_rpalidx = m_rooms[m_roomnum].params[1] & 0x3F;
            m_tsidx = m_rooms[m_roomnum].params[0] & 0x1F;
            LoadTileset(m_tilesetOffsets[m_tsidx]);
            m_bigTiles.clear();
            LoadBigTiles(m_bigTileOffsets[m_rooms[m_roomnum].params[0] & 0x3F][0]);
            LoadBigTiles(m_bigTileOffsets[m_rooms[m_roomnum].params[0] & 0x3F][1 + (m_rooms[m_roomnum].params[3] >> 5)]);
            
            LoadTilemap(m_rooms[m_roomnum].offset);
            ss.str(std::string());
            ss << "Room: " << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << m_roomnum
               << " Tileset: " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(m_tsidx)
               << " Palette: " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(m_rpalidx)
               << " BigTiles: " << std::hex << std::uppercase << std::setw(1) << std::setfill('0') << static_cast<unsigned>(m_rooms[m_roomnum].params[3] >> 5)
               << " BGM: " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(m_rooms[m_roomnum].params[3] & 0x1F)
               << " Map Offset: 0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << static_cast<unsigned>(m_rooms[m_roomnum].offset);
            SetStatusText(ss.str());
            ss.str(std::string());
            ss << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << m_roomnum;
            m_pgMgr146->Append(new wxStringProperty("Room Number", "RN", ss.str()));
            ss.str(std::string());
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(m_tsidx);
            m_pgMgr146->Append(new wxStringProperty("Tileset", "TS", ss.str()));
            ss.str(std::string());
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(m_rpalidx);
            m_pgMgr146->Append(new wxStringProperty("Room Palette", "RP", ss.str()));
            ss.str(std::string());
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(m_rooms[m_roomnum].params[3] >> 5);
            m_pgMgr146->Append(new wxStringProperty("Big Tiles", "BT", ss.str()));
            ss.str(std::string());
            ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<unsigned>(m_rooms[m_roomnum].params[3] & 0x1F);
            m_pgMgr146->Append(new wxStringProperty("BGM", "BGM", ss.str()));
            ss.str(std::string());
            ss << "0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << static_cast<unsigned>(m_rooms[m_roomnum].offset);
            m_pgMgr146->Append(new wxStringProperty("Map Offset", "MO", ss.str()));
            DrawTilemap(1, m_rpalidx);    
            break;
        default:
            // do nothing
            break;
    }
}
