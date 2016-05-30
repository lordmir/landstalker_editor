#include "MainFrame.h"

#include <fstream>
#include <sstream>

#include <wx/wx.h>
#include <wx/aboutdlg.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/colour.h>

MainFrame::MainFrame(wxWindow* parent)
    : MainFrameBaseClass(parent),
      m_loaded(false)
{
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
            inFile.read(reinterpret_cast<char*>(m_rom), sz);
            DrawTest();
        }
        inFile.close();
    }
}

void MainFrame::DrawTest()
{
    const size_t TILE_WIDTH = 8;
    const size_t TILE_HEIGHT = 8;
    const size_t ROW_WIDTH = 256;
    const size_t ROW_HEIGHT = 256;
    const size_t N_TILES = ROW_WIDTH * ROW_HEIGHT;
    const size_t BMP_WIDTH = TILE_WIDTH * ROW_WIDTH;
    const size_t BMP_HEIGHT = TILE_HEIGHT * ROW_HEIGHT;
    const size_t N_PIXELS = BMP_WIDTH * BMP_HEIGHT;
    const size_t BYTES_PER_PIXEL = 3;
    
    const uint16_t gpal[16] = {0x1000, 0xCCC, 0x008, 0x080, 0x088, 0x800, 0x808, 0x880,
                               0x888, 0x00E, 0x0E0, 0x0EE, 0xE00, 0xE0E, 0xEE0, 0x000};
    uint8_t rgba[64];
    
    uint8_t* p = rgba;
    for(size_t i = 0; i < sizeof(gpal); ++i)
    {
        *p++ =  (gpal[i] & 0xF)       * 18;
        *p++ = ((gpal[i] >> 4) & 0xF) * 18;
        *p++ = ((gpal[i] >> 8) & 0xF) * 18;
        *p++ = (gpal[i] >> 12) ? 0 : 0xFF;
    }
    
    uint8_t* rgbmap = reinterpret_cast<uint8_t*>(malloc(N_PIXELS * BYTES_PER_PIXEL));
    uint8_t* alphamap = reinterpret_cast<uint8_t*>(malloc(N_PIXELS));
    uint8_t* rom_ptr = m_rom;
    
    for(size_t ty = 0; ty < ROW_HEIGHT; ty++)
    for(size_t tx = 0; tx < ROW_WIDTH; tx++)
    for(size_t py = 0; py < TILE_HEIGHT; py++)
    {
        size_t offset = (py  + ty * TILE_HEIGHT) * BMP_WIDTH + tx * TILE_WIDTH;
        uint8_t* rgb_ptr = rgbmap + offset * BYTES_PER_PIXEL;
        uint8_t* alpha_ptr = alphamap + offset;
        for(size_t px = 0; px < TILE_WIDTH; px++)
        {
            if(px & 1)
            {
                p = rgba + ((*rom_ptr++ >> 2) & 0x3C);
            }
            else
            {
                p = rgba + ((*rom_ptr & 0x0F) << 2);
            }
            *rgb_ptr++ = *p++;
            *rgb_ptr++ = *p++;
            *rgb_ptr++ = *p++;
            *alpha_ptr++ = *p;
        }
    }
    m_img.SetData(rgbmap, BMP_WIDTH, BMP_HEIGHT);
    m_img.SetAlpha(alphamap);
    m_loaded = true;
    m_scrollWin27->SetScrollbars(1,1,m_img.GetWidth(),m_img.GetHeight(),0,0);
    wxClientDC dc(m_scrollWin27);  
    PaintNow(dc);
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
}

void MainFrame::PaintNow(wxDC& dc)
{
    if(m_img.IsOk())
    { 
        wxMemoryDC memDC;
        wxBitmap* bmp = new wxBitmap(m_img);
        int x, y, w, h;
        m_scrollWin27->GetViewStart(&x, &y);
        m_scrollWin27->GetClientSize(&w, &h);
        
        
        memDC.SelectObject(*bmp);
        dc.Blit(0, 0, w, h, &memDC, x, y, wxCOPY, true);
        memDC.SelectObject(wxNullBitmap);
        delete bmp;
    }
}

void MainFrame::OnScrollwin27Paint(wxPaintEvent& event)
{
    wxPaintDC dc(m_scrollWin27);  
    PaintNow(dc);
}
