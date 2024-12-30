#include <landstalker/3d_maps/include/Tilemap3DCmp.h>

#include <stdexcept>
#include <unordered_map>
#include <set>
#include <map>
#include <functional>
#include <cstdint>
#include <algorithm>
#include <iostream>

#include <landstalker/misc/include/BitBarrel.h>
#include <landstalker/misc/include/BitBarrelWriter.h>
#include <landstalker/misc/include/Literals.h>

static uint16_t getCodedNumber(BitBarrel& bb)
{
    uint16_t exp = 0, num = 0;
    
    while(!bb.getNextBit())
    {
        exp++;
    }
    
    if(exp)
    {
        num = 1 << exp;
        num += static_cast<uint16_t>(bb.readBits(exp));
    }
    
    return num;
}

static uint16_t ilog2(uint16_t num)
{
    uint16_t ret = 0;
    while(num)
    {
        num >>= 1;
        ret++;
    }
    return ret;
}

bool Tilemap3D::operator==(const Tilemap3D& rhs) const
{
    return ((this->height == rhs.height) &&
        (this->width == rhs.width) &&
        (this->left == rhs.left) &&
        (this->top == rhs.top) &&
        (this->hmwidth == rhs.hmwidth) &&
        (this->hmheight == rhs.hmheight) &&
        (this->heightmap == rhs.heightmap) &&
        (this->foreground == rhs.foreground) &&
        (this->background == rhs.background));
}

bool Tilemap3D::operator!=(const Tilemap3D& rhs) const
{
    return !(*this == rhs);
}

