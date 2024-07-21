#include <landstalker/2d_maps/include/Tilemap.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>
#include <landstalker/misc/include/Utils.h>

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

Tilemap::Tilemap(std::size_t width, std::size_t height, std::size_t tile_width, std::size_t tile_height, std::size_t left, std::size_t top, uint8_t palette)
	: TILE_WIDTH(tile_width),
	  TILE_HEIGHT(tile_height),
	  m_width(width),
	  m_height(height),
	  m_left(left),
	  m_top(top),
	  m_palette(palette)
{
	m_tilevals.resize(width * height);
}

bool Tilemap::operator==(const Tilemap& rhs)
{
	return ((this->m_height == rhs.m_height) &&
		(this->m_left == rhs.m_left) &&
		(this->m_top == rhs.m_top) &&
		(this->m_left == rhs.m_left) &&
		(this->m_tilevals == rhs.m_tilevals));
}

std::size_t Tilemap::GetWidth() const
{
	return m_width;
}

std::size_t Tilemap::GetHeight() const
{
	return m_height;
}

std::size_t Tilemap::GetLeft() const
{
	return m_left * TILE_WIDTH;
}

std::size_t Tilemap::GetTop() const
{
	return m_top * TILE_HEIGHT;
}

std::size_t Tilemap::GetTileWidth() const
{
	return TILE_WIDTH;
}

std::size_t Tilemap::GetTileHeight() const
{
	return TILE_HEIGHT;
}

void Tilemap::SetLeft(std::size_t new_left)
{
	m_left = new_left;
}

void Tilemap::SetTop(std::size_t new_top)
{
	m_top = new_top;
}

void Tilemap::Resize(std::size_t new_width, std::size_t new_height)
{
	if ((m_width == 0) || (m_height == 0))
	{
		m_tilevals.clear();
		m_tilevals.assign(new_width * new_height, 0);
	}
	else
	{
		if (new_height < GetHeight())
		{
			m_tilevals.erase(std::next(m_tilevals.begin(), new_height * GetWidth()), m_tilevals.end());
		}
		else if (new_height > GetHeight())
		{
			m_tilevals.resize(GetWidth() * new_height);
		}
		if (new_width < GetWidth())
		{
			for (std::size_t y = 0; y < new_height; ++y)
			{
				auto row_it = std::next(m_tilevals.begin(), y * new_width);
				m_tilevals.erase(std::next(row_it, new_width), std::next(row_it, GetWidth()));
				row_it += new_width;
			}
		}
		else if (new_width > GetWidth())
		{
			std::size_t diff = new_width - GetWidth();
			for (std::size_t y = 0; y < new_height; ++y)
			{
				auto row_it = std::next(m_tilevals.begin(), y * new_width);
				m_tilevals.insert(std::next(row_it, GetWidth()), diff, 0);
			}
		}
	}
	m_height = new_height;
	m_width = new_width;
}

uint8_t Tilemap::GetPalette() const
{
	return m_palette;
}

void Tilemap::SetPalette(uint8_t palette)
{
	m_palette = palette;
}

uint16_t Tilemap::GetTileValue(const TilePoint& point) const
{
	return m_tilevals[point.y * m_width + point.x];
}

void Tilemap::SetTileValue(const TilePoint& point, uint16_t index)
{
	m_tilevals[point.y * m_width + point.x] = index;
}


void Tilemap::Fill(uint16_t base, std::size_t increment, int limit)
{
	uint16_t index = base;
	std::size_t i = 0;
	for (uint16_t t = base; (t < limit || limit == -1) && i < m_tilevals.size(); t += increment, i++)
	{
		m_tilevals[i] = index;
		index += increment;
	}
}

void Tilemap::Copy(const uint8_t* src, uint16_t base)
{
	for (auto& tile : m_tilevals)
	{
		uint16_t tileval = (src[0] << 8 | src[1]) + base;
		tile = tileval;
	}
}

void Tilemap::Copy(std::vector<uint16_t>::const_iterator begin, std::vector<uint16_t>::const_iterator end)
{
	if (static_cast<std::size_t>(std::distance(begin, end)) <= (m_width * m_height))
	{
		std::copy(begin, end, m_tilevals.begin());
	}
	else
	{
		std::ostringstream ss;
		ss << "Attempt to copy " << std::distance(begin, end) << " tiles into a tilemap of size " << m_tilevals.size();
		Debug(ss.str());
	}
}

void Tilemap::Clear()
{
	std::fill(m_tilevals.begin(), m_tilevals.end(), 0);
	m_tilevals.resize(GetWidth() * GetHeight());
}

bool Tilemap::IsXYPointValid(const wxPoint& point) const
{
	bool retval = false;
	TilePoint tilepoint(XYToTilePoint(point));
	if ((tilepoint.x >= GetLeft()) && (tilepoint.x < (GetLeft() + GetWidth())) &&
		(tilepoint.y >= GetTop()) && (tilepoint.y < (GetTop() + GetHeight())))
	{
		retval = true;
	}
	return retval;
}

