#include <landstalker/2d_maps/include/Tilemap2DRLE.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iterator>
#include <numeric>
#include <algorithm>

#include <landstalker/misc/include/LZ77.h>
#include <landstalker/misc/include/Utils.h>


Tilemap2D::Tilemap2D()
	: m_width(0),
	  m_height(0),
	  m_left(0),
	  m_top(0),
	  m_base(0),
	  m_compression(Compression::NONE)
{
}

Tilemap2D::Tilemap2D(size_t width, size_t height, size_t base)
	: m_width(width),
	  m_height(height),
	  m_left(0),
	  m_top(0),
	  m_base(base),
	  m_compression(Compression::NONE)
{
	m_tiles.resize(width * height);
}

Tilemap2D::Tilemap2D(const std::string& filename, Compression compression, size_t base)
	: m_width(0),
	  m_height(0),
	  m_left(0),
	  m_top(0),
	  m_base(base),
	  m_compression(compression)
{
	Open(filename, compression, base);
}

Tilemap2D::Tilemap2D(const std::string& filename, size_t width, size_t height, Compression compression, size_t base)
	: m_width(width),
	  m_height(height),
	  m_left(0),
	  m_top(0),
	  m_base(base),
	  m_compression(compression)
{
	Open(filename, width, height, compression, base);
}

Tilemap2D::Tilemap2D(const std::vector<uint8_t>& data, Compression compression, size_t base)
	: m_width(0),
	  m_height(0),
	  m_left(0),
	  m_top(0),
	  m_base(base),
	  m_compression(compression)
{
	Open(data, compression, base);
}

Tilemap2D::Tilemap2D(const std::vector<uint8_t>& data, size_t width, size_t height, Compression compression, size_t base)
	: m_width(width),
	  m_height(height),
	  m_left(0),
	  m_top(0),
	  m_base(base),
	  m_compression(compression)
{
	Open(data, width, height, compression, base);
}

bool Tilemap2D::operator==(const Tilemap2D& rhs) const
{
	return ((this->m_width == rhs.m_width) &&
		(this->m_height == rhs.m_height) &&
		(this->m_top == rhs.m_top) &&
		(this->m_left == rhs.m_left) &&
		(this->m_tiles == rhs.m_tiles));
}

bool Tilemap2D::operator!=(const Tilemap2D& rhs) const
{
	return !(*this == rhs);
}

bool Tilemap2D::Open(const std::string& filename, Tilemap2D::Compression compression, size_t base)
{
	m_base = base;
	m_compression = compression;

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
		Open(bytes, compression, base);
		retval = true;
	}
	return retval;

}

bool Tilemap2D::Open(const std::string& filename, size_t width, size_t height, Tilemap2D::Compression compression, size_t base)
{
	m_width = width;
	m_height = height;
	return Open(filename, compression, base);
}

uint32_t Tilemap2D::Open(const std::vector<uint8_t>& data, Tilemap2D::Compression compression, size_t base)
{
	uint32_t retval = false;
	m_base = base;
	m_compression = compression;

	switch (m_compression)
	{
	case Compression::NONE:
		if ((m_width > 0) && (m_height > 0) && (data.size() >= m_width * m_height * 2))
		{
			retval = UnpackBytes(data);
		}
		break;
	case Compression::RLE:
		try
		{
			retval = Uncompress(data);
		}
		catch (const std::exception&)
		{
			retval = 0;
		}
		break;
	case Compression::LZ77:
		if (data.size() > 0)
		{
			try
			{
				retval = UncompressLZ77(data);
			}
			catch (const std::exception&)
			{
				retval = 0;
			}
		}
		break;
	}

	return retval;
}

uint32_t Tilemap2D::Open(const std::vector<uint8_t>& data, size_t width, size_t height, Tilemap2D::Compression compression, size_t base)
{
	m_width = width;
	m_height = height;
	return Open(data, compression, base);
}