uint16_t Tilemap3D::Decode(const uint8_t* src)
{
    BitBarrel bb(src);
    foreground.clear();
    background.clear();
    heightmap.clear();

    left   = static_cast<uint8_t>(bb.readBits(8));
    top    = static_cast<uint8_t>(bb.readBits(8));
    width  = static_cast<uint8_t>(bb.readBits(8) + 1);
    height = static_cast<uint8_t>((bb.readBits(8) + 1) / 2);
    
    uint16_t tileDictionary[2] = {0, 0};
    uint16_t offsetDictionary[14] = {0xFFFF,
                                     1,
                                     2,
                                     static_cast<uint16_t>(GetWidth()),
                                     static_cast<uint16_t>(GetWidth() *2u),
                                     static_cast<uint16_t>(GetWidth() + 1),
                                     0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t t = GetSize() * 2;
    std::vector<uint16_t> buffer(t,0);
    
    tileDictionary[1] = static_cast<uint16_t>(bb.readBits(10));
    tileDictionary[0] = static_cast<uint16_t>(bb.readBits(10));
    
    for(size_t i = 6; i < 14; ++i)
    {
        offsetDictionary[i] = static_cast<uint16_t>(bb.readBits(12));
    }
    
    int16_t dst_addr = -1;
    
    while(true)
    {
        uint16_t start = getCodedNumber(bb);
        
        if(!start) start++;
        dst_addr += start;
        
        if(dst_addr >= t)
        {
            break;
        }
        
        uint8_t command = static_cast<uint8_t>(bb.readBits(3));
        if(command > 5)
        {
            command = static_cast<uint8_t>(6 + (((command & 1) << 2) | bb.readBits(2)));
        }
        buffer[dst_addr] = offsetDictionary[command];
        
        if(bb.getNextBit())
        {
            uint16_t row_addr = dst_addr;
            bool width_offset = bb.getNextBit();
            do
            {
                do
                {
                    row_addr += GetWidth() + (width_offset ? 1 : 0);
                    buffer[row_addr] = offsetDictionary[command];
                } while(bb.getNextBit());
                width_offset = !width_offset;
            } while(bb.getNextBit());
        }
    }
    
    uint16_t tiles[2] = {tileDictionary[0], tileDictionary[1]};
    dst_addr = 0;
    do
    {
        uint16_t operand = buffer[dst_addr];
        uint16_t offset;
        if(operand != 0xFFFF)
        {
            offset = dst_addr - operand;
            do
            {
                buffer[dst_addr++] = buffer[offset++];
            } while ((dst_addr < t) && (buffer[dst_addr] == 0));
        }
        else
        {
            do
            {
                operand = static_cast<uint8_t>(bb.readBits(2));
                uint16_t value = 0;
                switch(operand)
                {
                    case 0:
                        if(tiles[0])
                        {
                            value = static_cast<uint16_t>(bb.readBits(ilog2(tiles[0])));
                        }
                        buffer[dst_addr++] = value;
                        break;
                    case 1:
                        if(tiles[1] != tileDictionary[1])
                        {
                            value = static_cast<uint16_t>(bb.readBits(ilog2(tiles[1] - tileDictionary[1])));
                        }
                        value += tileDictionary[1];
                        buffer[dst_addr++] = value;
                        break;
                    case 2:
                        value = tiles[0]++;
                        buffer[dst_addr++] = value;
                        break;
                    case 3:
                        value = tiles[1]++;
                        buffer[dst_addr++] = value;
                        break;
                }
            } while ((dst_addr < t) && (buffer[dst_addr] == 0));
        }
    } while(dst_addr < t);
    
    background.resize(GetSize());
    foreground.resize(GetSize());
    std::copy(buffer.begin() + t/2, buffer.end(), background.begin());
    std::copy(buffer.begin(), buffer.begin() + t / 2, foreground.begin());
    
    bb.advanceNextByte();
    hmwidth = static_cast<uint8_t>(bb.readBits(8));
    hmheight = static_cast<uint8_t>(bb.readBits(8));
    
    uint16_t hm_pattern = 0;
    uint16_t hm_rle_count = 0;
    
    heightmap.assign(GetHeightmapSize(), 0);
    dst_addr = 0;
    for(size_t y = 0; y < GetHeightmapHeight(); y++)
    {
        for(size_t x = 0; x < GetHeightmapWidth(); x++)
        {
            if(!hm_rle_count--)
            {
                uint8_t read_count = 0;
                hm_rle_count = 0;
                hm_pattern = static_cast<uint16_t>(bb.readBits(16));
                do
                {
                    read_count = static_cast<uint8_t>(bb.readBits(8));
                    hm_rle_count += read_count;
                } while(read_count == 0xFF);
            }
            heightmap[dst_addr++] = hm_pattern;
        }
    }
    bb.advanceNextByte();
    return static_cast<uint16_t>(bb.getBytePosition());
}

void makeCodedNumber(uint16_t value, BitBarrelWriter& bb)
{
    uint16_t exp = ilog2(value) - 1;
    uint16_t i = exp;
    uint16_t num = value - (1 << exp);

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

int findMatchFrequency(const std::vector<uint16_t>& input, size_t offset, std::unordered_map<int, int>& fc)
{
    size_t lookback_size = std::min<size_t>(offset, 4095);
    size_t lookahead_size = input.size() - offset;
    int best = 0;
    for (size_t b = 1; b <= lookback_size; ++b)
    {
        int match_run = 0;
        for (size_t m = 0; m < lookahead_size; ++m)
        {
            if (input[offset - b + m] != input[offset + m])
            {
                break;
            }
            match_run++;
        }
        if (match_run > best)
        {
            best = match_run;
        }
    }
    if (best < 2) return 0;
    for (size_t b = 1; b <= lookback_size; ++b)
    {
        int match_run = 0;
        for (size_t m = 0; m < lookahead_size; ++m)
        {
            if (input[offset - b + m] != input[offset + m])
            {
                break;
            }
            match_run++;
        }
        if (match_run == best)
        {
            fc[b] ++;
        }
    }
    return best;
}

std::pair<int, int> findMatch(const std::vector<uint16_t>& input, size_t offset, const std::vector<uint16_t>& back_offsets)
{
    size_t lookback_size = std::min<size_t>(offset, 4095);
    size_t lookahead_size = input.size() - offset;
    std::pair<int, int> ret = { 0,0 };
    for (size_t i = 0; i < back_offsets.size(); ++i)
    {
        size_t b = back_offsets[i];
        if ((b == 0) || (b > lookback_size)) continue;
        int match_run = 0;
        for (size_t m = 0; m < lookahead_size; ++m)
        {
            if (input[offset - b + m] != input[offset + m])
            {
                break;
            }
            match_run++;
        }
        if (match_run > ret.second)
        {
            ret.second = match_run;
            ret.first = i;
        }
    }
    if (ret.second == 0)
    {
        ret.first = 0;
        ret.second = 1;
    }

    return ret;
}

uint16_t Tilemap3D::Encode(uint8_t* dst, size_t size)
{
    BitBarrelWriter cmap;
    std::vector<uint16_t> offsets = { 0, 1, 2, static_cast<uint16_t>(GetWidth()), static_cast<uint16_t>(GetWidth() * 2), static_cast<uint16_t>(GetWidth() + 1)};
    struct LZ77Entry
    {
        LZ77Entry(int run_length_in, int back_offset_idx_in, int index_in)
            : run_length(run_length_in), back_offset_idx(back_offset_idx_in), index(index_in)
        {}
        int run_length;
        int back_offset_idx;
        int index;
        std::vector<std::pair<bool, int>> vertical_info;
    };
    struct TileEntry
    {
        TileEntry(uint8_t code_in, uint16_t data_in, uint8_t data_length_in)
            : code(code_in), data(data_in), data_length(data_length_in)
        {}
        uint8_t code;
        uint16_t data;
        uint8_t data_length;
    };
    std::vector<LZ77Entry> lz77;
    uint16_t tile_dict[2] = { 0 };
    uint16_t tile_increment[2] = { 0 };
    std::vector<TileEntry> tile_entries;
    std::vector<std::pair<int, uint16_t>> hm_buffer;

    // COMPRESS MAP
    // Combine foreground and background
    std::vector<uint16_t> tiles(GetSize() * 2);
    std::copy(foreground.begin(), foreground.end(), tiles.begin());
    std::copy(background.begin(), background.end(), tiles.begin() + GetSize());
    // First stage of map compression involves LZ77 with a fixed-size dictionary
    // STEP 1: Run map through LZ77 compressor. Make a list of LZ77 offset frequencies.
    std::unordered_map<int, int> offset_freq_count;
    std::vector<bool> compressed(tiles.size(), false);
    size_t idx = 1;
    do
    {
        int run = findMatchFrequency(tiles, idx, offset_freq_count);
        if (run == 0)
        {
            idx++;
        }
        else
        {
            idx += run;
        }
    } while (idx < tiles.size());

    // STEP 2: Identify top 8 back offsets and add to back offset dictionary

    typedef std::function<bool(const std::pair<int, int>&, const std::pair<int, int>&)> Comparator;
    Comparator comparator = [](const std::pair<int, int>& p1, const std::pair<int, int>& p2)
    {
        if (p1.second != p2.second)
        {
            return p1.second > p2.second;
        }
        else
        {
            return p1.first < p2.first;
        }
    };

    std::multiset<std::pair<int, int>, Comparator> frequency_counts(offset_freq_count.begin(), offset_freq_count.end(), comparator);
    for (auto it = frequency_counts.cbegin(); it != frequency_counts.cend(); ++it)
    {
#ifndef NDEBUG
        std::cout << "Offset " << std::hex << it->first << ": " << std::dec << it->second << std::endl;
#endif
        if (std::find(offsets.begin(), offsets.end(), it->first) == offsets.end())
        {
            offsets.push_back(static_cast<uint16_t>(it->first));
        }
    }
    offsets.resize(14);
#ifndef NDEBUG
    for (size_t i = 0; i < offsets.size(); ++i)
    {
        std::cout << "Offset " << std::hex << offsets[i] << ": " << std::dec << offset_freq_count[offsets[i]] << std::endl;
    }
#endif
    // STEP 3: Compress map using LZ77 and the back offset dictionary created during step 2

    lz77.emplace_back(1, 0, 0);
    idx = 1;
    do
    {
        auto result = findMatch(tiles, idx, offsets);
        if ((result.first != 0) || (lz77.back().back_offset_idx != 0))
        {
            lz77.emplace_back(result.second, result.first, idx);
        }
        else
        {
            lz77.back().run_length++;
        }
        if (lz77.back().back_offset_idx == 0)
        {
#ifndef NDEBUG
            std::cout << " LOAD TILE @ " << idx << std::endl;
#endif
            compressed[idx] = false;
            idx++;
        }
        else
        {
#ifndef NDEBUG
            std::cout << " LZ77 @ " << idx << " : copy " << lz77.back().run_length
                      << " bytes from offset " << lz77.back().back_offset_idx << "("
                      << (idx - offsets[lz77.back().back_offset_idx]) << ")" << std::endl;
#endif
            std::fill(compressed.begin() + idx, compressed.begin() + idx + lz77.back().run_length, true);
            idx += lz77.back().run_length;
        }
    } while (idx < tiles.size());

    // STEP 5: For each LZ77 operation, scan downwards and down-right in 2D, and combine entries
    for (auto it = lz77.begin(); it != lz77.end(); ++it)
    {
        size_t count = 0;
        bool right = false;
        bool begin = true;
        if (it->back_offset_idx != -1)
        {
            size_t next = it->index;
            size_t prev = next;
            while (next < tiles.size())
            {
                next += GetWidth() + (right ? 1 : 0);
                auto nit = std::find_if(it, lz77.end(), [&](const LZ77Entry& comp)
                    {
                        return (comp.index == static_cast<int>(next)) && (comp.back_offset_idx == it->back_offset_idx);
                    });
                if (nit != lz77.end())
                {
                    count++;
                    nit->back_offset_idx = -1;
                    prev = next;
                }
                else
                {
                    if (count > 0)
                    {
                        it->vertical_info.emplace_back(right, count);
                        count = 0;
                    }
                    else
                    {
                        if (begin == false)
                        {
                            break;
                        }
                    }
                    begin = false;
                    right = !right;
                    next = prev;
                }
            }
        }
    }

    auto it = std::remove_if(lz77.begin(), lz77.end(), [](const LZ77Entry& comp)
        {
            return comp.back_offset_idx == -1;
        });
    lz77.erase(it, lz77.end());

    // STEP 6: Identify sequential runs of tiles, the longest run including decrements will be
    //         stored in element 1 of the tile dict. The longest run where ilog2(base) == ilog2(highest tile #)
    //         will be stored in element 2 of the tile dict.

    std::map<uint16_t, int> incrementing_tile_counts;
    std::map<uint16_t, int> ranged_tile_counts;
    for (size_t i = 0; i < tiles.size(); ++i)
    {
        if (compressed[i] == false)
        {
            for (auto& itc : incrementing_tile_counts)
            {
                if (tiles[i] == itc.first + itc.second)
                {
                    itc.second++;
                }
                if ((tiles[i] >= itc.first) && (tiles[i] < itc.first + itc.second))
                {
                    ranged_tile_counts[tiles[i]]++;
                }
            }
            if (incrementing_tile_counts.find(tiles[i]) == incrementing_tile_counts.end())
            {
                incrementing_tile_counts[tiles[i]] = 1;
            }
        }
    }
    std::multiset<std::pair<uint16_t, int>, Comparator> incrementing_tile_freqs(
        incrementing_tile_counts.begin(), incrementing_tile_counts.end(), comparator);
    uint16_t max_tile = incrementing_tile_counts.rbegin()->first;
    uint16_t min_dict_entry = 1 << (ilog2(max_tile) - 1);
    tile_dict[1] = 0;
    for (auto& irt : incrementing_tile_counts)
    {
        if ((tile_dict[1] == 0) && (irt.first >= min_dict_entry))
        {
            tile_dict[1] = irt.first;
            irt.second = 0;
        }
        else
        {
            irt.second *= 4;
            irt.second += ranged_tile_counts[irt.first];
        }
    }
    if (tile_dict[1] == 0)
    {
        tile_dict[1] = min_dict_entry;
    }
    std::multiset<std::pair<uint16_t, int>, Comparator> scored_tile_freqs(
        incrementing_tile_counts.begin(), incrementing_tile_counts.end(), comparator);

    tile_dict[0] = incrementing_tile_freqs.begin()->first;
#ifndef NDEBUG
    std::cout << "TILE DICT 1: " << std::hex << tile_dict[1] << std::endl;
    std::cout << "TILE DICT 0: " << std::hex << tile_dict[0] << std::endl;
#endif

    // STEP 7: Start to compress tile data. Identify if tile is (1) equal to any in tile dictionary + increment,
    //         (2) between tileDict[0] and tileDict[0] + tileDictIncr[0], or (3) none of the above.

    for (size_t i = 0; i < tiles.size(); ++i)
    {
        if (compressed[i] == false)
        {
            if (tiles[i] == tile_dict[0] + tile_increment[0])
            {
                tile_increment[0]++;
#ifndef NDEBUG
                std::cout << "INCREMENT TILE 1 [" << std::hex << tiles[i] << " @ " << std::dec << i << std::endl;
#endif
                tile_entries.emplace_back(3_u8, 0_u16, 0_u8);
            }
            else if (tiles[i] == tile_dict[1] + tile_increment[1])
            {
                tile_increment[1]++;
#ifndef NDEBUG
                std::cout << "INCREMENT TILE 2 [" << std::hex << tiles[i] << " @ " << std::dec << i << std::endl;
#endif
                tile_entries.emplace_back(2_u8, 0_u16, 0_u8);
            }
            else if ((tiles[i] >= tile_dict[0]) && (tiles[i] < (tile_dict[0] + tile_increment[0])))
            {
#ifndef NDEBUG
                std::cout << "PLACE REL TILE [" << std::hex << tiles[i] << " @ " << std::dec << i << std::endl;
#endif
                tile_entries.emplace_back(1_u8, static_cast<uint16_t>(tiles[i] - tile_dict[0]), static_cast<uint8_t>(ilog2(tile_increment[0])));
            }
            else
            {
#ifndef NDEBUG
                std::cout << "PLACE TILE " << std::hex << tiles[i] << " @ " << std::dec << i << std::endl;
#endif
                tile_entries.emplace_back(0_u8, tiles[i], static_cast<uint8_t>(ilog2(tile_dict[1])));
            }
        }
    }

    // STEP 8: Map compression complete, compress heightmap. For heightmap data, compress as RLE.

    for (size_t i = 0; i < GetHeightmapSize(); ++i)
    {
        if (hm_buffer.empty() || hm_buffer.back().second != heightmap[i])
        {
            hm_buffer.emplace_back(0, heightmap[i]);
        }
        else
        {
            hm_buffer.back().first++;
        }
    }

    // STEP 9: Compression complete! Begin output:

    // Top, Left, Width, Height
    cmap.Write<uint8_t>(GetLeft());
    cmap.Write<uint8_t>(GetTop());
    cmap.Write<uint8_t>(GetWidth() - 1);
    cmap.Write<uint8_t>(GetHeight() * 2 - 1);
    // Tile Dictionary
    cmap.WriteBits(tile_dict[0], 10);
    cmap.WriteBits(tile_dict[1], 10);
    // Offset Dictionary
    for (size_t i = 0; i < 8; ++i)
    {
        cmap.WriteBits(offsets[6 + i], 12);
    }
    // LZ77 Data: run length, back offset index, vertical run data
    int last_idx = -1;
    for (const auto& entry : lz77)
    {
        // Run length
        makeCodedNumber(static_cast<uint16_t>(entry.index - last_idx), cmap);
        last_idx = entry.index;
        // Back index
        if (entry.back_offset_idx < 6)
        {
            cmap.WriteBits(entry.back_offset_idx, 3);
        }
        else
        {
            cmap.WriteBits(3, 2);
            cmap.WriteBits(entry.back_offset_idx - 6, 3);
        }
        //Vertical run
        if (!entry.vertical_info.empty())
        {
            cmap.WriteBits(1, 1);

            bool begin = true;
            for (const auto& v : entry.vertical_info)
            {
                if (begin)
                {
                    cmap.WriteBits(entry.vertical_info[0].first, 1);
                    begin = false;
                }
                else
                {
                    cmap.WriteBits(1, 1);
                }
                for (int i = 1; i < v.second; ++i)
                {
                    cmap.WriteBits(1, 1);
                }
                cmap.WriteBits(0, 1);
            }
            cmap.WriteBits(0, 1);
        }
        else
        {
            cmap.WriteBits(0, 1);
        }
    }
    if (last_idx < static_cast<int>(tiles.size()))
    {
        makeCodedNumber(static_cast<uint16_t>(tiles.size() - last_idx + 1), cmap);
    }
    else
    {
        makeCodedNumber(1, cmap);
    }

    // Tile data: 2-bit operand + variable length data

    for (const auto& entry : tile_entries)
    {
        cmap.WriteBits(entry.code, 2);
        switch (entry.code)
        {
        case 0:
        case 1:
            cmap.WriteBits(entry.data, entry.data_length);
            break;
        default:
            break;
        }
    }
    // Advance to next byte
    cmap.AdvanceNextByte();
    // Heightmap dims
    cmap.Write<uint8_t>(GetHeightmapWidth());
    cmap.Write<uint8_t>(GetHeightmapHeight());
    // Heightmap data: pattern + run length. If run length >= 0xFF, output 0xFF then run length - 0xFF
    for (const auto& entry : hm_buffer)
    {
        cmap.Write<uint16_t>(entry.second);
        int len = entry.first;
        while (len >= 0xFF)
        {
            cmap.Write<uint8_t>(0xFF);
            len -= 0xFF;
        }
        cmap.Write<uint8_t>(static_cast<uint8_t>(len));
    }
    if (cmap.GetByteCount() <= size)
    {
        std::copy(cmap.Begin(), cmap.End(), dst);
    }
    else
    {
        throw std::runtime_error("Output buffer not large enough to hold result.");
    }
    return static_cast<uint16_t>(cmap.GetByteCount());
}

uint8_t Tilemap3D::GetLeft() const
{
    return left;
}

uint8_t Tilemap3D::GetTop() const
{
    return top;
}

uint8_t Tilemap3D::GetWidth() const
{
    return width;
}

uint8_t Tilemap3D::GetHeight() const
{
    return height;
}

uint16_t Tilemap3D::GetSize() const
{
    return width * height;
}

void Tilemap3D::Resize(uint8_t w, uint8_t h)
{
    const std::vector<uint16_t> old_bg = background;
    const std::vector<uint16_t> old_fg = foreground;

    background.resize(w * h);
    foreground.resize(w * h);
    
    auto obit = old_bg.cbegin();
    auto ofit = old_fg.cbegin();
    auto nbit = background.begin();
    auto nfit = foreground.begin();
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; j++)
        {
            if ((i < height) && (j < width))
            {
                *nbit++ = *obit++;
                *nfit++ = *ofit++;
            }
            else
            {
                *nbit++ = 0;
                *nfit++ = 0;
            }
        }
        if (w < width)
        {
            obit += width - w;
            ofit += width - w;
        }
    }

    width = w;
    height = h;
}

