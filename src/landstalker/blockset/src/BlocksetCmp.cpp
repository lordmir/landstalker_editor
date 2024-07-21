#include <landstalker/blockset/include/BlocksetCmp.h>

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
#include <cassert>
#include <stdexcept>
#include <landstalker/misc/include/BitBarrel.h>
#include <landstalker/misc/include/BitBarrelWriter.h>
#include <landstalker/blockset/include/Block.h>
#include <landstalker/tileset/include/TileAttributes.h>

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
        d.push_front(static_cast<T>(x));
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
static std::ostream& operator<< (std::ostream& str, const TileQueue<T1, N1>& rhs)
{
    std::copy (rhs.d.begin(), rhs.d.end(), std::ostream_iterator<uint16_t>(str, ":"));
    return str;
}

static unsigned int ilog2(unsigned int in)
{
    unsigned int ret = 0;

    while (in) {
        in >>= 1;
        ret++;
    }

    return ret;
}

/* Gets compressed variable-width number. Number is in the form 2^Exp + Man */
/* Exp is the number of leading zeroes. The following bits make up the
 * mantissa. The same number of bits make up the exponent and mantissa */
static uint16_t getCompNumber(BitBarrel& bb)
{
    int16_t exponent = 0, mantissa = 0;
    while(bb.getNextBit() == false)
    {
        exponent++;
    }
    if(!exponent) return 0;
    
    uint16_t val = 1 << exponent;
    mantissa = static_cast<int16_t>(bb.readBits(exponent));
    val += mantissa;
    --val;

    return val;
}

static void writeCompNumber(BitBarrelWriter& bb, uint16_t val)
{
    uint16_t exp = static_cast<uint16_t>(ilog2(val) - 1);
    uint16_t i = exp;
    uint16_t num = val - (1 << exp);


    while (i--)
    {
        bb.WriteBits(0, 1);
    }
    bb.WriteBits(1, 1);

    if (exp)
    {
        bb.WriteBits(num, exp);
    }
}

static uint16_t decodeTile(TileQueue<uint16_t, 16>& tq, BitBarrel& bb)
{
    if(bb.getNextBit())
    {
        uint8_t idx = static_cast<uint8_t>(bb.readBits(4));
        if(idx) tq.moveToFront(idx);
    }
    else
    {
        uint16_t val = static_cast<uint16_t>(bb.readBits(11));
        tq.push(val);
    }
    return tq.front();
}

static void decompressTiles(std::vector<Tile>& tiles, BitBarrel& bb)
{
    TileQueue<uint16_t, 16> tq;
    std::vector<Tile>::iterator it;
    for(it = tiles.begin(); it != tiles.end(); it += 2)
    {
        uint16_t tile = decodeTile(tq, bb);
        it->SetIndex(tile);
        if(!bb.getNextBit())
        {
            (it + 1)->SetIndex(decodeTile(tq, bb));
        }
        else
        {
            if(it->Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP))
            {
                (it + 1)->SetIndex(tile - 1);
            }
            else
            {
                (it + 1)->SetIndex(tile + 1);
            }
        }
    }
}

static void maskTiles(std::vector<Tile>& tiles, const TileAttributes::Attribute& attr, BitBarrel& bb)
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
                    it->Attributes().setAttribute(attr);
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

uint16_t BlocksetCmp::Decode(const uint8_t* src, size_t length, Blockset& blocks)
{
    BitBarrel bb(src);
    TileQueue<uint16_t, 16> tq;
    std::vector<Tile> new_tiles;
    
    if (length < 2)
    {
        throw std::runtime_error("Unexpected end of input data");
    }

    const uint16_t TOTAL = static_cast<uint16_t>(bb.readBits(16));
    
    new_tiles.resize(TOTAL * 4);   
    blocks.reserve(blocks.size() + TOTAL);
    
    maskTiles(new_tiles, TileAttributes::Attribute::ATTR_PRIORITY, bb);
    maskTiles(new_tiles, TileAttributes::Attribute::ATTR_VFLIP, bb);
    maskTiles(new_tiles, TileAttributes::Attribute::ATTR_HFLIP, bb);
    
    decompressTiles(new_tiles, bb);
    
    std::ostringstream ss;
    std::vector<Tile>::const_iterator it;

    for(it = new_tiles.begin(); it != new_tiles.end(); it+=4)
    {
        blocks.push_back(MapBlock(it, it+4));
    }

    bb.advanceNextByte();
    return static_cast<uint16_t>(bb.getBytePosition());;
}

static void SetMask(const Blockset& blocks, const TileAttributes::Attribute& attr, BitBarrelWriter& cbs)
{
    bool attr_set = false;
    uint16_t count = 0;
    for (const auto& block : blocks)
    {
        for (size_t t = 0; t < 4; ++t)
        {
            if (block.GetTile(t).Attributes().getAttribute(attr) == attr_set)
            {
                count++;
            }
            else
            {
                attr_set = !attr_set;
                writeCompNumber(cbs, ++count);
                count = 0;
            }
        }
    }
    writeCompNumber(cbs, ++count);
}

static void EncodeTile(TileQueue<uint16_t, 16>& tq, uint16_t tileval, BitBarrelWriter& cbs)
{
    int tq_idx = tq.find(tileval);
    if (tq_idx == -1)
    {
        cbs.WriteBits(tileval, 12);
        tq.push(tileval);
    }
    else
    {
        cbs.WriteBits(1, 1);
        cbs.WriteBits(tq_idx, 4);
        if (tq_idx) tq.moveToFront(tq_idx);
    }
}

static void CompressTiles(const Blockset& blocks, BitBarrelWriter& cbs)
{
    TileQueue<uint16_t, 16> tq;
    for (const auto& block : blocks)
    {
        for (size_t i = 0; i < 4; i += 2)
        {
            uint16_t next = block.GetTile(i).GetIndex();
            EncodeTile(tq, next, cbs);
            if (block.GetTile(i).Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP))
            {
                next--;
            }
            else
            {
                next++;
            }

            if (block.GetTile(i + 1).GetIndex() == next)
            {
                cbs.WriteBits(1, 1);
            }
            else
            {
                cbs.WriteBits(0, 1);
                EncodeTile(tq, block.GetTile(i + 1).GetIndex(), cbs);
            }
        }
    }
}

uint16_t BlocksetCmp::Encode(const Blockset& blocks, uint8_t* dst, size_t bufsize)
{
    BitBarrelWriter cbs;
    // STEP 1: Write out total blocks
    cbs.Write<uint16_t>(static_cast<uint16_t>(blocks.size()));
    // STEP 2: Calculate mask for PRIORITY
    SetMask(blocks, TileAttributes::Attribute::ATTR_PRIORITY, cbs);
    // STEP 3: Calculate mask for VFLIP
    SetMask(blocks, TileAttributes::Attribute::ATTR_VFLIP, cbs);
    // STEP 4: Calculate mask for HFLIP
    SetMask(blocks, TileAttributes::Attribute::ATTR_HFLIP, cbs);
    // STEP 5: Encode tile values
    CompressTiles(blocks, cbs);
    // Done!
    if (cbs.GetByteCount() <= bufsize)
    {
        std::copy(cbs.Begin(), cbs.End(), dst);
    }
    else
    {
        throw std::runtime_error("Output buffer not large enough to hold result.");
    }
    return static_cast<uint16_t>(cbs.GetByteCount());
}