bool Tilemap2D::GetBits(std::vector<uint8_t>& data, Compression compressed)
{
	std::vector<uint8_t> tile_data;
	data.clear();
	try
	{
		switch (compressed)
		{
		case Compression::NONE:
			tile_data = PackBytes();
			break;
		case Compression::LZ77:
			tile_data = CompressLZ77();
			break;
		case Compression::RLE:
			tile_data = Compress();
			break;
		default:
			throw std::runtime_error("Bad Compression Value");
		}
	}
	catch (std::exception& e)
	{
		Debug("Error compressing map:");
		Debug(e.what());
		return false;
	}
	data.reserve(tile_data.size());
	std::copy(tile_data.begin(), tile_data.end(), std::back_inserter(data));
	return true;
}

bool Tilemap2D::Save(const std::string& filename, Compression compressed)
{
	std::vector<uint8_t> bits;
	bool retval = GetBits(bits, compressed);
	std::ofstream ofs(filename, std::ios::out | std::ios::binary);
	if (retval && ofs)
	{
		ofs.write(reinterpret_cast<const char*>(bits.data()), bits.size());
		ofs.close();
	}
	else
	{
		retval = false;
	}
	return retval;
}

Tilemap2D::Compression Tilemap2D::GetCompression() const
{
	return m_compression;
}

void Tilemap2D::SetCompression(Tilemap2D::Compression c)
{
	m_compression = c;
}

std::string Tilemap2D::GetFileExtension(Tilemap2D::Compression c)
{
	switch (c)
	{
	case Compression::RLE:
		return ".rle";
	case Compression::LZ77:
		return ".lz77";
	default:
	case Compression::NONE:
		return ".bin";
	}
}

std::string Tilemap2D::GetFileExtension() const
{
	return GetFileExtension(m_compression);
}

Tilemap2D::Compression Tilemap2D::FromFileExtension(const std::string& filename)
{
	std::string ext = str_to_lower(std::filesystem::path(filename).extension().string());
	if (ext == ".lz77")
	{
		return Compression::LZ77;
	}
	else if (ext == ".rle")
	{
		return Compression::RLE;
	}
	return Compression::NONE;
}

void Tilemap2D::Clear()
{
	m_tiles.clear();
}

void Tilemap2D::Fill(const Tile& tile)
{
	std::fill(m_tiles.begin(), m_tiles.end(), tile);
}

void Tilemap2D::FillIncrementing(const Tile& tile)
{
	std::iota(m_tiles.begin(), m_tiles.end(), tile);
}