void Tilemap3D::ResizeHeightmap(uint8_t w, uint8_t h)
{
    const std::vector<uint16_t> old_hm = heightmap;

    heightmap.resize(w * h);

    auto ohit = old_hm.cbegin();
    auto nhit = heightmap.begin();
    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; j++)
        {
            if ((i < height) && (j < width))
            {
                *nhit++ = *ohit++;
            }
            else
            {
                *nhit++ = 0x4000;
            }
        }
        if (w < width)
        {
            ohit += width - w;
        }
    }

    hmwidth = w;
    hmheight = h;
}

void Tilemap3D::InsertHeightmapColumn(uint8_t before)
{
    if (before < hmheight && hmheight < 64)
    {
        const std::vector<uint16_t> orig = heightmap;
        hmheight += 1;
        heightmap.resize(hmwidth * hmheight);
        auto src = orig.data();
        auto dst = heightmap.data();
        int idx = 0;
        for (int y = 0; y < hmheight; ++y)
        {
            for (int x = 0; x < hmwidth; ++x)
            {
                if (y <= before)
                {
                    dst[idx] = src[idx];
                    ++idx;
                }
                else
                {
                    dst[idx] = src[idx - hmwidth];
                    ++idx;
                }
            }
        }
    }
}

void Tilemap3D::InsertHeightmapRow(uint8_t before)
{
    if (before < hmwidth && hmwidth < 64)
    {
        const std::vector<uint16_t> orig = heightmap;
        hmwidth += 1;
        heightmap.resize(hmwidth * hmheight);
        auto src = orig.data();
        auto dst = heightmap.data();
        for (int y = 0; y < hmheight; ++y)
        {
            for (int x = 0; x < hmwidth; ++x)
            {
                if (x <= before)
                {
                    dst[y * hmwidth + x] = src[y * (hmwidth - 1) + x];
                }
                else
                {
                    dst[y * hmwidth + x] = src[y * (hmwidth - 1) + x - 1];
                }
            }
        }
    }
}

