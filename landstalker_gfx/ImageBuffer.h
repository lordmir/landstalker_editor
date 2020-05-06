#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H

#include <vector>
#include <memory>
#include <wx/image.h>
#include <wx/bitmap.h>

#include "Tile.h"
#include "Tileset.h"
#include "Palette.h"
#include "BigTile.h"

class ImageBuffer
{
public:
	ImageBuffer();
	ImageBuffer(size_t width, size_t height);
	void Clear();
	void Resize(size_t width, size_t height);
	void InsertTile(size_t x, size_t y, uint8_t palette_index, const Tile& tile, const Tileset& tileset);
	bool WritePNG(const std::string& filename, const std::vector<Palette>& pals);
	void InsertBlock(size_t x, size_t y, uint8_t palette_index, const BigTile& block, const Tileset& tileset);
	std::vector<uint8_t> GetRGB(const std::vector<Palette>& pals) const;
	std::vector<uint8_t> GetAlpha(const std::vector<Palette>& pals) const;
	std::shared_ptr<wxBitmap> MakeBitmap(const std::vector<Palette>& pals) const;
	size_t GetHeight() const;
	size_t GetWidth() const;
private:
	size_t m_width;
	size_t m_height;
	std::vector<uint8_t> m_pixels;
	mutable wxImage m_img;
};

#endif // IMAGE_BUFFER_H
