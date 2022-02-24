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
	std::size_t x;
	std::size_t y;
};

class Tilemap
{
public:
	Tilemap(std::size_t width, std::size_t height, std::size_t tile_width, std::size_t tile_height, std::size_t left = 0, std::size_t top = 0, uint8_t palette = 0);
	Tilemap() = delete;
	virtual ~Tilemap() = default;
	std::size_t GetWidth() const;
	std::size_t GetHeight() const;
	std::size_t GetLeft() const;
	std::size_t GetTop() const;
	std::size_t GetTileWidth() const;
	std::size_t GetTileHeight() const;
	void SetLeft(std::size_t new_left);
	void SetTop(std::size_t new_top);
	void Resize(std::size_t new_width, std::size_t new_height);
	uint8_t GetPalette() const;
	void SetPalette(uint8_t palette);
	uint16_t GetTileValue(const TilePoint& point) const;
	void SetTileValue(const TilePoint& point, uint16_t index);
	void Fill(uint16_t base, std::size_t increment = 0);
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
	virtual std::size_t GetWidthInTiles() const = 0;
	virtual std::size_t GetHeightInTiles() const = 0;
	virtual std::size_t GetBitmapWidth() const;
	virtual std::size_t GetBitmapHeight() const;
protected:
	const std::size_t TILE_WIDTH;
	const std::size_t TILE_HEIGHT;
	std::vector<uint16_t> m_tilevals;

private:
	std::size_t m_width;
	std::size_t m_height;
	std::size_t m_left;
	std::size_t m_top;
	uint8_t m_palette;
};

#endif // TILEMAP_H