void Tilemap3D::DeleteHeightmapColumn(uint8_t row)
{
    if (row < hmheight && hmheight > 1)
    {
        std::vector<uint16_t> orig = heightmap;
        hmheight -= 1;
        heightmap.resize(hmwidth * hmheight);
        auto src = orig.data();
        auto dst = heightmap.data();
        for (int y = 0; y < hmheight; ++y)
        {
            for (int x = 0; x < hmwidth; ++x)
            {
                if (y < row)
                {
                    dst[y * hmwidth + x] = src[y * hmwidth + x];
                }
                else
                {
                    dst[y * hmwidth + x] = src[(y + 1) * hmwidth + x];
                }
            }
        }
    }
}

void Tilemap3D::ClearTilemap()
{
    std::fill(foreground.begin(), foreground.end(), 0_u16);
    std::fill(background.begin(), background.end(), 0_u16);
}

void Tilemap3D::InsertTilemapRow(int row)
{
    if (row < width && width < 64)
    {
        const std::vector<uint16_t> orig_fg = foreground;
        const std::vector<uint16_t> orig_bg = background;
        width += 1;
        foreground.resize(width * height);
        background.resize(width * height);
        auto fg_src = orig_fg.data();
        auto fg_dst = foreground.data();
        auto bg_src = orig_bg.data();
        auto bg_dst = background.data();
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (x <= row)
                {
                    fg_dst[y * width + x] = fg_src[y * (width - 1) + x];
                    bg_dst[y * width + x] = bg_src[y * (width - 1) + x];
                }
                else
                {
                    fg_dst[y * width + x] = fg_src[y * (width - 1) + x - 1];
                    bg_dst[y * width + x] = bg_src[y * (width - 1) + x - 1];
                }
            }
        }
    }
}

