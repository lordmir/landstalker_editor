#include "SpriteFrame.h"
#include <vector>
#include <iterator>
#include "Rom.h"
#include "LZ77.h"
#include "Utils.h"

SpriteFrame::SpriteFrame(const std::vector<uint8_t>& src)
	: m_compressed(false),
	  m_sprite_gfx()
{
	SetBits(src);
}

SpriteFrame::SpriteFrame(const std::string& filename)
	: m_compressed(false),
	  m_sprite_gfx()
{
	Open(filename);
}

SpriteFrame::SpriteFrame()
	: m_compressed(false)
{
}

bool SpriteFrame::operator==(const SpriteFrame& rhs) const
{
	return ((this->m_subsprites == rhs.m_subsprites) &&
		    (this->m_sprite_gfx == rhs.m_sprite_gfx) &&
		    (this->m_compressed == rhs.m_compressed));
}

bool SpriteFrame::operator!=(const SpriteFrame& rhs) const
{
	return !(*this == rhs);
}

bool SpriteFrame::Open(const std::string& filename)
{
	bool retval = false;
	std::streampos filesize;
	std::ifstream ifs(filename, std::ios::binary);

	ifs.unsetf(std::ios::skipws);

	ifs.seekg(0, std::ios::end);
	filesize = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<uint8_t> bytes;
	bytes.reserve(filesize);
	bytes.insert(bytes.begin(),
		std::istream_iterator<uint8_t>(ifs),
		std::istream_iterator<uint8_t>());

	if (bytes.size() > 0)
	{
		Clear();
		SetBits(bytes);
		retval = true;
	}
	return retval;
}

std::vector<uint8_t> SpriteFrame::GetBits()
{
	std::vector<uint8_t> bits;
	std::size_t expected_tiles = 0;
	std::size_t actual_tiles = m_sprite_gfx.GetTileCount();

	if (m_subsprites.size() > 0)
	{
		for (const auto& s : m_subsprites)
		{
			uint8_t b1, b2;
			b1 = (((s.y < 0) ? (s.y + 0x100) : s.y) >> 1) & 0x7C;
			b1 |= (s.w - 1) & 0x03;
			b1 = (((s.x < 0) ? (s.x + 0x100) : s.x) >> 1) & 0x7C;
			b1 |= (s.h - 1) & 0x03;

			bits.push_back(b1);
			bits.push_back(b2);
			expected_tiles += s.h * s.w;
		}
		bits.back() |= 0x80;
	}

	auto last_cmd = bits.end();
	if (m_compressed) // compression
	{
		uint16_t byte_count = std::min(actual_tiles, expected_tiles) * 64;
		bits.push_back(0x20 | byte_count >> 8);
		bits.push_back(byte_count & 0xFF);
		auto tiles = m_sprite_gfx.GetBits();
		std::vector<uint8_t> buffer(65536);
		buffer.resize(LZ77::Encode(tiles.data(), byte_count, buffer.data()));
		bits.insert(bits.end(), buffer.begin(), buffer.end());
	}
	else
	{
		uint16_t total_words = std::min(actual_tiles, expected_tiles) * 32;
		uint16_t current_word = 0;
		uint16_t operation_length = 0;
		int blanks = 0;
		auto tiles = m_sprite_gfx.GetBits();
		const auto write_blanks = [&]()
		{
			last_cmd = bits.end();
			bits.push_back(0x80 | (blanks >> 8));
			bits.push_back(blanks & 0xFF);
			current_word += blanks;
			blanks = 0;
		};
		const auto write_data = [&]()
		{
			operation_length -= blanks;
			last_cmd = bits.end();
			bits.push_back(0x10 | (operation_length >> 8));
			bits.push_back(operation_length & 0xFF);
			bits.insert(bits.end(), tiles.begin() + current_word * 2, tiles.begin() + (current_word + operation_length) * 2);
			current_word += operation_length;
			operation_length = blanks;
		};
		while (current_word < total_words)
		{
			uint16_t p = (current_word + operation_length) * 2;
			uint16_t word = (tiles[p] << 8) | tiles[p + 1];
			bool is_last_word = (current_word == total_words - 1);
			operation_length++;
			if (word == 0) // blank
			{
				blanks++;
				// Only compress zero runs if we have at least two.
				if((blanks > 1) && (operation_length > blanks))
				{
					// Encountered a run of at least two blanks.
					// We also have data pending. write out what we have so far.
					write_data();
				}
				if (is_last_word)
				{
					// Final word. Write out what we have so far.
					write_blanks();
				}
			}
			else
			{
				operation_length++;
				if (blanks > 1)
				{
					// Encountered non-blanks. We also have a run of at least two blanks.
					// Write out the number of blanks we have encountered.
					write_blanks();
				}
				if (is_last_word)
				{
					// Final word. Write out what we have so far.
					write_data();
				}
				blanks = 0;
			}
		}
	}
	// Fill in padding if required
	if (actual_tiles < expected_tiles)
	{
		last_cmd = bits.end();
		uint16_t word_count = (expected_tiles - actual_tiles) * 32;
		bits.push_back(0x80 | (word_count >> 8));
		bits.push_back(word_count & 0xFF);
	}

	// Mark final command to stop decompression
	*last_cmd |= 0x40;
	return bits;
}

