#include <landstalker/tileset/include/Tileset.h>
#include <algorithm>
#include <sstream>
#include <numeric>
#include <iterator>
#include <landstalker/misc/include/Utils.h>
#include <landstalker/misc/include/LZ77.h>
#include <landstalker/misc/include/Literals.h>

static const std::size_t MAXIMUM_CAPACITY = 0x400;

template<class T>
void HFlip(std::vector<T>& elems, int width)
{
    int height = elems.size() / width;
    for (int i = 0; i < height; ++i)
    {
        auto source_it = elems.begin() + width * i;
        std::reverse(source_it, source_it + width);
    }
}

template<class T>
void VFlip(std::vector<T>& elems, int width)
{
    int height = elems.size() / width;
    for (int i = 0; i < height / 2; ++i)
    {
        auto source_it = elems.begin() + width * i;
        auto dest_it = elems.end() - width * (i + 1);
        std::swap_ranges(source_it, source_it + width, dest_it);
    }
}

struct BlockDimensions
{
    int width;
    int height;
    int Area() const { return width * height; }
};

const std::unordered_map<Tileset::BlockType, BlockDimensions> BLOCK_DIMENSIONS =
{
    {Tileset::BlockType::NORMAL,   {1, 1}},
    {Tileset::BlockType::BLOCK1X2, {1, 2}},
    {Tileset::BlockType::BLOCK2X1, {2, 1}},
    {Tileset::BlockType::BLOCK2X2, {2, 2}},
    {Tileset::BlockType::BLOCK3X3, {3, 3}},
    {Tileset::BlockType::BLOCK4X4, {4, 4}},
    {Tileset::BlockType::BLOCK4X6, {4, 6}}
};

Tileset::Tileset(std::size_t width, std::size_t height, uint8_t bit_depth, Tileset::BlockType blocktype)
    : m_width(width * BLOCK_DIMENSIONS.at(blocktype).width),
      m_height(height * BLOCK_DIMENSIONS.at(blocktype).height),
      m_bit_depth(bit_depth),
      m_tilewidth(width),
      m_tileheight(height),
	  m_compressed(false),
      m_blocktype(blocktype)
{
}

Tileset::Tileset(const std::string& filename, bool compressed, std::size_t width, std::size_t height, uint8_t bit_depth, Tileset::BlockType blocktype)
{
    Open(filename, compressed, width, height, bit_depth, blocktype);
}

Tileset::Tileset(const std::vector<uint8_t>& src, bool compressed, std::size_t width, std::size_t height, uint8_t bit_depth, Tileset::BlockType blocktype)
    : m_width(width * BLOCK_DIMENSIONS.at(blocktype).width),
      m_height(height * BLOCK_DIMENSIONS.at(blocktype).height),
      m_bit_depth(bit_depth),
      m_tilewidth(width),
      m_tileheight(height),
      m_compressed(compressed),
      m_blocktype(blocktype)
{
    SetBits(src, compressed);
}

Tileset::~Tileset()
{
}

bool Tileset::operator==(const Tileset& rhs) const
{
    return ((this->m_bit_depth == rhs.m_bit_depth) &&
        (this->m_tileheight == rhs.m_tileheight) &&
        (this->m_tilewidth == rhs.m_tilewidth) &&
        (this->m_tiles == rhs.m_tiles));
}

bool Tileset::operator!=(const Tileset& rhs) const
{
    return !(*this == rhs);
}