void Tilemap3D::InsertTilemapColumn(int col)
{
    if (col < height && height < 64)
    {
        const std::vector<uint16_t> orig_fg = foreground;
        const std::vector<uint16_t> orig_bg = background;
        height += 1;
        foreground.resize(width * height);
        background.resize(width * height);
        auto fg_src = orig_fg.data();
        auto fg_dst = foreground.data();
        auto bg_src = orig_bg.data();
        auto bg_dst = background.data();
        int idx = 0;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (y <= col)
                {
                    fg_dst[idx] = fg_src[idx];
                    bg_dst[idx] = bg_src[idx];
                    ++idx;
                }
                else
                {
                    fg_dst[idx] = fg_src[idx - width];
                    bg_dst[idx] = bg_src[idx - width];
                    ++idx;
                }
            }
        }
    }
}

void Tilemap3D::DeleteTilemapRow(int row)
{
    if (row < width && width > 1)
    {
        std::vector<uint16_t> orig_fg = foreground;
        std::vector<uint16_t> orig_bg = background;
        width -= 1;
        foreground.resize(width * height);
        const uint16_t* fg_src = orig_fg.data();
        uint16_t* fg_dst = foreground.data();
        const uint16_t* bg_src = orig_bg.data();
        uint16_t* bg_dst = background.data();
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (x < row)
                {
                    fg_dst[y * width + x] = fg_src[y * (width + 1) + x];
                    bg_dst[y * width + x] = bg_src[y * (width + 1) + x];
                }
                else
                {
                    fg_dst[y * width + x] = fg_src[y * (width + 1) + x + 1];
                    bg_dst[y * width + x] = bg_src[y * (width + 1) + x + 1];
                }
            }
        }
    }
}

