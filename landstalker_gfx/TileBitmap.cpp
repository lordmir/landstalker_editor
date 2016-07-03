#include "TileBitmap.h"
#include <wx/wx.h>
#include <wx/dcclient.h>

TileBitmap::TileBitmap()
: drawn_(false),
  size_(0)
{
}

TileBitmap::~TileBitmap()
{
    delete bmp;
}

void TileBitmap::setBits(const uint8_t* src, size_t num_tiles)
{
    memDC.SelectObject(wxNullBitmap);
    size_ = num_tiles;
    for(size_t i = 0; i < N_PIXELS / 2; ++i)
    {
        tilepixels_[i*2] = *src >> 4;
        tilepixels_[i*2 + 1] = *src++ & 0x0F;
    }
    drawn_ = false;
}

void TileBitmap::setPalette(const Palette& palette)
{
    uint8_t* rgb_ptr = rgb_pixels;
    uint8_t* alpha_ptr = alpha_pixels;
    
    memDC.SelectObject(wxNullBitmap);
    
    for(size_t o = 0; o < 4; ++o)
    {
        for(size_t t = 0; t < N_TILES; ++t)
        {
            for(size_t y = 0; y < HEIGHT; ++y)
            {
                for(size_t x = 0; x < WIDTH; ++x)
                {
                    size_t p = t * HEIGHT * WIDTH;
                    if(o & 1)
                    {
                        p += WIDTH - 1 - x;
                    }
                    else
                    {
                        p += x;
                    }
                    if(o & 2)
                    {
                        p += HEIGHT * WIDTH - (y + 1) * HEIGHT;
                    }
                    else
                    {
                        p += y * HEIGHT;
                    }
                    *rgb_ptr++ = palette.getR(tilepixels_[p]);
                    *rgb_ptr++ = palette.getG(tilepixels_[p]);
                    *rgb_ptr++ = palette.getB(tilepixels_[p]);
                    *alpha_ptr++ = palette.getA(tilepixels_[p]);
                }
            }
        }
    }
    
    img_.SetData(rgb_pixels, WIDTH, HEIGHT*N_TILES*4, true);
    img_.SetAlpha(alpha_pixels, true);
    if(bmp)
    {
        delete bmp;
    }
    bmp = new wxBitmap(img_);
    memDC.SelectObject(*bmp);
    drawn_ = true;
}

bool TileBitmap::draw(wxDC& dc, size_t tilenum, size_t x, size_t y, const TileAttributes& attrs)
{
    if(drawn_)
    {
        tilenum %= N_TILES;
        if( attrs.getAttribute(TileAttributes::ATTR_VFLIP))
        {
            tilenum += N_TILES * 2;
        }
        if(attrs.getAttribute(TileAttributes::ATTR_HFLIP))
        {
            tilenum += N_TILES;
        }
        //dc.StretchBlit(left, top, width, height, &memDC, 0, 0, WIDTH, HEIGHT, wxCOPY, true, 0, 0);
        dc.Blit(x*WIDTH, y*HEIGHT, WIDTH, HEIGHT, &memDC, 0, tilenum*HEIGHT, wxCOPY, true);
    }
    return drawn_;
}

size_t TileBitmap::size() const
{
    return size_;
}