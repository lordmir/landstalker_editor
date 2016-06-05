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

MainFrame::MainFrame(wxWindow* parent)
    : MainFrameBaseClass(parent),
      m_gfxSize(0),
      m_scale(1),
      m_rpalidx(0)
{
    m_imgs = new ImgLst();
}

MainFrame::~MainFrame()
{
    delete m_imgs;
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

class TreeNodeData : public wxTreeItemData
{
public:
    enum NodeType {NODE_BASE, NODE_TILESET, NODE_ANIM_TILESET, NODE_ROOM_PAL, NODE_ROOM};
    TreeNodeData(NodeType nodeType = NODE_BASE, size_t value = 0) : m_nodeType(nodeType), m_value(value) {}
    size_t GetValue() const { return m_value; }
    NodeType GetNodeType() const { return m_nodeType; }
private:
    const NodeType m_nodeType;
    const size_t m_value;
};

void MainFrame::OnButton41ButtonClicked(wxCommandEvent& event)
{
    std::ifstream inFile;
    size_t sz = 0;
    inFile.open(m_filePicker49->GetPath(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!inFile.is_open())
    {
        std::ostringstream ss;
        ss << "Unable to open ROM file " << m_filePicker49->GetPath() << ".";
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
            ss << "ROM file " << m_filePicker49->GetPath() << ": Bad ROM size! Expected " << std::dec << sizeof(m_rom) << " bytes, read " << sz << " bytes.";
            wxMessageBox(ss.str());
        }
        else
        {
            uint32_t* offsets = reinterpret_cast<uint32_t*>(m_rom + 0x44070);
            inFile.read(reinterpret_cast<char*>(m_rom), sz);
            std::ostringstream ss;
            m_treeCtrl55->DeleteAllItems();
            m_treeCtrl55->SetImageList(m_imgs);
            wxTreeItemId nodeRoot = m_treeCtrl55->AddRoot("");
            wxTreeItemId nodeTs = m_treeCtrl55->AppendItem(nodeRoot, "Tilesets", 1, 1, new TreeNodeData());
            wxTreeItemId nodeATs = m_treeCtrl55->AppendItem(nodeRoot, "Animated Tilesets", 1, 1, new TreeNodeData());
            wxTreeItemId nodeRPal = m_treeCtrl55->AppendItem(nodeRoot, "Room Palettes", 2, 2, new TreeNodeData());
            wxTreeItemId nodeRm = m_treeCtrl55->AppendItem(nodeRoot, "Rooms", 0, 0, new TreeNodeData());
            for(size_t i = 0; i < 31; ++i)
            {
                ss.str(std::string());
                uint32_t offset = ntohl(*offsets++);
                ss << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << offset;
                m_choice53->Append(ss.str());
                m_treeCtrl55->AppendItem(nodeTs, ss.str(), 1, 1, new TreeNodeData(TreeNodeData::NODE_TILESET, offset));
                m_tilesetOffsets.push_back(offset);
            }
            for(size_t i = 0; i < 63; i++)
            {
                
            }
            m_choice53->SetSelection(0);
            InitPals(nodeRPal);
            DrawTest(m_rom, 65536);
        }
        inFile.close();
    }
}

void MainFrame::DrawTest(const uint8_t* buf, size_t n_tiles, size_t row_width, size_t scale, uint8_t pal)
{
    const size_t TILE_WIDTH = 8;
    const size_t TILE_HEIGHT = 8;
    const size_t MAX_WIDTH = 128;
    const size_t MAX_HEIGHT = 256;
    const size_t ROW_WIDTH = std::min(std::min(MAX_WIDTH, n_tiles), row_width);
    const size_t ROW_HEIGHT = std::min(MAX_HEIGHT, (n_tiles + ROW_WIDTH - 1) / ROW_WIDTH);
    const size_t BMP_WIDTH = TILE_WIDTH * ROW_WIDTH;
    const size_t BMP_HEIGHT = TILE_HEIGHT * ROW_HEIGHT;
    const size_t N_PIXELS = BMP_WIDTH * BMP_HEIGHT;
    const size_t BYTES_PER_PIXEL = 3;
    const size_t PALETTE_ENTRIES = 16;
    
    uint8_t rgba[PALETTE_ENTRIES * 4];
    
    uint8_t* p = rgba;
    for(size_t i = 0; i < PALETTE_ENTRIES; ++i)
    {
        *p++ =  (m_pal[pal][i] & 0xF)       * 18;
        *p++ = ((m_pal[pal][i] >> 4) & 0xF) * 18;
        *p++ = ((m_pal[pal][i] >> 8) & 0xF) * 18;
        *p++ = (m_pal[pal][i] >> 12) ? 0 : 0xFF;
    }
    
    uint8_t* rgbmap = reinterpret_cast<uint8_t*>(malloc(N_PIXELS * BYTES_PER_PIXEL));
    uint8_t* alphamap = reinterpret_cast<uint8_t*>(malloc(N_PIXELS));
    std::memset(rgbmap, 0x00, N_PIXELS * BYTES_PER_PIXEL);
    std::memset(alphamap, 0x00, N_PIXELS);
    
    for(size_t ty = 0; ty < ROW_HEIGHT; ty++)
    for(size_t tx = 0; tx < ROW_WIDTH; tx++)
    {
        if((ty * ROW_WIDTH + tx) >= n_tiles) goto finished;
        for(size_t py = 0; py < TILE_HEIGHT; py++)
        {
            size_t offset = (py  + ty * TILE_HEIGHT) * BMP_WIDTH + tx * TILE_WIDTH;
            uint8_t* rgb_ptr = rgbmap + offset * BYTES_PER_PIXEL;
            uint8_t* alpha_ptr = alphamap + offset;
            for(size_t px = 0; px < TILE_WIDTH; px++)
            {
                if(px & 1)
                {
                    p = rgba + ((*buf++ & 0x0F) << 2);
                }
                else
                {
                    p = rgba + ((*buf >> 2) & 0x3C);
                }
                *rgb_ptr++ = *p++;
                *rgb_ptr++ = *p++;
                *rgb_ptr++ = *p++;
                *alpha_ptr++ = *p;
            }
        }
    }
finished:
    m_img.SetData(rgbmap, BMP_WIDTH, BMP_HEIGHT);
    m_img.SetAlpha(alphamap);
    m_scale = scale;
    m_scrollWin27->SetScrollbars(scale,scale,m_img.GetWidth(),m_img.GetHeight(),0,0);
    wxClientDC dc(m_scrollWin27); 
    dc.Clear();
    PaintNow(dc, scale);
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
}

void MainFrame::PaintNow(wxDC& dc, size_t scale)
{
    if(m_img.IsOk())
    { 
        wxMemoryDC memDC;
        wxBitmap* bmp = new wxBitmap(m_img);
        int x, y, w, h;
        m_scrollWin27->GetViewStart(&x, &y);
        m_scrollWin27->GetClientSize(&w, &h);
        
        
        memDC.SelectObject(*bmp);
        double dscale = static_cast<double>(scale);
        dc.SetUserScale(dscale, dscale);
        dc.Blit(0, 0, w/dscale+1, h/dscale+1, &memDC, x, y, wxCOPY, true);
        memDC.SelectObject(wxNullBitmap);
        delete bmp;
    }
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
    size_t len = LZ77::Decode(m_rom + m_tilesetOffsets[m_choice53->GetCurrentSelection()], 0, buffer, elen);
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
    DrawTest(buffer, (len + 31) / 32, 16, 2, m_rpalidx);
}

void MainFrame::LoadTileset(size_t offset)
{
    std::memset(m_gfxBuffer, 0x00, sizeof(m_gfxBuffer));
    size_t elen = 0;
    m_gfxSize = LZ77::Decode(m_rom + offset, sizeof(m_gfxBuffer), m_gfxBuffer, elen);
    DrawTest(m_gfxBuffer, (m_gfxSize + 31) / 32, 16, 2, m_rpalidx);    
}

void MainFrame::InitPals(const wxTreeItemId& node)
{
    uint32_t pal_ptr = ntohl(*reinterpret_cast<uint32_t*>(m_rom + 0xA0A04));
    const uint16_t* pal = reinterpret_cast<const uint16_t*>(m_rom + pal_ptr);
    for(size_t i = 0; i < 54; ++i)
    {
        m_pal[i][0]  = 0x1000;
        m_pal[i][1]  = 0x0CCC;
        m_pal[i][15] = 0x0000;
        for(size_t j = 0; j < 13; ++j)
        {
            m_pal[i][j+2] = ntohs(*pal++);
        }
        std::ostringstream ss;
        ss << "0x" << std::hex << std::uppercase << std::setw(6) << std::setfill('0')
           << ((uint8_t*)pal - m_rom);
        m_treeCtrl55->AppendItem(node, ss.str(), 2, 2, new TreeNodeData(TreeNodeData::NODE_ROOM_PAL, i));
    }
}
void MainFrame::OnTreectrl55TreeItemActivated(wxTreeEvent& event)
{
    TreeNodeData* itemData = static_cast<TreeNodeData*>(m_treeCtrl55->GetItemData(event.GetItem()));
    switch(itemData->GetNodeType())
    {
        case TreeNodeData::NODE_TILESET:
            LoadTileset(itemData->GetValue());
            break;
        case TreeNodeData::NODE_ROOM_PAL:
            m_rpalidx = itemData->GetValue();
            DrawTest(m_gfxBuffer, (m_gfxSize + 31) / 32, 16, 2, m_rpalidx);    
            break;
        default:
            // do nothing
            break;
    }
}
