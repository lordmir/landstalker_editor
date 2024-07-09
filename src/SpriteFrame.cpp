#include "SpriteFrame.h"
#include <vector>
#include <iterator>
#include "Rom.h"
#include "LZ77.h"
#include "Utils.h"

SpriteFrame::SpriteFrame(const std::vector<uint8_t>& src)
	: m_compressed(false)
{
	SetBits(src);
}

SpriteFrame::SpriteFrame(const std::string& filename)
	: m_compressed(false)
{
	Open(filename);
}

SpriteFrame::SpriteFrame()
	: m_compressed(false)
{
}

SpriteFrame::SpriteFrame(const SpriteFrame& rhs)
	: m_subsprites(rhs.m_subsprites),
	  m_sprite_gfx(std::make_shared<Tileset>(*rhs.m_sprite_gfx)),
	  m_compressed(rhs.m_compressed)
{
}

SpriteFrame& SpriteFrame::operator=(const SpriteFrame& rhs)
{
	this->m_compressed = rhs.m_compressed;
	this->m_sprite_gfx = std::make_shared<Tileset>(*rhs.m_sprite_gfx);
	this->m_subsprites = rhs.m_subsprites;
	return *this;
}

bool SpriteFrame::operator==(const SpriteFrame& rhs) const
{
	return ((this->m_subsprites == rhs.m_subsprites) &&
		    (*this->m_sprite_gfx == *rhs.m_sprite_gfx) &&
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
	bytes.reserve(static_cast<std::size_t>(filesize));
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
	return GetBits(m_compressed);
}

std::vector<uint8_t> SpriteFrame::GetBits(bool compressed)
{
	std::vector<uint8_t> bits;
	std::size_t expected_tiles = 0;
	std::size_t actual_tiles = m_sprite_gfx->GetTileCount();

	if (m_subsprites.size() > 0)
	{
		for (const auto& s : m_subsprites)
		{
			uint8_t b1, b2;
			b1 = (((s.y < 0) ? (s.y + 0x100) : s.y) >> 1) & 0x7C;
			b1 |= (s.w - 1) & 0x03;
			b2 = (((s.x < 0) ? (s.x + 0x100) : s.x) >> 1) & 0x7C;
			b2 |= (s.h - 1) & 0x03;

			bits.push_back(b1);
			bits.push_back(b2);
			expected_tiles += s.h * s.w;
		}
		bits.back() |= 0x80;
	}

	int last_cmd = bits.size();
	if (compressed) // compression
	{
		uint16_t word_count = static_cast<uint16_t>(std::min<std::size_t>(actual_tiles, expected_tiles) * 16);
		bits.push_back(0x20 | word_count >> 8);
		bits.push_back(word_count & 0xFF);
		auto tiles = m_sprite_gfx->GetBits();
		std::vector<uint8_t> buffer(65536);
		buffer.resize(LZ77::Encode(tiles.data(), word_count*2, buffer.data()));
		bits.insert(bits.end(), buffer.begin(), buffer.end());
	}
	else
	{
		uint16_t total_words = static_cast<uint16_t>(std::min<std::size_t>(actual_tiles, expected_tiles) * 16);
		uint16_t blanks = 0;
		auto tiles = m_sprite_gfx->GetBits();
		uint16_t src_idx = 0;
		uint16_t copy_start_idx = 0;
		uint16_t copy_len = 0;
		const uint16_t THRESHOLD = 4;
		while (src_idx < total_words * 2)
		{
			uint16_t word = (tiles[src_idx] << 8) | tiles[src_idx + 1];
			src_idx += 2;
			bool is_last_word = (src_idx == total_words * 2);

			// If blank, increment blank run counter. If counter > threshold, write out non-blank data.
			// If non-blank, check blank run counter. If counter > threshold, write out blanks. Reset counter.
			// If end, check blank run counter. If counter > threshold, write out blanks. Else, write out data.

			if (word == 0)
			{
				blanks++;
				if ((blanks >= THRESHOLD) && (copy_len > 0))
				{
					// Encountered a run of at least THRESHOLD blanks.
					// We also have data pending. write out what we have so far.
					last_cmd = bits.size();
					bits.push_back(0x00 | (copy_len >> 8));
					bits.push_back(copy_len & 0xFF);
					bits.insert(bits.end(), tiles.begin() + copy_start_idx, tiles.begin() + copy_start_idx + copy_len * 2);
					copy_start_idx += copy_len * 2;
					copy_len = 0;
				}
			}
			else
			{
				copy_len++;
				if (blanks < THRESHOLD)
				{
					copy_len += blanks;
				}
				else
				{
					last_cmd = bits.size();
					bits.push_back(static_cast<uint8_t>(0x80 | (blanks >> 8)));
					bits.push_back(static_cast<uint8_t>(blanks & 0xFF));
					copy_start_idx += blanks * 2;
				}
				blanks = 0;
			}

			if (is_last_word)
			{
				if (blanks >= THRESHOLD)
				{
					last_cmd = bits.size();
					bits.push_back(static_cast<uint8_t>(0x80 | (blanks >> 8)));
					bits.push_back(static_cast<uint8_t>(blanks & 0xFF));
				}
				else
				{
					last_cmd = bits.size();
					bits.push_back(0x00 | (copy_len >> 8));
					bits.push_back(copy_len & 0xFF);
					copy_len += blanks;
					bits.insert(bits.end(), tiles.begin() + copy_start_idx, tiles.begin() + copy_start_idx + copy_len * 2);
				}
			}

		}
	}
	// Fill in padding if required
	if (actual_tiles < expected_tiles)
	{
		last_cmd = bits.size();
		uint16_t word_count = static_cast<uint16_t>((expected_tiles - actual_tiles) * 32);
		bits.push_back(0x80 | (word_count >> 8));
		bits.push_back(word_count & 0xFF);
	}

	// Mark final command to stop decompression
	bits.at(last_cmd) |= 0x40;
	return bits;
}

std::size_t SpriteFrame::SetBits(const std::vector<uint8_t>& src)
{
	auto it = src.begin();
	m_subsprites.clear();
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
		m_subsprites.emplace_back(x,y,w,h,tile_idx);
		tile_idx += w * h;
	} while ((*it++ & 0x80) == 0);

	for (const auto& subs : m_subsprites)
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
#ifndef NDEBUG
			ss.str(std::string());
			ss.clear();
			ss << "Insert " << count << " zero words." << std::endl;
			Debug(ss.str().c_str());
#endif
			dest_it += count * 2;
		}
		else if ((ctrl & 0x02) > 0)
		{
			std::size_t elen = 0;
			std::size_t dlen = LZ77::Decode(&(*it), src.end() - it, &(*dest_it), elen);
			dest_it += dlen;
#ifndef NDEBUG
			ss.str(std::string());
			ss.clear();
			ss << "Copy " << elen << " compressed bytes, " << dlen << " bytes decompressed.";
			Debug(ss.str().c_str());
#endif
			it += elen;
			m_compressed = true;
		}
		else
		{
#ifndef NDEBUG
			ss.str(std::string());
			ss.clear();
			ss << "Copy " << count << " words directly.";
			Debug(ss.str().c_str());
#endif
			std::copy(it, it + count * 2, dest_it);
			dest_it += count * 2;
			it += count * 2;
		}
	} while ((ctrl & 0x04) == 0);

	m_sprite_gfx = std::make_shared<Tileset>(sprite_gfx);

	Debug("Done!");
	return std::distance(src.begin(), it);
}

