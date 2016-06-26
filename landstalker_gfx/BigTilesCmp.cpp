#include "BigTilesCmp.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <algorithm>
#include <iterator>
#include <sstream>
#include "BitBarrel.h"
#include "BigTile.h"
#include <wx/msgdlg.h>

template<class T, size_t N>
class TileQueue
{
public:
    TileQueue()
    : d(std::deque<T>(N,0))
    {
    }
    void push(int x)
    {
        d.push_front(x);
        d.erase(d.end() - 1);
    }
    void moveToFront(int x)
    {
        std::rotate(d.begin(), d.begin() + x, d.begin() + x + 1);
    }
    uint16_t front()
    {
        return d.front();
    }
    int find(const T& param)
    {
        typename std::deque<T>::const_iterator it;
        it = std::find(d.begin(), d.end(), param);
        if(it == d.end()) return -1;
        return it - d.begin();
    }
    template <class T1, size_t N1>
    friend std::ostream& operator<< (std::ostream& str, const TileQueue<T1, N1>& rhs);
private:
    std::deque<T> d;  
};

template<class T1, size_t N1>
std::ostream& operator<< (std::ostream& str, const TileQueue<T1, N1>& rhs)
{
    std::copy (rhs.d.begin(), rhs.d.end(), std::ostream_iterator<uint16_t>(str, ":"));
    return str;
}


/* Gets compressed variable-width number. Number is in the form 2^Exp + Man */
/* Exp is the number of leading zeroes. The following bits make up the
 * mantissa. The same number of bits make up the exponent and mantissa */
uint16_t getCompNumber(BitBarrel& bb)
{
    int16_t exponent = 0, mantissa = 0;
    while(bb.getNextBit() == false)
    {
        exponent++;
    }
    if(!exponent) return 0;
    
    uint16_t val = 1 << exponent;
    mantissa = bb.readBits(exponent);
    val += mantissa;
    --val;
   /* std::ostringstream ss;
    ss << "E: " << exponent << " M: " << mantissa << " V: " << val;
    wxMessageBox(ss.str()); */
    return val;
}

uint16_t decodeTile(TileQueue<uint16_t, 16>& tq, BitBarrel& bb)
{
    if(bb.getNextBit())
    {
        uint8_t idx = bb.readBits(4);
        if(idx) tq.moveToFront(idx);
    }
    else
    {
        uint16_t val = bb.readBits(11);
        tq.push(val);
    }
    return tq.front();
}

void decompressTiles(std::vector<Tile>& tiles, BitBarrel& bb)
{
    TileQueue<uint16_t, 16> tq;
    std::vector<Tile>::iterator it;
    for(it = tiles.begin(); it != tiles.end(); it += 2)
    {
        uint16_t tile = decodeTile(tq, bb);
        it->setIndex(tile);
        if(!bb.getNextBit())
        {
            (it + 1)->setIndex(decodeTile(tq, bb));
        }
        else
        {
            if(it->attributes().getAttribute(TileAttributes::ATTR_HFLIP))
            {
                (it + 1)->setIndex(tile - 1);
            }
            else
            {
                (it + 1)->setIndex(tile + 1);
            }
        }
    }
}

void maskTiles(std::vector<Tile>& tiles, const TileAttributes::Attribute& attr, BitBarrel& bb)
{
    uint16_t count = 0;
    std::vector<Tile>::iterator it = tiles.begin();
    bool firstloop = true;
    bool setAttr = false;
    do
    {
        uint16_t num = getCompNumber(bb);
        if(!(firstloop && num == 0))
        {
            if(!firstloop) num++;
            count += num;
            if(setAttr)
            {
                for(int16_t i = 0; i < num ; i++)
                {
                    assert(it != tiles.end());
                    it->attributes().setAttribute(attr);
                    it++;
                }
            }
            else
            {
                std::advance(it, num);
                assert(it <= tiles.end());
            }
        }
        firstloop = false;
        setAttr = !setAttr;
    } while (it != tiles.end());
}

uint16_t BigTilesCmp::Decode(const uint8_t* src, std::vector<BigTile>& tiles)
{
    BitBarrel bb(src);
    TileQueue<uint16_t, 16> tq;
    std::vector<Tile> new_tiles;
    
    const uint16_t TOTAL = bb.readBits(16);
    
    new_tiles.resize(TOTAL * 4);   
    tiles.reserve(tiles.size() + TOTAL);
    
    maskTiles(new_tiles, TileAttributes::ATTR_PRIORITY, bb);
    maskTiles(new_tiles, TileAttributes::ATTR_VFLIP, bb);
    maskTiles(new_tiles, TileAttributes::ATTR_HFLIP, bb);
    
    decompressTiles(new_tiles, bb);
    
    std::ostringstream ss;
    std::vector<Tile>::const_iterator it;

    for(it = new_tiles.begin(); it != new_tiles.end(); it+=4)
    {
        tiles.push_back(BigTile(it, it+4));
    }
    /*
    std::vector<BigTile>::const_iterator bit;
    for(bit = tiles.begin(); bit != tiles.end(); ++bit)
    {
        ss << bit->print() << std::endl;
    }
    wxMessageBox(ss.str());
    */
    return TOTAL;
}