std::vector<uint8_t> Tilemap2D::Compress() const
{
	std::vector<uint8_t> cmp;

	cmp.push_back(m_width & 0xFF);
	cmp.push_back(m_height & 0xFF);
	
	// First, encode tile parameters as RLE
	size_t idx = 0;
	size_t count;
	while (idx < m_tiles.size())
	{
		for (count = 0; idx + count + 1 < m_tiles.size() && count < 0x400; ++count)
		{
			if ((m_tiles[idx].GetTileValue() & 0xF800) != (m_tiles[idx + count + 1].GetTileValue() & 0xF800))
			{
				break;
			}
		}
		uint8_t byte1 = (m_tiles[idx].GetTileValue() >> 8) & 0xF8;
		idx += count + 1;
		if (count > 0x04)
		{
			byte1 |= (count) >> 8;
			cmp.push_back(byte1);
			cmp.push_back(count & 0xFF);
		}
		else
		{
			byte1 |= (count) & 0x03;
			byte1 |= 0x04;
			cmp.push_back(byte1);
		}
	}
	cmp.push_back(0x00);
	cmp.push_back(0x00);
	// Next, encode RLE
	uint16_t last = m_tiles.front().GetIndex();
	uint16_t incr = m_tiles.front().GetIndex();
	// First, we need to write out a single RLE run in order to set up our registers
	count = 0;
	for (auto it = m_tiles.cbegin() + 1; it != m_tiles.cend(); ++it)
	{
		if (it->GetIndex() != m_tiles.cbegin()->GetIndex())
		{
			break;
		}
		count++;
		if (count >= 7)
		{
			break;
		}
	}
	uint8_t byte = 0x40 | ((count & 0x07) << 3) | ((last >> 8) & 0x07);
	cmp.push_back(byte);
	cmp.push_back(last & 0xFF);
	// Now we can proceed with the RLE encoding
	auto it = m_tiles.cbegin() + count + 1;
	while (it != m_tiles.cend())
	{
		if (it->GetIndex() == last)
		{
			// Best option : RLE Fill last
			count = 0;
			for (auto jt = it + 1; jt != m_tiles.cend(); ++jt)
			{
				if (jt->GetIndex() != last)
				{
					break;
				}
				count++;
				if (count >= 0x3F)
				{
					break;
				}
			}
			cmp.push_back(0x80 | (count & 0x3F));
			it += count + 1;
		}
		else if (it->GetIndex() == incr + 1)
		{
			// Best option : RLE Increment
			count = 0;
			incr++;
			for (auto jt = it + 1; jt != m_tiles.cend(); ++jt)
			{
				if (jt->GetIndex() != ++incr)
				{
					incr--;
					break;
				}
				count++;
				if (count >= 0x3F)
				{
					break;
				}
			}
			cmp.push_back(0xC0 | (count & 0x3F));
			it += count + 1;
		}
		else
		{
			if (std::next(it) != m_tiles.cend() && std::next(it)->GetIndex() == it->GetIndex())
			{
				// Best option : RLE Fill
				count = 0;
				for (auto jt = it + 1; jt != m_tiles.cend(); ++jt)
				{
					if (it->GetIndex() != jt->GetIndex())
					{
						break;
					}
					count++;
					if (count >= 7)
					{
						break;
					}
				}
				cmp.push_back(0x40 | ((it->GetIndex() >> 8) & 0x07) | ((count & 0x07) << 3));
				cmp.push_back(it->GetIndex() & 0xFF);
				last = it->GetIndex();
				it += count + 1;
			}
			else
			{
				// Best option : Copy
				cmp.push_back(it->GetIndex() >> 8);
				cmp.push_back(it->GetIndex() & 0xFF);
				it++;
			}
		}
	}


	cmp.push_back(0x07);
	cmp.push_back(0xFF);

	return cmp;
}

uint32_t Tilemap2D::Uncompress(const std::vector<uint8_t>& data)
{
	auto d = data.begin();
	const std::size_t size = data.size();
	if (size <= 6)
	{
		throw std::runtime_error("Compressed data size is too small");
	}
	m_width = *d++;
	m_height = *d++;
	m_tiles.clear();
	m_tiles.reserve(m_width * m_height);

	// First, the tile attributes (priority, palette, HFLIP, VFLIP)
	// These are stored using simple RLE
	bool done = false;
	size_t idx = 0;
	while (done == false)
	{
		uint16_t attrs = (*d & 0xF8) << 8;
		uint16_t length = *d++ & 0x03;
		if (d >= data.end())
		{
			throw std::runtime_error("Unexpected end of compressed data.");
		}
		if ((*(d - 1) & 0x04) == 0)
		{
			length <<= 8;
			length |= *d++;
			if (length == 0)
			{
				// End of attributes
				done = true;
				break;
			}
			else if (d >= data.end())
			{
				throw std::runtime_error("Unexpected end of compressed data.");
			}
		}
		if (length + idx >= m_width * m_height)
		{
			throw std::runtime_error("Compressed data is bad or corrupted.");
		}
		do
		{
			m_tiles.push_back(attrs);
		} while (length--);
	}
	if (m_tiles.size() != m_width * m_height)
	{
		throw std::runtime_error("Compressed data is bad or corrupted.");
	}
	// Next, we load the tile indicies
	idx = 0;
	done = false;
	uint16_t value = 0;
	uint16_t count = 0;
	uint16_t last_value = 0xBEEF;
	uint16_t incr_value = 0xBEEF;
	uint8_t cmd = 0;
	while (done == false)
	{
		cmd = *d >> 6;
		switch (cmd)
		{
		case 0: // Single tile copy
			if (d + 1 >= data.end())
			{
				throw std::runtime_error("Unexpected end of compressed data.");
			}
			value = *d++ << 8;
			value |= *d++;
			value &= 0x07FF;
			if (value == 0x7FF)
			{
				done = true;
			}
			else
			{
				if (idx >= m_tiles.size())
				{
					throw std::runtime_error("Compressed data is bad or corrupted.");
				}
				m_tiles[idx++].SetIndex(value);
			}
			break;
		case 1: // RLE tile copy
			if (d + 1 >= data.end())
			{
				throw std::runtime_error("Unexpected end of compressed data.");
			}
			count = ((*d & 0x38) >> 3);
			value = *d++ << 8;
			value |= *d++;
			value &= 0x07FF;
			if (idx + count >= m_tiles.size())
			{
				throw std::runtime_error("Compressed data is bad or corrupted.");
			}
			do
			{
				m_tiles[idx++].SetIndex(value);
			} while (count--);
			last_value = value;
			if (incr_value == 0xBEEF)
			{
				incr_value = value;
			}
			break;
		case 2: // RLE last tile copy
			count = *d++ & 0x3F;
			if (d >= data.end())
			{
				throw std::runtime_error("Unexpected end of compressed data.");
			}
			if (last_value == 0xBEEF)
			{
				throw std::runtime_error("Compressed data is bad or corrupted.");
			}
			else if (idx + count >= m_tiles.size())
			{
				throw std::runtime_error("Compressed data is bad or corrupted.");
			}
			do
			{
				m_tiles[idx++].SetIndex(last_value);
			} while (count--);
			break;
		case 3: // RLE increment
			count = *d++ & 0x3F;
			if (d >= data.end())
			{
				throw std::runtime_error("Unexpected end of compressed data.");
			}
			if (incr_value == 0xBEEF)
			{
				throw std::runtime_error("Compressed data is bad or corrupted.");
			}
			else if (idx + count >= m_tiles.size())
			{
				throw std::runtime_error("Compressed data is bad or corrupted.");
			}
			do
			{
				m_tiles[idx++].SetIndex(++incr_value);
			} while (count--);
			break;
		}
	}
	return std::distance(data.begin(), d);
}

