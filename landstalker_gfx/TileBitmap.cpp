#include "TileBitmap.h"
#include <wx/wx.h>
#include <wx/dcclient.h>

TileBitmap::TileBitmap()
: drawn_(false),
  size_(0)
{
    bmp = new wxBitmap();
    bmp->Create(1,1);
    memDC.SelectObject(*bmp);
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

// Draw an internal bitmap containing the tilemap.
// Will be arranged in a 4 x N grid of 8x8 tiles. Each column is a different tile.
// Rows contain cached orientations for the tiles (X and Y flips)
void TileBitmap::setPalette(const Palette& palette)
{
    uint8_t* rgb_ptr = rgb_pixels;
    uint8_t* alpha_ptr = alpha_pixels;

    memDC.SelectObject(wxNullBitmap);

    // 4 orientations: normal, X-Flip, Y-Flip, XY-Flip
    for(size_t o = 0; o < 4; ++o)
    {
        // Eight lines per tile
        for(size_t y = 0; y < HEIGHT; ++y)
        {
            // For each unique tile
            for(size_t t = 0; t < N_TILES; ++t)
            {
                // Eight pixels per line
                for(size_t x = 0; x < WIDTH; ++x)
                {
                    size_t p = t * HEIGHT * WIDTH;
                    if(o & 1) // Horizontal flip
                    {
                        p += WIDTH - 1 - x;
                    }
                    else // No HFLIP
                    {
                        p += x;
                    }
                    if(o & 2) // Vertical flip
                    {
                        p += HEIGHT * WIDTH - (y + 1) * HEIGHT;
                    }
                    else // No VFLIP
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
    img_.SetData(rgb_pixels, WIDTH*N_TILES, HEIGHT*4, true);
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
        size_t h_offset = 0;
        if( attrs.getAttribute(TileAttributes::ATTR_VFLIP))
        {
            h_offset += 2 * HEIGHT;
        }
        if(attrs.getAttribute(TileAttributes::ATTR_HFLIP))
        {
            h_offset += HEIGHT;
        }
        //dc.StretchBlit(left, top, width, height, &memDC, 0, 0, WIDTH, HEIGHT, wxCOPY, true, 0, 0);
        dc.Blit(x*WIDTH, y*HEIGHT, WIDTH, HEIGHT, &memDC, tilenum*WIDTH, h_offset, wxCOPY, true);
    }
    return drawn_;
}

size_t TileBitmap::size() const
{
    return size_;
}
