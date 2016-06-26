#ifndef TILEBITMAP_H
#define TILEBITMAP_H

#include <memory>
#include <cstdint>
#include <map>
#include "wxcrafter.h"
#include "TileAttributes.h"
#include "Palette.h"
    
class TileBitmap
{
public:
    TileBitmap();
    
    void setBits(const uint8_t* src);
    void setPalette(const Palette& palette);
    bool draw(wxDC& dc, size_t x, size_t y, const TileAttributes& attrs);
private:
    void update();
    
    static const size_t WIDTH = 8;
    static const size_t HEIGHT = 8;
    static const size_t N_PIXELS = WIDTH * HEIGHT;
    static const size_t BPP = N_PIXELS * 3;
    
    uint8_t tilepixels_[N_PIXELS];
    uint8_t rgb_pixels[BPP];
    uint8_t alpha_pixels[N_PIXELS];
    bool drawn_;
    wxImage img_;
};

#endif // TILEBITMAP_H
