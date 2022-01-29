#ifndef _TILEMAP_2D_RLE_
#define _TILEMAP_2D_RLE_

#include <vector>
#include <cstdint>
#include <cstdlib>
#include "Tile.h"

class Tilemap2D
{
public:
	enum Compression
	{
		NONE,
		RLE,
		LZ77
	};

	Tilemap2D(size_t width, size_t height, size_t base = 0);
	Tilemap2D(const std::string& filename, Compression compression = RLE, size_t base = 0);
	Tilemap2D(const std::string& filename, size_t width, size_t height, Compression compression = NONE, size_t base = 0);
	Tilemap2D(const std::vector<uint8_t>& data, Compression compression = RLE, size_t base = 0);
	Tilemap2D(const std::vector<uint8_t>& data, size_t width, size_t height, Compression compression = NONE, size_t base = 0);

	bool Open(const std::string& filename, Compression compression = RLE, size_t base = 0);
	bool Open(const std::string& filename, size_t width, size_t height, Compression compression = NONE, size_t base = 0);
	bool Open(const std::vector<uint8_t>& data, Compression compression = RLE, size_t base = 0);
	bool Open(const std::vector<uint8_t>& data, size_t width, size_t height, Compression compression = NONE, size_t base = 0);

	bool GetBits(std::vector<uint8_t>& data, Compression compression = NONE);
	bool Save(const std::string& filename, Compression compression = NONE);
	Compression GetCompression() const;

	void Clear();
	void Fill(const Tile& tile);
	void FillIncrementing(const Tile& tile);

	size_t GetHeight() const;
	size_t GetWidth() const;
	size_t GetBase() const;
	size_t GetLeft() const;
	size_t GetTop() const;
	void SetLeft(uint8_t left);
	void SetTop(uint8_t top);

	Tile GetTile(size_t x, size_t y) const;
	void SetTile(const Tile& tile, size_t x, size_t y);
	bool IsTileValid(int x, int y) const;

	void InsertRow(int position);
	void InsertColumn(int position);
	void DeleteRow(int position);
	void DeleteColumn(int position);
	void Resize(int width, int height);

	Tile* Data();
private:
	std::vector<uint8_t> Compress() const;
	void Uncompress(const std::vector<uint8_t>& data);
	std::vector<uint8_t> CompressLZ77() const;
	void UncompressLZ77(const std::vector<uint8_t>& data);
	void UnpackBytes(const std::vector<uint8_t>& data);
	std::vector<uint8_t> PackBytes() const;
	size_t m_width;
	size_t m_height;
	size_t m_left;
	size_t m_top;
	size_t m_base;
	Compression m_compression;
	std::vector<Tile> m_tiles;
};

#endif // _TILEMAP_2D_RLE_