std::vector<uint8_t> Tilemap2D::CompressLZ77() const
{
	auto bytes = PackBytes();
	bytes.insert(bytes.begin(), {0,0,0,0});
	bytes[0] = static_cast<uint8_t>(m_left);
	bytes[1] = static_cast<uint8_t>(m_top);
	bytes[2] = static_cast<uint8_t>(m_width);
	bytes[3] = static_cast<uint8_t>(m_height);
	std::vector<uint8_t> result(65536);
	auto elen = LZ77::Encode(bytes.data(), bytes.size(), result.data());
	result.resize(elen);
	return result;
}

uint32_t Tilemap2D::UncompressLZ77(const std::vector<uint8_t>& data)
{
	std::size_t elen = 0;
	std::vector<uint8_t> result(65536);
	auto dlen = LZ77::Decode(data.data(), data.size(), result.data(), elen);
	result.resize(dlen);
	if (result.size() < 4) throw std::runtime_error("Bad LZ77 compressed map!");
	m_left = result[0];
	m_top = result[1];
	m_width = result[2];
	m_height = result[3];
	result.erase(result.begin(), result.begin() + 4);
	if (result.size() < (m_width * m_height * 2)) throw std::runtime_error("Bad LZ77 compressed map!");
	UnpackBytes(result);
	return elen;
}

uint32_t Tilemap2D::UnpackBytes(const std::vector<uint8_t>& data)
{
	m_tiles.clear();
	m_tiles.reserve(m_width * m_height);
	uint32_t size = std::min<uint32_t>(data.size(), m_width * m_height * 2);
	for (std::size_t i = 0; i < size; i += 2)
	{
		uint16_t val = (data[i] << 8) | data[i + 1];
		m_tiles.push_back(val);
	}
	return size;
}

std::vector<uint8_t> Tilemap2D::PackBytes() const
{
	std::vector<uint8_t> result;
	result.reserve(m_tiles.size() * 2);
	for (const auto& t : m_tiles)
	{
		result.push_back(t.GetTileValue() >> 8);
		result.push_back(t.GetTileValue() & 0xFF);
	}
	return result;
}

size_t Tilemap2D::GetHeight() const
{
	return m_height;
}