void Tilemap3D::DeleteTilemapColumn(int col)
{
    if (col < height && height > 1)
    {
        std::vector<uint16_t> orig_fg = foreground;
        std::vector<uint16_t> orig_bg = background;
        height -= 1;
        foreground.resize(width * height);
        background.resize(width * height);
        auto fg_src = orig_fg.data();
        auto fg_dst = foreground.data();
        auto bg_src = orig_bg.data();
        auto bg_dst = background.data();
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (y < col)
                {
                    fg_dst[y * width + x] = fg_src[y * width + x];
                    bg_dst[y * width + x] = bg_src[y * width + x];
                }
                else
                {
                    fg_dst[y * width + x] = fg_src[(y + 1) * width + x];
                    bg_dst[y * width + x] = bg_src[(y + 1) * width + x];
                }
            }
        }
    }
}

void Tilemap3D::DeleteHeightmapRow(uint8_t col)
{
    if (col < hmwidth && hmwidth > 1)
    {
        std::vector<uint16_t> orig = heightmap;
        hmwidth -= 1;
        heightmap.resize(hmwidth * hmheight);
        const uint16_t* src = orig.data();
        uint16_t* dst = heightmap.data();
        for (int y = 0; y < hmheight; ++y)
        {
            for (int x = 0; x < hmwidth; ++x)
            {
                if (x < col)
                {
                    dst[y * hmwidth + x] = src[y * (hmwidth + 1) + x];
                }
                else
                {
                    dst[y * hmwidth + x] = src[y * (hmwidth + 1) + x + 1];
                }
            }
        }
    }
}

void Tilemap3D::SetLeft(uint8_t pleft)
{
    this->left = pleft;
}

void Tilemap3D::SetTop(uint8_t ptop)
{
    this->top = ptop;
}

