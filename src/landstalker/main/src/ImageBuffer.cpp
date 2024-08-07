#include <landstalker/main/include/ImageBuffer.h>

#include <cassert>
#include <png.h>
#include <numeric>
#include <emmintrin.h>
#include <landstalker/misc/include/Utils.h>

#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
// disable setjmp / longjmp warning for libpng
#pragma warning( disable : 4611 )
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif
#endif

ImageBuffer::ImageBuffer()
    : m_width(0), m_height(0)
{}

ImageBuffer::ImageBuffer(std::size_t width, std::size_t height)
	: m_width(width), m_height(height)
{
    m_pixels.assign(width * height, 0);
    m_priority.assign(width * height, 0);
}

void ImageBuffer::Clear()
{
    Clear(0);
}

void ImageBuffer::Clear(uint8_t colour)
{
    std::fill(m_pixels.begin(), m_pixels.end(), colour);
    std::fill(m_priority.begin(), m_priority.end(), colour);
}

void ImageBuffer::Resize(std::size_t width, std::size_t height)
{
    m_width = width;
    m_height = height;
    m_pixels.clear();
    m_pixels.assign(m_width * m_height, 0);
    m_priority.assign(width * height, 0);
}

void ImageBuffer::PutPixel(std::size_t x, std::size_t y, uint8_t colour)
{
    m_pixels[x + m_width * y] = colour;
}

void ImageBuffer::InsertTile(int x, int y, uint8_t palette_index, const Tile& tile, const Tileset& tileset, bool use_alpha)
{
	int max_x = x + 7;
	int max_y = y + 7;
    std::vector<uint8_t> cmap = tileset.GetColourIndicies();
    if (cmap.empty())
    {
        cmap = tileset.GetDefaultColourIndicies();
    }
    if ((max_x >= static_cast<int>(m_width)) || (max_y >= static_cast<int>(m_height)) || (x < 0 ) || (y < 0))
    {
        std::ostringstream ss;
        ss << "Attempt to draw tile in out-of-range position " << x << ", " << y 
           << " : The image buffer is only " << m_width << " x " << m_height << " pixels." << std::endl;
        Debug(ss.str());
    }
    else
    {
	    auto tile_bits = tileset.GetTile(tile);
	    const uint8_t pal_bits = palette_index << 4;
        std::size_t begin_offset = y * m_width + x;
        auto row_it = m_pixels.begin() + begin_offset;
        auto dest_it = row_it;
        auto pri_row_it = m_priority.begin() + begin_offset;
        auto pri_dest_it = row_it;
        uint8_t priority = tile.Attributes().getAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
        for (std::size_t i = 0; i < tile_bits.size(); ++i)
        {
            if (i % tileset.GetTileWidth() == 0)
            {
                dest_it = row_it + m_width * (i / tileset.GetTileWidth());
                pri_dest_it = pri_row_it + m_width * (i / tileset.GetTileWidth());
				y++;
				x -= tileset.GetTileWidth();
            }
            if (!use_alpha || (cmap[tile_bits[i]] != 0))
            {
                *dest_it = cmap[tile_bits[i]] | pal_bits;
                *pri_dest_it = priority;
            }
            dest_it++;
            pri_dest_it++;
        }
    }
}

void ImageBuffer::ClearTile(int x, int y, const Tileset& ts)
{

    int max_x = x + 7;
    int max_y = y + 7;
    if ((max_x >= static_cast<int>(m_width)) || (max_y >= static_cast<int>(m_height)) || (x < 0) || (y < 0))
    {
        std::ostringstream ss;
        ss << "Attempt to clear tile in out-of-range position " << x << ", " << y
            << " : The image buffer is only " << m_width << " x " << m_height << " pixels." << std::endl;
        Debug(ss.str());
    }
    else
    {
        std::size_t begin_offset = y * m_width + x;
        auto row_it = m_pixels.begin() + begin_offset;
        auto dest_it = row_it;
        auto pri_row_it = m_priority.begin() + begin_offset;
        auto pri_dest_it = row_it;
        for (std::size_t i = 0; i < ts.GetTileWidth() * ts.GetTileHeight(); ++i)
        {
            if (i % ts.GetTileWidth() == 0)
            {
                dest_it = row_it + m_width * (i / ts.GetTileWidth());
                pri_dest_it = pri_row_it + m_width * (i / ts.GetTileWidth());
                y++;
                x -= ts.GetTileWidth();
            }
            *dest_it++ = 0;
            *pri_dest_it++ = false;
        }
    }
}