size_t Tilemap2D::GetWidth() const
{
	return m_width;
}

size_t Tilemap2D::GetBase() const
{
	return m_base;
}

size_t Tilemap2D::GetLeft() const
{
	return m_left;
}

size_t Tilemap2D::GetTop() const
{
	return m_top;
}

void Tilemap2D::SetLeft(uint8_t left)
{
	m_left = left;
}

void Tilemap2D::SetTop(uint8_t top)
{
	m_top = top;
}

void Tilemap2D::SetBase(uint16_t base)
{
	m_base = base;
}

Tile Tilemap2D::GetTile(size_t x, size_t y) const
{
	auto ret = Tile();
	if (IsTileValid(x, y))
	{
		ret = m_tiles[y * m_width + x];
		ret.SetIndex(0x7FF & (ret.GetIndex() - m_base));
	}
	return ret;
}

void Tilemap2D::SetTile(const Tile& tile, size_t x, size_t y)
{
	if (IsTileValid(x, y))
	{
		Tile t = tile;
		t.SetIndex(0x7FF & (t.GetIndex() + m_base));
		m_tiles[y * m_width + x] = t;
	}
}

bool Tilemap2D::IsTileValid(int x, int y) const
{
	bool retval = false;
	if ((x >= 0) && (x < static_cast<int>(m_width)) &&
		(y >= 0) && (y < static_cast<int>(m_height)))
	{
		retval = true;
	}
	return retval;
}

void Tilemap2D::InsertRow(int position, const Tile& fill)
{
	m_height += 1;
	std::vector<Tile> old = m_tiles;
	m_tiles.resize(m_width * m_height);
	auto oit = old.cbegin();
	auto nit = m_tiles.begin();
	for (std::size_t y = 0; y < m_height; ++y)
	{
		for (std::size_t x = 0; x < m_width; ++x)
		{
			if (y == static_cast<std::size_t>(position))
			{
				*nit++ = fill;
			}
			else
			{
				*nit++ = *oit++;
			}
		}
	}
}

void Tilemap2D::InsertColumn(int position, const Tile& fill)
{
	m_width += 1;
	std::vector<Tile> old = m_tiles;
	m_tiles.resize(m_width * m_height);
	auto oit = old.cbegin();
	auto nit = m_tiles.begin();
	for (std::size_t y = 0; y < m_height; ++y)
	{
		for (std::size_t x = 0; x < m_width; ++x)
		{
			if (x == static_cast<std::size_t>(position))
			{
				*nit++ = fill;
			}
			else
			{
				*nit++ = *oit++;
			}
		}
	}
}

void Tilemap2D::DeleteRow(int position)
{
	if (m_height < 2)
	{
		return;
	}
	m_height -= 1;
	std::vector<Tile> old = m_tiles;
	m_tiles.resize(m_width * m_height);
	auto oit = old.cbegin();
	auto nit = m_tiles.begin();
	for (std::size_t y = 0; y < m_height + 1; ++y)
	{
		for (std::size_t x = 0; x < m_width; ++x)
		{
			if (y == static_cast<std::size_t>(position))
			{
				oit++;
			}
			else
			{
				*nit++ = *oit++;
			}
		}
	}
}

void Tilemap2D::DeleteColumn(int position)
{
	if (m_width < 2)
	{
		return;
	}
	m_width -= 1;
	std::vector<Tile> old = m_tiles;
	m_tiles.resize(m_width * m_height);
	std::vector<Tile>::const_iterator oit = old.cbegin();
	std::vector<Tile>::iterator nit = m_tiles.begin();
	for (std::size_t y = 0; y < m_height; ++y)
	{
		for (std::size_t x = 0; x < m_width + 1; ++x)
		{
			if (x == static_cast<std::size_t>(position))
			{
				oit++;
			}
			else
			{
				*nit++ = *oit++;
			}
		}
	}
}

void Tilemap2D::Resize(int width, int height)
{
	m_width = width;
	m_height = height;
	m_tiles.resize(width * height);
}

Tile* Tilemap2D::Data()
{
	return m_tiles.data();
}
