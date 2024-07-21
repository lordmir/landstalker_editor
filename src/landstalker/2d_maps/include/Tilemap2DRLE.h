#ifndef _TILEMAP_2D_RLE_
#define _TILEMAP_2D_RLE_

#include <vector>
#include <cstdint>
#include <cstdlib>
#include <landstalker/tileset/include/Tile.h>

class Tilemap2D
{
public:
	enum class Compression
	{
		NONE,
		RLE,
		LZ77
	};

	Tilemap2D();
	Tilemap2D(size_t width, size_t height, size_t base = 0);
	Tilemap2D(const std::string& filename, Compression compression = Compression::RLE, size_t base = 0);
	Tilemap2D(const std::string& filename, size_t width, size_t height, Compression compression = Compression::NONE, size_t base = 0);
	Tilemap2D(const std::vector<uint8_t>& data, Compression compression = Compression::RLE, size_t base = 0);
	Tilemap2D(const std::vector<uint8_t>& data, size_t width, size_t height, Compression compression = Compression::NONE, size_t base = 0);

	bool operator==(const Tilemap2D& rhs) const;
	bool operator!=(const Tilemap2D& rhs) const;

	bool Open(const std::string& filename, Compression compression = Compression::RLE, size_t base = 0);
	bool Open(const std::string& filename, size_t width, size_t height, Compression compression = Compression::NONE, size_t base = 0);
	uint32_t Open(const std::vector<uint8_t>& data, Compression compression = Compression::RLE, size_t base = 0);
	uint32_t Open(const std::vector<uint8_t>& data, size_t width, size_t height, Compression compression = Compression::NONE, size_t base = 0);

	bool GetBits(std::vector<uint8_t>& data, Compression compression = Compression::NONE);
	bool Save(const std::string& filename, Compression compression = Compression::NONE);
	Compression GetCompression() const;
	void SetCompression(Compression c);
	static std::string GetFileExtension(Compression c);
	std::string GetFileExtension() const;
	static Compression FromFileExtension(const std::string& filename);

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
	void SetBase(uint16_t base);

	Tile GetTile(size_t x, size_t y) const;
	void SetTile(const Tile& tile, size_t x, size_t y);
	bool IsTileValid(int x, int y) const;

	void InsertRow(int position, const Tile& fill = 0);
	void InsertColumn(int position, const Tile& fill = 0);
	void DeleteRow(int position);
	void DeleteColumn(int position);
	void Resize(int width, int height);

	Tile* Data();
private:
	std::vector<uint8_t> Compress() const;
	uint32_t Uncompress(const std::vector<uint8_t>& data);
	std::vector<uint8_t> CompressLZ77() const;
	uint32_t UncompressLZ77(const std::vector<uint8_t>& data);
	uint32_t UnpackBytes(const std::vector<uint8_t>& data);
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