uint32_t Tileset::SetBits(const std::vector<uint8_t>& src, bool compressed)
{
    const std::size_t tile_size_bytes = m_width * m_height * m_bit_depth / 8;
    const std::vector<uint8_t>* input = &src;
    m_compressed = compressed;
    uint32_t ret = src.size();

	std::vector<uint8_t> buffer;
    if (compressed == true)
    {
		buffer.resize(65536);
        std::size_t elen, dlen;
        dlen = LZ77::Decode(src.data(), src.size(), buffer.data(), elen);
        buffer.resize(dlen);
        input = &buffer;
        ret = elen;
    }
    const std::size_t num_tiles = (input->size() + (tile_size_bytes - 1)) / tile_size_bytes;

    m_tiles.clear();
    m_tiles.assign(num_tiles, std::vector<uint8_t>(m_width * m_height));
    std::size_t tc = 0;
    for (auto& t : m_tiles)
    {
        for (std::size_t i = 0; i < tile_size_bytes; ++i)
        {
            uint8_t byte = 0;
            if (i + tc < input->size())
            {
                byte = input->at(i + tc);
            }
            uint8_t shift = static_cast<uint8_t>(8 - m_bit_depth);
            uint8_t mask = static_cast<uint8_t>(0xFF << shift);
            const std::size_t pixels_per_byte = 8 / m_bit_depth;
            for (std::size_t p = 0; p < pixels_per_byte; ++p)
            {
                t[i * pixels_per_byte + p] = (byte & mask) >> shift;
                mask >>= static_cast<uint8_t>(m_bit_depth);
                shift -= static_cast<uint8_t>(m_bit_depth);
            }
        }
        tc += tile_size_bytes;
    }
    TransposeBlock();
    return ret;
}

void Tileset::SetParams(std::size_t width, std::size_t height, uint8_t bit_depth, Tileset::BlockType blocktype)
{
    Clear();
    m_blocktype = blocktype;
    m_width = width * BLOCK_DIMENSIONS.at(blocktype).width;
    m_height = height * BLOCK_DIMENSIONS.at(blocktype).height;
    m_tilewidth = width;
    m_tileheight = height;
    m_bit_depth = bit_depth;
    if (m_colour_indicies.size() < static_cast<std::size_t>(1 << bit_depth))
    {
        m_colour_indicies.clear();
    }
}

bool Tileset::Open(const std::string& filename, bool compressed, std::size_t width, std::size_t height, uint8_t bit_depth, Tileset::BlockType blocktype)
{
    bool retval = false;
    std::streampos filesize;
    std::ifstream ifs(filename, std::ios::binary);

    ifs.unsetf(std::ios::skipws);

    ifs.seekg(0, std::ios::end);
    filesize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes;
    bytes.reserve(static_cast<std::size_t>(filesize));
    bytes.insert(bytes.begin(),
        std::istream_iterator<uint8_t>(ifs),
        std::istream_iterator<uint8_t>());

    if (bytes.size() > 0)
    {
        Clear();
        SetParams(width, height, bit_depth, blocktype);
        SetBits(bytes, compressed);
        retval = true;
    }
    return retval;
}