uint8_t Tilemap3D::GetHeightmapWidth() const
{
    return hmwidth;
}

uint8_t Tilemap3D::GetHeightmapHeight() const
{
    return hmheight;
}

uint16_t Tilemap3D::GetHeightmapSize() const
{
    return hmwidth * hmheight;
}

uint8_t Tilemap3D::GetTileWidth() const
{
    return tile_width;
}

uint8_t Tilemap3D::GetTileHeight() const
{
    return tile_height;
}

void Tilemap3D::SetTileDims(uint8_t tw, uint8_t th)
{
    tile_width = tw;
    tile_height = th;
}

std::size_t Tilemap3D::GetCartesianWidth() const
{
    return (GetWidth() + GetHeight() + GetLeft()) * 2;
}

std::size_t Tilemap3D::GetCartesianHeight() const
{
    return (GetWidth() + GetHeight() + GetTop() + 1);
}

std::size_t Tilemap3D::GetPixelWidth() const
{
    return GetCartesianWidth() * tile_width;
}

std::size_t Tilemap3D::GetPixelHeight() const
{
    return GetCartesianHeight() * tile_height;
}

bool Tilemap3D::IsIsoPointValid(const IsoPoint2D& iso) const
{
    return ((iso.x >= 0 && iso.x < GetWidth()) &&
        (iso.y >= 0 && iso.y < GetHeight()));
}

bool Tilemap3D::IsPointValid(const Point2D& p, Layer layer) const
{
    const auto iso = ToIsometric(p, layer);
    return IsIsoPointValid(iso);
}

bool Tilemap3D::IsPixelPointValid(const PixelPoint2D& pix, Layer layer) const
{
    const auto iso = PixelToIsometric(pix, layer);
    return IsIsoPointValid(iso);
}

bool Tilemap3D::IsHMPointValid(const HMPoint2D& p) const
{
    return ((p.x >= 0 && p.x < hmwidth) &&
            (p.y >= 0 && p.y < hmheight));
}

Point2D Tilemap3D::IsoToCartesian(const IsoPoint2D& iso, Layer /*layer*/) const
{
    if (IsIsoPointValid(iso) == false) return { -1, -1 };
    return Point2D{-1, -1};
}

Point2D Tilemap3D::PixelToCartesian(const PixelPoint2D& pix, Layer layer) const
{
    if (IsPixelPointValid(pix, layer) == false) return { -1, -1 };
    return Point2D{ -1, -1 };
}

IsoPoint2D Tilemap3D::ToIsometric(const Point2D& p, Layer layer) const
{
    if (IsPointValid(p, layer) == false) return { -1, -1 };

    int xgrid = (p.x - GetLeft()) / 2;
    int ygrid = (2 * (p.y - GetTop())) / 2;
    int x = (ygrid + xgrid - GetHeight() + 1) / 2;
    int y = (ygrid - xgrid + GetHeight() - 1) / 2;

    return IsoPoint2D{ x, y };
}

IsoPoint2D Tilemap3D::PixelToIsometric(const PixelPoint2D& /*pix*/, Layer /*layer*/) const
{
    // TODO
    return IsoPoint2D{ -1, -1 };
}

PixelPoint2D Tilemap3D::IsoToPixel(const IsoPoint2D& iso, Layer layer, bool offset) const
{
    if (IsIsoPointValid(iso) == false) return { -1, -1 };
    int layer_offset = (layer == Layer::BG) ? 2 : 0;
    int left_offset = offset ? GetLeft() : 0;
    int top_offset = offset ? GetTop() : 0;
    return {
        ((iso.x - iso.y + (GetHeight() - 1)) * 2 + left_offset + layer_offset) * tile_width,
        (iso.x + iso.y + top_offset) * tile_height
    };
}

PixelPoint2D Tilemap3D::ToPixel(const Point2D& iso, Layer /*layer*/) const
{
    if (IsIsoPointValid(iso) == false) return { -1, -1 };
    return PixelPoint2D{ -1, -1 };
}

PixelPoint2D Tilemap3D::EntityPositionToPixel(uint16_t x, uint16_t y, uint16_t z) const
{
    const int SCALE_FACTOR = 0x100;
    const int LEFT = GetLeft() * SCALE_FACTOR;
    const int TOP = GetTop() * SCALE_FACTOR;
    const int HEIGHT = GetHeight() * SCALE_FACTOR;
    int xx = x - LEFT;
    int yy = y - TOP;
    int ix = (xx - yy + (HEIGHT - SCALE_FACTOR)) * 2 + LEFT;
    int iy = (xx + yy - z * 2) + TOP;
    return Point2D{ ix * tile_width / SCALE_FACTOR, iy * tile_height / SCALE_FACTOR };
}

PixelPoint2D Tilemap3D::Iso3DToPixel(const Point3D& iso) const
{
    //if (IsHMPointValid({ iso.x, iso.y }) == false) return { -1, -1 };
    int xx = iso.x - GetLeft();
    int yy = iso.y - GetTop();
    int ix = (xx - yy + (GetHeight() - 1)) * 2 + GetLeft();
    int iy = (xx + yy - iso.z * 2) + GetTop();
    return Point2D{ ix * tile_width, iy * tile_height };
}

