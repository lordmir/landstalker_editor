#ifndef MAINFRAME_H
#define MAINFRAME_H
#include "wxcrafter.h"
#include <cstdint>
#include <vector>

class wxImage;

class MainFrame : public MainFrameBaseClass
{
public:
    MainFrame(wxWindow* parent);
    virtual ~MainFrame();

    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
protected:
    virtual void OnTreectrl55TreeItemActivated(wxTreeEvent& event);
    virtual void OnButton51ButtonClicked(wxCommandEvent& event);
    virtual void OnScrollwin27Paint(wxPaintEvent& event);
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnButton41ButtonClicked(wxCommandEvent& event);
private:
    void DrawTest(const uint8_t* buf, size_t n_tiles, size_t row_width = -1, size_t scale = 1, uint8_t pal = 0);
    void PaintNow(wxDC& dc, size_t scale = 1);
    void InitPals(const wxTreeItemId& node);
    void LoadTileset(size_t offset);
    ImgLst* m_imgs;
    uint8_t m_rom[2*1024*1024];
    uint8_t m_gfxBuffer[131072];
    size_t m_gfxSize;
    wxImage m_img;
    size_t m_scale;
    uint8_t m_rpalidx;
    std::vector<uint32_t> m_tilesetOffsets;
    uint16_t m_pal[54][15];
};
#endif // MAINFRAME_H