std::vector<uint8_t> Tileset::GetBits(bool compressed)
{
    std::vector<uint8_t> bits;
    std::vector<uint8_t> buffer;
    std::vector<std::vector<uint8_t>> tilebuf;
    std::vector<uint8_t>* retval = &bits;
    const std::vector<std::vector<uint8_t>>& tiles = (m_blocktype != BlockType::NORMAL) ? tilebuf : m_tiles;

    bits.reserve(m_width * m_height * m_tiles.size() * m_bit_depth / 8);
    if (m_blocktype != BlockType::NORMAL)
    {
        // Convert back into tiles and reverse transpose
        for (const auto& t : m_tiles)
        {
            for (int x = 0; x < BLOCK_DIMENSIONS.at(m_blocktype).width; ++x)
            {
                for (int y = 0; y < BLOCK_DIMENSIONS.at(m_blocktype).height; ++y)
                {
                    tilebuf.insert(tilebuf.end(), std::vector<uint8_t>());
                    for (std::size_t i = 0; i < m_tileheight; ++i)
                    {
                        auto line = t.begin() + i * m_width + x * m_tilewidth + y * m_width * m_tileheight;
                        tilebuf.back().insert(tilebuf.back().end(), line, line + m_tilewidth);
                    }
                }
            }
            if (m_blocktype == BlockType::BLOCK4X6)
            {
                // Additional processing for block4x6 special case 
                // Fix tile ordering
                // Block would be transposed as follows:
                // 00  06  12  18       00  04  08  12
                // 01  07  13  19       01  05  09  13
                // 02  08  14  20  ==>  02  06  10  14
                // 03  09  15  21       03  07  11  15
                // 04  10  16  22       16  18  20  22
                // 05  11  17  23       17  19  21  23
                auto block_start = tilebuf.end() - BLOCK_DIMENSIONS.at(m_blocktype).Area();
                auto block_end = tilebuf.end();
                std::vector<std::vector<uint8_t>> blockbuf(block_start, block_end);
                std::array<uint8_t, 24> UntransposeLUT{ 0,1,2,3,6,7,8,9,12,13,14,15,18,19,20,21,4,5,10,11,16,17,22,23 };
                for (std::size_t i = 0; i < blockbuf.size(); ++i)
                {
                    block_start[i] = blockbuf[UntransposeLUT[i]];
                }
            }
        }
    }
    for (const auto& t : tiles)
    {
        // Convert to n-bitdepth
        uint8_t byte = 0;
        uint8_t bit = 0;
        for (const auto& p : t)
        {
            byte <<= m_bit_depth;
            byte |= p & (0xFF >> (8 - m_bit_depth));
            bit += static_cast<uint8_t>(m_bit_depth);
            if (bit == 8)
            {
                bit = 0;
                bits.push_back(byte);
            }
        }
    }
    if (compressed)
    {
        buffer.resize(65536);
        auto csize = LZ77::Encode(bits.data(), bits.size(), buffer.data());
        buffer.resize(csize);
        retval = &buffer;
    }
    return *retval;
}

bool Tileset::Save(const std::string& filename, bool compressed)
{
    bool retval = false;
    auto bits = GetBits(compressed);
    std::ofstream ofs(filename, std::ios::out | std::ios::binary);
    if (ofs)
    {
        ofs.write(reinterpret_cast<const char*>(bits.data()), bits.size());
        ofs.close();
        retval = true;
    }
    return retval;
}

void Tileset::Clear()
{
    m_tiles.clear();
}

void Tileset::Reset(int size)
{
    if (size != -1)
    {
        m_tiles.resize(size);
    }
    for (auto& elem : m_tiles)
    {
        std::fill(elem.begin(), elem.end(), 0_u8);
    }
}

void Tileset::Resize(int size)
{
    m_tiles.resize(size);
}

std::vector<uint8_t> Tileset::GetTileRGB(const Tile& tile, const Palette& palette) const
{
    auto t = GetTile(tile);
    std::vector<uint8_t> ret;
    ret.reserve(m_width * m_height * 3);
    for (auto p : t)
    {
        if (m_colour_indicies.empty())
        {
            ret.push_back(palette.getR(p));
            ret.push_back(palette.getG(p));
            ret.push_back(palette.getB(p));
        }
        else
        {
            ret.push_back(palette.getR(m_colour_indicies[p]));
            ret.push_back(palette.getG(m_colour_indicies[p]));
            ret.push_back(palette.getB(m_colour_indicies[p]));
        }
    }

    return ret;
}

std::vector<uint8_t> Tileset::GetTileA(const Tile& tile, const Palette& palette) const
{
    auto t = GetTile(tile);
    std::vector<uint8_t> ret;
    ret.reserve(m_width * m_height);
    for (auto p : t)
    {
        if (m_colour_indicies.empty())
        {
            ret.push_back(palette.getA(p));
        }
        else
        {
            ret.push_back(palette.getA(m_colour_indicies[p]));
        }
    }
    return ret;
}

std::vector<uint32_t> Tileset::GetTileRGBA(const Tile& tile, const Palette& palette) const
{
    auto t = GetTile(tile);
    std::vector<uint32_t> ret;
    ret.reserve(m_width * m_height);
    for (auto p : t)
    {
        if (m_colour_indicies.empty())
        {
            ret.push_back(palette.getRGBA(p));
        }
        else
        {
            ret.push_back(palette.getRGBA(m_colour_indicies[p]));
        }
    }

    return ret;
}

