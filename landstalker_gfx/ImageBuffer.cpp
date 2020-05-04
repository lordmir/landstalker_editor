#include "ImageBuffer.h"

ImageBuffer::ImageBuffer(size_t width, size_t height)
	: m_width(width), m_height(height)
{
	m_pixels.assign(width * height, 0);
}

void ImageBuffer::InsertTile(size_t x, size_t y, uint8_t palette_index, const Tile& tile, const Tileset& tileset)
{
	auto tile_bits = tileset.getTile(tile);
	const uint8_t pal_bits = palette_index << 4;
	auto row_it = m_pixels.begin() + y * m_width + x;
	auto dest_it = row_it;
	for (size_t i = 0; i < tile_bits.size(); ++i)
	{
		if (i % 8 == 0)
		{
			dest_it = row_it + m_width * (i/8);
		}
		if (tile_bits[i] != 0)
		{
			*dest_it = tile_bits[i] | pal_bits;
		}
		dest_it++;
	}
}

void ImageBuffer::InsertBlock(size_t x, size_t y, uint8_t palette_index, const BigTile& block, const Tileset& tileset)
{
	InsertTile(x, y, palette_index, block.getTile(0), tileset);
	InsertTile(x + 8, y, palette_index, block.getTile(1), tileset);
	InsertTile(x, y + 8, palette_index, block.getTile(2), tileset);
	InsertTile(x + 8, y + 8, palette_index, block.getTile(3), tileset);
}

std::vector<uint8_t> ImageBuffer::GetRGB(const std::vector<Palette>& pals) const
{
	std::vector<uint8_t> ret;
	ret.reserve(m_width * m_height * 3);
	for (const auto& pixel : m_pixels)
	{
		ret.push_back(pals[pixel >> 4].getR(pixel & 0x0F));
		ret.push_back(pals[pixel >> 4].getG(pixel & 0x0F));
		ret.push_back(pals[pixel >> 4].getB(pixel & 0x0F));
	}
	return ret;
}

std::vector<uint8_t> ImageBuffer::GetAlpha(const std::vector<Palette>& pals) const
{
	std::vector<uint8_t> ret;
	ret.reserve(m_width * m_height);
	for (const auto& pixel : m_pixels)
	{
		ret.push_back(pals[pixel >> 4].getA(pixel & 0x0F));
	}
	return ret;
}

size_t ImageBuffer::GetHeight() const
{
	return m_height;
}

size_t ImageBuffer::GetWidth() const
{
	return m_width;
}