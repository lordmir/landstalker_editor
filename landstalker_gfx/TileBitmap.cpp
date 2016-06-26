#include "TileBitmap.h"
#include <wx/wx.h>
#include <wx/dcclient.h>

TileBitmap::TileBitmap()
: drawn_(false)
{
}

void TileBitmap::setBits(const uint8_t* src)
{
    for(size_t i = 0; i < N_PIXELS / 2; ++i)
    {
        tilepixels_[i*2] = *src >> 4;
        tilepixels_[i*2 + 1] = *src++ & 0x0F;
    }
}

void TileBitmap::setPalette(const Palette& palette)
{
    uint8_t* rgb_ptr = rgb_pixels;
    uint8_t* alpha_ptr = alpha_pixels;
    for(size_t i = 0; i < N_PIXELS; ++i)
    {
        *rgb_ptr++ = palette.getR(tilepixels_[i]);
        *rgb_ptr++ = palette.getG(tilepixels_[i]);
        *rgb_ptr++ = palette.getB(tilepixels_[i]);
        *alpha_ptr++ = palette.getA(tilepixels_[i]);
    }
    img_.SetData(rgb_pixels, WIDTH, HEIGHT, true);
    img_.SetAlpha(alpha_pixels, true);
    drawn_ = true;
}

bool TileBitmap::draw(wxDC& dc, size_t x, size_t y, const TileAttributes& attrs)
{
    if(drawn_)
    {
        if(img_.IsOk())
        { 
            wxMemoryDC memDC;
            wxBitmap* bmp;
            
            if(attrs.getAttribute(TileAttributes::ATTR_HFLIP))
            {
                img_ = img_.Mirror(true);
            }
            if( attrs.getAttribute(TileAttributes::ATTR_VFLIP))
            {
                img_ = img_.Mirror(false);
            }
            bmp = new wxBitmap(img_);
            memDC.SelectObject(*bmp);
            dc.Blit(x*WIDTH, y*HEIGHT, WIDTH, HEIGHT, &memDC, 0, 0, wxCOPY, true);
            memDC.SelectObject(wxNullBitmap);
            delete bmp;
        }
    }
    return drawn_;
}