std::vector<uint32_t> Tileset::GetTileBGRA(const Tile& tile, const Palette& palette) const
{
    auto t = GetTile(tile);
    std::vector<uint32_t> ret;
    ret.reserve(m_width * m_height);
    for (auto p : t)
    {
        if (m_colour_indicies.empty())
        {
            ret.push_back(palette.getBGRA(p));
        }
        else
        {
            ret.push_back(palette.getBGRA(m_colour_indicies[p]));
        }
    }

    return ret;
}

void Tileset::SetColourIndicies(const std::vector<uint8_t>& colour_indicies)
{
	if (colour_indicies.size() == 0)
	{
		m_colour_indicies.clear();
	}
    else if (colour_indicies.size() >= static_cast<std::size_t>(1 << m_bit_depth))
    {
        bool ok = true;
        for (auto c : colour_indicies)
        {
            if (c >= 16)
            {
                ok = false;
                break;
            }
        }
        if(ok) m_colour_indicies = colour_indicies;
    }
}

std::vector<uint8_t> Tileset::GetColourIndicies() const
{
    return m_colour_indicies;
}

std::vector<uint8_t> Tileset::GetDefaultColourIndicies() const
{
    std::vector<uint8_t> ret(1 << m_bit_depth);
    std::iota(ret.begin(), ret.end(), 0_u8);
    return ret;
}

std::array<bool, 16> Tileset::GetLockedColours() const
{
    std::array<bool, 16> retval{0};
    retval.fill(true);
    if (m_colour_indicies.empty())
    {
        std::fill(retval.begin(), retval.begin() + (1 << m_bit_depth), false);
    }
    else
    {
        for (int i = 0; i < (1 << m_bit_depth); ++i)
        {
            retval[m_colour_indicies[i]] = false;
        }
    }
    return retval;
}

std::size_t Tileset::GetTileCount() const
{
    return m_tiles.size();
}

std::size_t Tileset::GetTileSizeBytes() const
{
    return m_width * m_height * m_bit_depth / 8;
}

std::size_t Tileset::GetTilesetUncompressedSizeBytes() const
{
	return m_tiles.size() * GetTileSizeBytes();
}

std::size_t Tileset::GetTileWidth() const
{
    return m_width;
}

std::size_t Tileset::GetTileHeight() const
{
    return m_height;
}

std::size_t Tileset::GetTileBitDepth() const
{
    return m_bit_depth;
}

Tileset::BlockType Tileset::GetTileBlockType() const
{
    return m_blocktype;
}

bool Tileset::GetCompressed() const
{
    return m_compressed;
}

void Tileset::DeleteTile(int tile_number)
{
    if ((tile_number >= 0) && (tile_number < static_cast<int>(m_tiles.size())))
    {
        m_tiles.erase(m_tiles.begin() + tile_number);
    }
}

void Tileset::InsertTilesBefore(int tile_number, int count)
{
    if ((tile_number >= 0) && (tile_number <= static_cast<int>(m_tiles.size())) &&
        (tile_number + count <= static_cast<int>(MAXIMUM_CAPACITY)))
    {
        for (int i = 0; i < count; ++i)
        {
            m_tiles.insert(m_tiles.begin() + tile_number, std::vector<uint8_t>(m_width * m_height));
        }
    }
	else
	{
		throw std::out_of_range("bad count");
	}
}

void Tileset::DuplicateTile(const Tile& src, const Tile& dst)
{
    if ((src.GetIndex() < m_tiles.size()) &&
        (dst.GetIndex() < m_tiles.size()) &&
        (src.GetIndex() != dst.GetIndex()))
    {
        m_tiles[src.GetIndex()] = m_tiles[dst.GetIndex()];
    }
}

void Tileset::SwapTile(const Tile& lhs, const Tile& rhs)
{
    if ((lhs.GetIndex() < m_tiles.size()) &&
        (rhs.GetIndex() < m_tiles.size()) &&
        (lhs.GetIndex() != rhs.GetIndex()))
    {
        std::swap(m_tiles[lhs.GetIndex()], m_tiles[rhs.GetIndex()]);
    }
}