void ImageBuffer::ClearBlock(int x, int y, const Blockset&, const Tileset& ts)
{
    if ((y + 7) * m_width + x + 7 < m_pixels.size())
    {
        ClearTile(x, y, ts);
        ClearTile(x + 8, y, ts);
        ClearTile(x, y + 8, ts);
        ClearTile(x + 8, y + 8, ts);
    }
    else
    {
        assert(true);
        Debug("Coordinates out of range");
    }
}

void ImageBuffer::InsertSprite(int x, int y, uint8_t palette_index, const SpriteFrame& frame, bool hflip)
{
    for (std::size_t i = 0; i < frame.GetSubSpriteCount(); ++i)
    {
        const auto& subs = frame.GetSubSprite(i);
        std::size_t index = subs.tile_idx;
        for (std::size_t xi = 0; xi < subs.w; ++xi)
            for (std::size_t yi = 0; yi < subs.h; ++yi)
            {
                int xx, yy;
                if (hflip)
                {
                    xx = -subs.x - xi * 8 + x - 8;
                    yy = subs.y + yi * 8 + y;
                    InsertTile(xx, yy, palette_index, !Tile(index++), *frame.GetTileset());
                }
                else
                {
                    xx = subs.x + xi * 8 + x;
                    yy = subs.y + yi * 8 + y;
                    InsertTile(xx, yy, palette_index, Tile(index++), *frame.GetTileset());
                }
            }
    }
}

void ImageBuffer::InsertMap(int x, int y, uint8_t palette_index, const Tilemap2D& map, const Tileset& tileset)
{
    for (std::size_t yy = 0; yy < map.GetHeight(); ++yy)
    {
        for (std::size_t xx = 0; xx < map.GetWidth(); ++xx)
        {
            const int xpos = x + xx * tileset.GetTileWidth();
            const int ypos = y + yy * tileset.GetTileHeight();
            InsertTile(xpos, ypos, palette_index, map.GetTile(xx, yy), tileset);
        }
    }
}

void ImageBuffer::Insert3DMapLayer(int x, int y, uint8_t palette_index, Tilemap3D::Layer layer, const std::shared_ptr<const Tilemap3D> map,
    const std::shared_ptr<const Tileset> tileset, const std::shared_ptr<const std::vector<MapBlock>> blockset, bool offset,
    std::optional<std::vector<TileSwap>> swaps, std::optional<std::vector<Door>> doors)
{
    Point2D tilepos = {0, 0};
    Tilemap3D disp_map = *map;
    if (swaps)
    {
        for (const auto& swap : *swaps)
        {
            swap.DrawSwap(disp_map, layer);
        }
    }
    if (doors)
    {
        for (const auto& door : *doors)
        {
            door.DrawDoor(disp_map, layer);
        }
    }
    for (int yy = 0; yy < disp_map.GetHeight(); ++yy)
        for (int xx = 0; xx < disp_map.GetWidth(); ++xx)
        {
            tilepos = { xx + x, yy + y };
            auto tile = disp_map.GetBlock(tilepos, layer);
            auto loc(disp_map.IsoToPixel(tilepos, layer, offset));
            if (tile >= blockset->size())
            {
                std::ostringstream ss;
                ss << "Attempt to index out of range block " << std::hex << tile << " - maximum is " << (blockset->size() - 1);
                Debug(ss.str());
                tile = 0;
            }
            InsertBlock(loc.x, loc.y, palette_index, blockset->at(tile), *tileset);
            tilepos.x++;
            if (tilepos.x == static_cast<int>(GetWidth()))
            {
                tilepos.x = 0;
                tilepos.y++;
            }
        }
}

bool ImageBuffer::WritePNG(const std::string& filename, const std::vector<std::shared_ptr<Palette>>& palettes, bool use_alpha)
{
    volatile bool retval = false;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if (setjmp(png_jmpbuf(png))) throw std::runtime_error("Unable to set libpng longjmp");

    png_set_IHDR(
        png,
        info,
        m_width, m_height,
        8,
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );

    png_color png_palette[256];
    memset(png_palette, 0, sizeof(png_palette));
    png_byte png_alpha[256];
    memset(png_alpha, 0, sizeof(png_alpha));

    std::size_t entry = 0;
    for (const auto& pal : palettes)
    {
        for (std::size_t i = 0; i < 16; ++i)
        {
            png_palette[entry + i].red = pal->getR(i);
            png_palette[entry + i].green = pal->getG(i);
            png_palette[entry + i].blue = pal->getB(i);
            png_alpha[entry + i] = pal->getA(i);
        }
        entry += 16;
    }
    png_set_PLTE(png, info, png_palette, 256);
    if (use_alpha)
    {
        png_set_tRNS(png, info, png_alpha, 256, NULL);
    };

    FILE* fp = fopen(filename.c_str(), "wb");

    if (fp != NULL)
    {
        png_init_io(png, fp);
        png_write_info(png, info);

        const uint8_t* row = m_pixels.data();
        for (std::size_t y = 0; y < m_height; ++y)
        {
            png_write_row(png, row);
            row += m_width;
        }

        png_write_end(png, info);
        fclose(fp);
        retval = true;
    }
    else
    {
        Debug("Unable to open PNG!");
    }

    png_destroy_write_struct(&png, &info);

    return retval;
}

