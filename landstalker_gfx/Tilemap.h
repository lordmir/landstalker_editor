#ifndef TILEMAP_H
#define TILEMAP_H

#include <cstdint>
#include <memory>
#include <wx/gdicmn.h>

#include "Tileset.h"
#include "Palette.h"
#include "ImageBuffer.h"

struct TilePoint
{
	size_t x;
	size_t y;
};

class Tilemap
{
public:
	Tilemap(size_t width, size_t height, size_t left = 0, size_t top = 0, uint8_t palette = 0);
	Tilemap() = delete;
	virtual ~Tilemap() = default;
	size_t GetWidth() const;
	size_t GetHeight() const;
	size_t GetLeft() const;
	size_t GetTop() const;
	void SetLeft(size_t new_left);
	void SetTop(size_t new_top);
	void Resize(size_t new_width, size_t new_height);
	uint8_t GetPalette() const;
	void SetPalette(uint8_t palette);
	uint16_t GetTileValue(const TilePoint& point) const;
	void SetTileValue(const TilePoint& point, uint16_t index);
	void Fill(uint16_t base, size_t increment = 0);
	void Copy(const uint8_t* src, uint16_t base = 0);
	void Copy(std::vector<uint16_t>::const_iterator begin, std::vector<uint16_t>::const_iterator end);
	void Clear();
	bool IsXYPointValid(const wxPoint& point) const;
	bool WriteBinaryFile(const std::string& filename, bool include_dimensions);
	bool WriteCSVFile(const std::string& filename);
	bool ReadBinaryFile(const std::string& filename, bool dimensions_included);
	bool ReadCSVFile(const std::string& filename);

	virtual TilePoint XYToTilePoint(const wxPoint& point) const = 0;
	virtual wxPoint ToXYPoint(const TilePoint& point) const = 0;
	virtual void Draw(ImageBuffer& imgbuf) const = 0;
	virtual size_t GetBitmapWidth() const = 0;
	virtual size_t GetBitmapHeight() const = 0;
protected:
	static const size_t TILEWIDTH;
	static const size_t TILEHEIGHT;
	std::vector<uint16_t> m_tilevals;

private:
	size_t m_width;
	size_t m_height;
	size_t m_left;
	size_t m_top;
	uint8_t m_palette;
};

#endif // TILEMAP_H