void Tileset::SetTile(const Tile& src, const std::vector<uint8_t>& value)
{
    if (src.GetIndex() < m_tiles.size())
    {
        if (value.size() == m_height * m_width)
        {
            bool ok = true;
            for (auto p : value)
            {
                if (p >= (1 << m_bit_depth))
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
            {
                m_tiles[src.GetIndex()] = value;
            }
        }
    }
}

void Tileset::TransposeBlock()
{
    if (m_blocktype == BlockType::NORMAL)
    {
        return;
    }
    const auto& DIMS = BLOCK_DIMENSIONS.at(m_blocktype);
    std::vector<uint8_t>::const_iterator src_it;
    if (m_blocktype != BlockType::BLOCK4X6)
    {
        for (auto& b : m_tiles)
        {
            std::vector<uint8_t> t = b;
            src_it = t.begin();
            for (int x = 0; x < DIMS.width; ++x)
            {
                auto dest_it = b.begin();
                for (std::size_t y = 0; y < m_height; ++y)
                {
                    std::copy(src_it, src_it + m_tilewidth, dest_it + x * m_tilewidth);
                    src_it += m_tilewidth;
                    dest_it += m_width;
                }
            }
        }
    }
    else
    {
        // 4x6 blocks are a little different - they are stored as two separate blocks - e.g.
        //
        //  00  04  08  12
        //  01  05  09  13
        //  02  06  10  14
        //  03  07  11  15
        //  16  18  20  22
        //  17  19  21  23
        //
        //  Here, we store which tiles need to be copied where, and use this lookup table to do
        //  the transform.
        const std::array<int, 24> transpose4x6  { 0,4,8,12,1,5,9,13,2,6,10,14,3,7,11,15,16,18,20,22,17,19,21,23};

        for (auto& b : m_tiles)
        {
            std::vector<std::vector<uint8_t>> tilebuf;
            src_it = b.begin();
            for (int i = 0; i < DIMS.Area(); ++i)
            {
                tilebuf.push_back(std::vector<uint8_t>(src_it, src_it + m_tileheight * m_tilewidth));
                src_it += m_tileheight * m_tilewidth;
            }
            for (std::size_t i = 0; i < tilebuf.size(); ++i)
            {
                src_it = tilebuf[transpose4x6[i]].begin();
                auto dest_it = b.begin() + (i % DIMS.width) * m_tilewidth + (i / DIMS.width) * m_width * m_tileheight;
                for (std::size_t y = 0; y < m_tileheight; ++y)
                {
                    std::copy(src_it, src_it + m_tilewidth, dest_it + y * m_width);
                    src_it += m_tilewidth;
                }
            }
        }
    }
}

void Tileset::UntransposeBlock(std::vector<uint8_t>& /*bits*/)
{
    // const std::array<int, 24> untranspose4x6{ 0,4,8,12,1,5,9,13,2,6,10,14,3,7,11,15,16,20,17,21,18,22,19,23};
}

std::vector<uint8_t> Tileset::GetTile(const Tile& tile) const
{
    std::size_t idx = tile.GetIndex();
    if (idx >= m_tiles.size())
    {
        std::ostringstream ss;
        ss << "Attempt to obtain out-of-range tile " << idx;
        Debug(ss.str());
        idx = 0;
    }
    std::vector<uint8_t> ret(m_tiles[idx]);
    if (tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_VFLIP))
    {
        VFlip(ret, m_width);
    }
    if (tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_HFLIP))
    {
        HFlip(ret, m_width);
    }
    return ret;
}

std::vector<uint8_t>& Tileset::GetTilePixels(int tile_index)
{
    if ((tile_index < 0) || (tile_index >= static_cast<int>(m_tiles.size())))
    {
        std::ostringstream ss;
        ss << "Attempt to obtain out-of-range tile " << tile_index;
        Debug(ss.str());
        throw(std::runtime_error(ss.str()));
    }
    return m_tiles[tile_index];
}