bool Tilemap::WriteBinaryFile(const std::string& filename, bool include_dimensions)
{
	bool retval = false;
	std::ofstream outfile(filename, std::ios::out | std::ios::binary);
	if (outfile.good())
	{
		if (include_dimensions)
		{
			uint16_t width = htons(m_width);
			uint16_t height = htons(m_height);
			uint16_t left = htons(m_left);
			uint16_t top = htons(m_top);
			outfile.write(reinterpret_cast<char*>(&width), sizeof(width));
			outfile.write(reinterpret_cast<char*>(&height), sizeof(height));
			outfile.write(reinterpret_cast<char*>(&left), sizeof(left));
			outfile.write(reinterpret_cast<char*>(&top), sizeof(top));
		}
		for (std::size_t y = 0; y < m_height; ++y)
		{
			for (std::size_t x = 0; x < m_width; ++x)
			{
				uint16_t temp = htons(GetTileValue({ x,y }));
				outfile.write(reinterpret_cast<char*>(&temp), sizeof(temp));
			}
		}
		retval = true;
	}
	outfile.close();
	return retval;
}

bool Tilemap::WriteCSVFile(const std::string& filename)
{
	bool retval = false;
	std::ofstream outfile(filename, std::ios::out);
	if (outfile.good())
	{
		outfile << std::dec << m_width << "," << m_height << "," << m_left << "," << m_top << std::endl;
		for (std::size_t y = 0; y < m_height; ++y)
		{
			for (std::size_t x = 0; x < m_width; ++x)
			{
				outfile << std::hex << std::setw(4) << std::setfill('0') << std::uppercase << GetTileValue({ x,y });
				if ((x + 1) < m_width)
				{
					outfile << ",";
				}
			}
			outfile << std::endl;
		}
		retval = true;
	}
	outfile.close();
	return retval;
}

bool Tilemap::ReadBinaryFile(const std::string& filename, bool dimensions_included)
{
	std::ifstream infile(filename, std::ios::in | std::ios::binary);
	bool retval = false;
	if (infile.good())
	{
		std::vector<uint16_t> vals;
		uint16_t width = 0;
		uint16_t height = 0;
		uint16_t left = 0;
		uint16_t top = 0;
		if (dimensions_included)
		{
			infile.read(reinterpret_cast<char*>(&width), sizeof(width));
			infile.read(reinterpret_cast<char*>(&height), sizeof(height));
			infile.read(reinterpret_cast<char*>(&left), sizeof(left));
			infile.read(reinterpret_cast<char*>(&top), sizeof(top));
		}
		else
		{
			width = m_width;
			height = m_height;
		}
		vals.reserve(width * height);
		for (std::size_t y = 0; y < m_height; ++y)
		{
			for (std::size_t x = 0; x < m_width; ++x)
			{
				uint16_t temp = 0;
				infile.read(reinterpret_cast<char*>(&temp), sizeof(temp));
				vals.push_back(ntohs(temp));
			}
		}
		retval = infile.good() && vals.size() == static_cast<std::size_t>(width*height);
		if (retval == true)
		{
			if (dimensions_included)
			{
				m_width = ntohs(width);
				m_height = ntohs(width);
				m_left = ntohs(width);
				m_top = ntohs(width);
			}
			Clear();
			for (std::size_t y = 0; y < m_height; ++y)
			{
				for (std::size_t x = 0; x < m_width; ++x)
				{
					SetTileValue({ x,y }, vals[y * m_width + x]);
				}
			}
		}
	}
	return retval;
}

bool Tilemap::ReadCSVFile(const std::string& filename)
{
	std::ifstream infile(filename, std::ios::in);
	bool retval = false;
	if (infile.good())
	{
		std::vector<uint16_t> vals;
		uint16_t width;
		uint16_t height;
		uint16_t left;
		uint16_t top;

		std::string line;
		std::string val;
		std::getline(infile, line);
		std::istringstream iss(line);

		std::getline(iss, val, ',');
		width  = std::stoul(val);
		std::getline(iss, val, ',');
		height = std::stoul(val);
		std::getline(iss, val, ',');
		left   = std::stoul(val);
		std::getline(iss, val, ',');
		top    = std::stoul(val);
		vals.reserve(width * height);
		for (std::size_t y = 0; y < height; ++y)
		{
			std::getline(infile, line);
			iss.str(line);
			for (std::size_t x = 0; x < width; ++x)
			{
				std::getline(iss, val, ',');
				vals.push_back(std::stoul(val, nullptr, 16));
			}
		}
		retval = infile.good() && vals.size() == static_cast<std::size_t>(width * height);
		if (retval == true)
		{
			m_width = width;
			m_height = height;
			m_left = left;
			m_top = top;
			Clear();
			for (std::size_t y = 0; y < m_height; ++y)
			{
				for (std::size_t x = 0; x < m_width; ++x)
				{
					SetTileValue({ x,y }, vals[y * m_width + x]);
				}
			}
		}
	}
	infile.close();
	return retval;
}

std::size_t Tilemap::GetBitmapWidth() const
{
	return GetWidthInTiles() * TILE_WIDTH;
}

std::size_t Tilemap::GetBitmapHeight() const
{
	return GetHeightInTiles() * TILE_HEIGHT;
}
