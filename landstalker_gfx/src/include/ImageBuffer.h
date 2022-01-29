#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H

#include <vector>
#include <memory>
#include <wx/image.h>
#include <wx/bitmap.h>

#include "Tile.h"
#include "Tileset.h"
#include "Palette.h"
#include "Block.h"
#include "Tilemap2DRLE.h"

class ImageBuffer
{
public:
	ImageBuffer();
	ImageBuffer(std::size_t width, std::size_t height);
	void Clear();
	void Resize(std::size_t width, std::size_t height);
	void InsertTile(int x, int y, uint8_t palette_index, const Tile& tile, const Tileset& tileset);
	void InsertMap(int x, int y, uint8_t palette_index, const Tilemap2D& map, const Tileset& tileset);
	bool WritePNG(const std::string& filename, const std::vector<Palette>& pals);
	void InsertBlock(std::size_t x, std::size_t y, uint8_t palette_index, const Block& block, const Tileset& tileset);
	const std::vector<uint8_t>& GetRGB(const std::vector<Palette>& pals) const;
	const std::vector<uint8_t>& GetAlpha(const std::vector<Palette>& pals, uint8_t low_pri_max_opacity = 0xFF, uint8_t high_pri_max_opacity = 0xFF) const;
	std::shared_ptr<wxBitmap> MakeBitmap(const std::vector<Palette>& pals, bool use_alpha = false, uint8_t low_pri_max_opacity = 0xFF, uint8_t high_pri_max_opacity = 0xFF) const;
	std::size_t GetHeight() const;
	std::size_t GetWidth() const;
private:
	std::size_t m_width;
	std::size_t m_height;
	std::vector<uint8_t> m_pixels;
	std::vector<uint8_t> m_priority;
	mutable std::vector<uint8_t> m_rgb;
	mutable std::vector<uint8_t> m_alpha;
	mutable wxImage m_img;
};

#endif // IMAGE_BUFFER_H