void ImageBuffer::InsertBlock(std::size_t x, std::size_t y, uint8_t palette_index, const MapBlock& block, const Tileset& tileset)
{
    if ((y + 7) * m_width + x + 7 < m_pixels.size())
    {
        InsertTile(x, y, palette_index, block.GetTile(0), tileset);
        InsertTile(x + 8, y, palette_index, block.GetTile(1), tileset);
        InsertTile(x, y + 8, palette_index, block.GetTile(2), tileset);
        InsertTile(x + 8, y + 8, palette_index, block.GetTile(3), tileset);
    }
    else
    {
        assert(true);
        Debug("Coordinates out of range");
    }
}

const std::vector<uint8_t>& ImageBuffer::GetRGB(const std::vector<std::shared_ptr<Palette>>& pals) const
{
    m_rgb.resize(m_width * m_height * 3);
    auto it = m_rgb.begin();
    std::unique_ptr<uint8_t[]> pal_lookup(new uint8_t[pals.size() * 16 * 4]);
    int idx = 0;
    for (const auto& p : pals)
    {
        for (int i = 0; i < 16; ++i)
        {
            pal_lookup[idx++] = p->getR(i);
            pal_lookup[idx++] = p->getG(i);
            pal_lookup[idx++] = p->getB(i);
            pal_lookup[idx++] = 0;
        }
    }
    for (const auto& pixel : m_pixels)
    {
        *it++ = pal_lookup[(pixel << 2)];
        *it++ = pal_lookup[(pixel << 2) + 1];
        *it++ = pal_lookup[(pixel << 2) + 2];
    }
    return m_rgb;
}

const std::vector<uint8_t>& ImageBuffer::GetAlpha(const std::vector<std::shared_ptr<Palette>>& pals, uint8_t low_pri_max_opacity, uint8_t high_pri_max_opacity) const
{
    m_alpha.resize(m_width * m_height);
    auto pri = m_priority.cbegin();
    auto it = m_alpha.begin();
    std::unique_ptr<uint8_t[]> pal_lookup(new uint8_t[pals.size() * 16]);
    int pi = 0;
    for (const auto& p : pals)
    {
        for (int i = 0; i < 16; ++i)
        {
            pal_lookup[pi++] = p->getA(i);
        }
    }
    for (const auto& pixel : m_pixels)
    {
        uint8_t alpha = pal_lookup[pixel];
        uint8_t max_opacity = *pri++ ? high_pri_max_opacity : low_pri_max_opacity;
        *it++ = std::min<uint8_t>(max_opacity, alpha);
    }
    return m_alpha;
}

std::shared_ptr<wxBitmap> ImageBuffer::MakeBitmap(const std::vector<std::shared_ptr<Palette>>& pals, bool use_alpha, uint8_t low_pri_max_opacity, uint8_t high_pri_max_opacity) const
{
    GetRGB(pals);
    wxImage* img = new wxImage(m_width, m_height, m_rgb.data(), true);
    if (use_alpha)
    {
        GetAlpha(pals, low_pri_max_opacity, high_pri_max_opacity);
        img->SetAlpha(m_alpha.data(), true);
    }
    auto ret = std::make_shared<wxBitmap>(*img);
    return ret;
}

wxImage ImageBuffer::MakeImage(const std::vector<std::shared_ptr<Palette>>& pals, bool use_alpha, uint8_t low_pri_max_opacity, uint8_t high_pri_max_opacity) const
{
    GetRGB(pals);
    wxImage img(m_width, m_height, m_rgb.data(), true);
    if (use_alpha)
    {
        GetAlpha(pals, low_pri_max_opacity, high_pri_max_opacity);
        img.SetAlpha(m_alpha.data(), true);
    }
    return img;
}

std::size_t ImageBuffer::GetHeight() const
{
	return m_height;
}

std::size_t ImageBuffer::GetWidth() const
{
	return m_width;
}