bool SpriteFrame::Save(const std::string& filename)
{
	return Save(filename, m_compressed);
}

bool SpriteFrame::Save(const std::string& filename, bool compressed)
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

void SpriteFrame::Clear()
{
	m_subsprites.clear();
	m_sprite_gfx->Clear();
}

int SpriteFrame::GetLeft() const
{
	if (m_subsprites.size() == 0)
	{
		return 0;
	}
	int left = 0xFFFFFF;
	for (const auto& s : m_subsprites)
	{
		if (s.x < left)
		{
			left = s.x;
		}
	}
	return left;
}

int SpriteFrame::GetRight() const
{
	if (m_subsprites.size() == 0)
	{
		return 0;
	}
	int right = -0xFFFFFF;
	for (const auto& s : m_subsprites)
	{
		int r = s.x + s.w * m_sprite_gfx->GetTileWidth();
		if (r > right)
		{
			right = r;
		}
	}
	return right;
}

int SpriteFrame::GetWidth() const
{
	return GetRight() - GetLeft();
}

int SpriteFrame::GetTop() const
{
	if (m_subsprites.size() == 0)
	{
		return 0;
	}
	int top = 0xFFFFFF;
	for (const auto& s : m_subsprites)
	{
		if (s.y < top)
		{
			top = s.y;
		}
	}
	return top;
}

int SpriteFrame::GetBottom() const
{
	if (m_subsprites.size() == 0)
	{
		return 0;
	}
	int bottom = -0xFFFFFF;
	for (const auto& s : m_subsprites)
	{
		int b = s.y + s.h * m_sprite_gfx->GetTileHeight();
		if (b > bottom)
		{
			bottom = b;
		}
	}
	return bottom;
}

int SpriteFrame::GetHeight() const
{
	return GetBottom() - GetTop();
}

std::vector<uint8_t> SpriteFrame::GetTile(const Tile& tile) const
{
	return m_sprite_gfx->GetTile(tile);
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
	return m_sprite_gfx->GetTilePixels(tile_index);
}

std::shared_ptr<const Tileset> SpriteFrame::GetTileset() const
{
	return m_sprite_gfx;
}

std::shared_ptr<Tileset> SpriteFrame::GetTileset()
{
	return m_sprite_gfx;
}

std::size_t SpriteFrame::GetTileCount() const
{
	return m_sprite_gfx->GetTileCount();
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
	return m_sprite_gfx->GetTileWidth();
}

std::size_t SpriteFrame::GetTileHeight() const
{
	return m_sprite_gfx->GetTileHeight();
}

std::size_t SpriteFrame::GetTileBitDepth() const
{
	return m_sprite_gfx->GetTileBitDepth();
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

std::vector<SpriteFrame::SubSprite> SpriteFrame::GetSubSprites() const
{
	return m_subsprites;
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

void SpriteFrame::SetSubSprites(const std::vector<SubSprite>& subs)
{
	m_subsprites = subs;
	int c = 0;
	for (auto& s : m_subsprites)
	{
		s.tile_idx += c;
		c += s.w * s.h;
	}
}