Point3D Tilemap3D::PixelToIso3D(const PixelPoint2D& p) const
{
    if (IsPixelPointValid(p) == false) return { -1, -1, -1 };
    return Point3D{-1, -1, -1};
}

PixelPoint2D Tilemap3D::HMPointToPixel(const HMPoint2D& p) const
{
    return Iso3DToPixel({p.x + 12, p.y + 12, 0});
}

HMPoint2D Tilemap3D::PixelToHMPoint(const PixelPoint2D& p) const
{
    return {
        p.y / (2 * tile_height) + p.x / (4 * tile_width) - (GetLeft() + 2 * (GetTop() + GetHeight() - 1)) / 4 - 12,
        p.y / (2 * tile_height) - p.x / (4 * tile_width) + (GetLeft() - 2 * (GetTop() - GetHeight() + 1)) / 4 - 12
    };
}

bool Tilemap3D::IsBlockValid(uint16_t block) const
{
    return (block < GetSize());
}

bool Tilemap3D::IsBlockValid(const IsoPoint2D& iso) const
{
    return IsIsoPointValid(iso);
}

BlockLoc Tilemap3D::GetBlock(uint16_t block, Layer layer) const
{
    BlockLoc ret{0xFFFF,{-1,-1}};
    if (IsBlockValid(block))
    {
        ret.position.x = block % GetWidth();
        ret.position.y = block / GetWidth();
        if (layer == Layer::FG)
        {
            ret.value = foreground[block];
        }
        else
        {
            ret.value = background[block];
        }
    }
    return ret;
}

uint16_t Tilemap3D::GetBlock(const IsoPoint2D& iso, Layer layer) const
{
    uint16_t block = static_cast<uint16_t>(iso.x + iso.y * GetWidth());
    if (IsBlockValid(block))
    {
        if (layer == Layer::FG)
        {
            return foreground[block];
        }
        else
        {
            return background[block];
        }
    }
    return 0xFFFF;
}

bool Tilemap3D::SetBlock(uint16_t block_value, uint16_t block_index, Layer layer)
{
    if (IsBlockValid(block_index))
    {
        if (layer == Layer::FG)
        {
            foreground[block_index] = block_value & 0x3FF;
        }
        else
        {
            background[block_index] = block_value & 0x3FF;
        }
        return true;
    }
    return false;
}

bool Tilemap3D::SetBlock(const BlockLoc& loc, Layer layer)
{
    if (IsBlockValid(loc.position))
    {
        int block = loc.position.x + loc.position.y * GetWidth();
        if (layer == Layer::FG)
        {
            foreground[block] = loc.value & 0x3FF;
        }
        else
        {
            background[block] = loc.value & 0x3FF;
        }
        return true;
    }
    return false;
}

uint8_t Tilemap3D::GetHeight(const HMPoint2D& p) const
{
    if (IsHMPointValid(p) == false) return 0xFF;
    return (heightmap[p.x + p.y * hmwidth] & 0x0F00) >> 8;
}

bool Tilemap3D::SetHeight(const HMPoint2D& p, uint8_t pheight)
{
    if (IsHMPointValid(p) && pheight < 0x10)
    {
        int cell = p.x + p.y * hmwidth;
        heightmap[cell] &= 0xF0FF;
        heightmap[cell] |= (pheight & 0x0F) << 8;
        return true;
    }
    return false;
}

uint8_t Tilemap3D::GetCellProps(const HMPoint2D& p) const
{
    if (IsHMPointValid(p) == false) return 0xFF;
    return (heightmap[p.x + p.y * hmwidth] & 0xF000) >> 12;
}

bool Tilemap3D::SetCellProps(const HMPoint2D& p, uint8_t props)
{
    if (IsHMPointValid(p) && props < 0x10)
    {
        int cell = p.x + p.y * hmwidth;
        heightmap[cell] &= 0x0FFF;
        heightmap[cell] |= (props & 0x0F) << 12;
        return true;
    }
    return false;
}

uint8_t Tilemap3D::GetCellType(const HMPoint2D& p) const
{
    if (IsHMPointValid(p) == false) return 0xFF;
    return heightmap[p.x + p.y * hmwidth] & 0x00FF;
}

bool Tilemap3D::SetCellType(const HMPoint2D& p, uint8_t type)
{
    if (IsHMPointValid(p) == true)
    {
        int cell = p.x + p.y * hmwidth;
        heightmap[cell] &= 0xFF00;
        heightmap[cell] |= type;
        return true;
    }
    return false;
}

uint16_t Tilemap3D::GetHeightmapCell(const HMPoint2D& iso) const
{
    if (IsHMPointValid(iso) == true)
    {
        int cell = iso.x + iso.y * hmwidth;
        return heightmap[cell];
    }
    return 0xFFFF;
}

bool Tilemap3D::SetHeightmapCell(const HMPoint2D& iso, uint16_t value)
{
    if (IsHMPointValid(iso) == true)
    {
        int cell = iso.x + iso.y * hmwidth;
        heightmap[cell] = value;
        return true;
    }
    return false;
}
