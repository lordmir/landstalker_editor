#ifndef MAINFRAME_H
#define MAINFRAME_H
#include "wxcrafter.h"
#include <cstdint>

class wxImage;

class MainFrame : public MainFrameBaseClass
{
public:
    MainFrame(wxWindow* parent);
    virtual ~MainFrame();

    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
protected:
    virtual void OnScrollwin27Paint(wxPaintEvent& event);
    virtual void OnPaint(wxPaintEvent& event);
    virtual void OnButton41ButtonClicked(wxCommandEvent& event);
private:
    void DrawTest();
    void PaintNow(wxDC& dc);
    uint8_t m_rom[2*1024*1024];
    wxImage m_img;
    bool m_loaded;
};
#endif // MAINFRAME_H