std::size_t SpriteFrame::SetBits(const std::vector<uint8_t>& src)
{
	auto it = src.begin();
	std::size_t tile_idx = 0;
	do
	{
		int y = ((*it & 0x7C) << 1);
		if (y > 0x80)
		{
			y -= 0x100;
		}
		std::size_t w = (*it & 0x03) + 1;
		int x = ((*++it & 0x7C) << 1);
		if (x > 0x80)
		{
			x -= 0x100;
		}
		std::size_t h = (*it & 0x03) + 1;
		m_subsprites.push_back({ x,y,w,h, tile_idx });
		tile_idx += w * h;
	} while ((*it++ & 0x80) == 0);

	for (const auto subs : m_subsprites)
	{
		std::ostringstream ss;
		ss << "Sprite T:" << subs.tile_idx << " X:" << subs.x << " Y:" << subs.y << " W:" << subs.w << " H:" << subs.h;
		Debug(ss.str().c_str());
	}
	std::ostringstream ss;
	ss << "Total tiles to load: " << tile_idx;
	Debug(ss.str().c_str());
	std::vector<uint8_t> sprite_gfx(tile_idx * 32, 0);
	auto dest_it = sprite_gfx.begin();

	uint16_t command;
	uint8_t ctrl;
	uint16_t count;
	do
	{
		command = (*it << 8) | *std::next(it);
		ctrl = command >> 12;
		count = command & 0xFFF;
		it += 2;

		if ((ctrl & 0x08) > 0)
		{
			std::ostringstream ss;
			ss << "Insert " << count << " zero words." << std::endl;
			Debug(ss.str().c_str());
			dest_it += count * 2;
			m_compressed = true;
		}
		else if ((ctrl & 0x02) > 0)
		{
			std::ostringstream ss;
			std::size_t elen = 0;
			std::size_t dlen = LZ77::Decode(&(*it), src.end() - it, &(*dest_it), elen);
			dest_it += dlen;
			ss << "Copy " << elen << " compressed bytes, " << dlen << " bytes decompressed.";
			Debug(ss.str().c_str());
			it += elen;
			m_compressed = true;
		}
		else
		{
			std::ostringstream ss;
			ss << "Copy " << count << " words directly.";
			Debug(ss.str().c_str());
			std::copy(it, it + count * 2, dest_it);
			dest_it += count * 2;
			it += count * 2;
		}
	} while ((ctrl & 0x04) == 0);

	m_sprite_gfx.SetBits(sprite_gfx);

	Debug("Done!");
	return std::distance(src.begin(), it);
}

bool SpriteFrame::Save(const std::string& filename, bool use_compression)
{
	bool retval = false;
	auto bits = GetBits();
	std::ofstream ofs(filename, std::ios::out | std::ios::binary);
	if (ofs)
	{
		ofs.write(reinterpret_cast<const char*>(bits.data()), bits.size());
		ofs.close();
		retval = true;
	}
	return retval;
}

