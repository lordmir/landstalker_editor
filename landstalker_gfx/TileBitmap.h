#ifndef TILEBITMAP_H
#define TILEBITMAP_H

#include <memory>
#include <cstdint>
#include <map>
#include <wx/wx.h>
#include <wx/dcclient.h>
#include "wxcrafter.h"
#include "TileAttributes.h"
#include "Palette.h"
    
class TileBitmap
{
public:
    TileBitmap();
    ~TileBitmap();
    
    void setBits(const uint8_t* src, size_t numTiles);
    void setPalette(const Palette& palette);
    bool draw(wxDC& dc, size_t tile_num, size_t x, size_t y, const TileAttributes& attrs);
    size_t size() const;
private:
    void update();
    
    static const size_t WIDTH = 8;
    static const size_t HEIGHT = 8;
    static const size_t N_TILES = 0x400;
    static const size_t N_PIXELS = WIDTH * HEIGHT * N_TILES;
    static const size_t N_CACHED_PIXELS = N_PIXELS * 4;
    static const size_t BPP = N_CACHED_PIXELS * 3;
    
    uint8_t tilepixels_[N_PIXELS];
    uint8_t rgb_pixels[BPP];
    uint8_t alpha_pixels[N_CACHED_PIXELS];
    bool drawn_;
    wxImage img_;
    size_t size_;
    
    wxMemoryDC memDC;
    wxBitmap* bmp;
};

#endif // TILEBITMAP_H