void SpriteFrame::Clear()
{
	m_subsprites.clear();
	m_sprite_gfx.Clear();
}

std::vector<uint8_t> SpriteFrame::GetTile(const Tile& tile) const
{
	return m_sprite_gfx.GetTile(tile);
}

std::pair<int, int> SpriteFrame::GetTilePosition(const Tile& tile) const
{
	int x = 0; int y = 0;

	auto it = m_subsprites.cbegin();
	for (; it != m_subsprites.cend(); ++it)
	{
		if ((tile.GetIndex() >= it->tile_idx) &&
		    (tile.GetIndex()) < (it->tile_idx + (it->w * it->h)))
		{
			int p = tile.GetIndex() - it->tile_idx;
			x = it->x + 8 * (p / it->h);
			y = it->y + 8 * (p % it->h);
			break;
		}
	}

	return std::pair<int, int>(x, y);
}

std::vector<uint8_t>& SpriteFrame::GetTilePixels(int tile_index)
{
	return m_sprite_gfx.GetTilePixels(tile_index);
}

std::vector<uint8_t> SpriteFrame::GetTileRGB(const Tile& tile, std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const
{
	PaletteO p = GetSpritePalette(low_palette, high_palette);
	return m_sprite_gfx.GetTileRGB(tile, p);
}

std::vector<uint8_t> SpriteFrame::GetTileA(const Tile& tile, std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const
{
	PaletteO p = GetSpritePalette(low_palette, high_palette);
	return m_sprite_gfx.GetTileA(tile, p);
}

std::vector<uint32_t> SpriteFrame::GetTileRGBA(const Tile& tile, std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const
{
	PaletteO p = GetSpritePalette(low_palette, high_palette);
	return m_sprite_gfx.GetTileRGBA(tile, p);
}

const Tileset& SpriteFrame::GetTileset() const
{
	return m_sprite_gfx;
}

Tileset& SpriteFrame::GetTileset()
{
	return m_sprite_gfx;
}

std::size_t SpriteFrame::GetTileCount() const
{
	return m_sprite_gfx.GetTileCount();
}

std::size_t SpriteFrame::GetExpectedTileCount() const
{
	std::size_t c = 0;
	for (const auto& s : m_subsprites)
	{
		c += s.h * s.w;
	}
	return c;
}

std::size_t SpriteFrame::GetSubSpriteCount() const
{
	return m_subsprites.size();
}

std::size_t SpriteFrame::GetTileWidth() const
{
	return m_sprite_gfx.GetTileWidth();
}

std::size_t SpriteFrame::GetTileHeight() const
{
	return m_sprite_gfx.GetTileHeight();
}

std::size_t SpriteFrame::GetTileBitDepth() const
{
	return m_sprite_gfx.GetTileBitDepth();
}

bool SpriteFrame::GetCompressed() const
{
	return m_compressed;
}

void SpriteFrame::SetCompressed(bool compressed)
{
	m_compressed = compressed;
}

SpriteFrame::SubSprite& SpriteFrame::GetSubSprite(std::size_t idx)
{
	return m_subsprites[idx];
}

SpriteFrame::SubSprite SpriteFrame::GetSubSprite(std::size_t idx) const
{
	return m_subsprites[idx];
}

SpriteFrame::SubSprite& SpriteFrame::AddSubSpriteBefore(std::size_t idx)
{
	return *m_subsprites.insert(m_subsprites.begin() + idx, 1, SubSprite());
}

void SpriteFrame::DeleteSubSprite(std::size_t idx)
{
	m_subsprites.erase(m_subsprites.begin() + idx);
}

void SpriteFrame::SwapSubSprite(std::size_t src1, std::size_t src2)
{
	std::swap(m_subsprites[src1], m_subsprites[src2]);	
}

PaletteO SpriteFrame::GetSpritePalette(std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const
{
	PaletteO p;
	if (low_palette)
	{
		p = *low_palette;
	}
	else if (high_palette)
	{
		p = *high_palette;
	}
	else
	{
		p.LoadDebugPal();
	}
	if (low_palette && high_palette)
	{
		p.AddHighPalette(*high_palette);
	}
	return p;
}
