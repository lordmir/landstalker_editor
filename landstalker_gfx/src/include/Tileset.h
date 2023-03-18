#ifndef TILESET_H
#define TILESET_H

#include <memory>
#include <cstdint>
#include <vector>
#include <array>
#include "TileAttributes.h"
#include "PaletteO.h"
#include "Tile.h"
    
class Tileset
{
public:
    enum BlockType
    {
        NORMAL,
        BLOCK1X2,
        BLOCK2X1,
        BLOCK2X2,
        BLOCK3X3,
        BLOCK4X4,
        BLOCK4X6
    };
	inline static const std::array<std::string, 7> BLOCKTYPE_STRINGS = { "Normal", "Block 1x2", "Block 2x1", "Block 2x2", "Block 3x3", "Block 4x4", "Block 4x6" };

    Tileset(std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, BlockType blocktype = NORMAL);
    Tileset(const std::string& filename, bool compressed = false, std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, BlockType blocktype = NORMAL);
    Tileset(const std::vector<uint8_t>& src, bool compressed = false, std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, BlockType blocktype = NORMAL);
    ~Tileset();

    bool operator==(const Tileset& rhs) const;
    bool operator!=(const Tileset& rhs) const;
    
    void SetBits(const std::vector<uint8_t>& src, bool compressed = false);
    void SetParams(std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, BlockType blocktype = NORMAL);
    bool Open(const std::string& filename, bool compressed = false, std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, BlockType blocktype = NORMAL);
    std::vector<uint8_t> GetBits(bool compressed = false);
    bool Save(const std::string& filename, bool compressed = false);
    void Clear();
    std::vector<uint8_t> GetTile(const Tile& tile) const;
    std::vector<uint8_t>& GetTilePixels(int tile_index);
    std::vector<uint8_t> GetTileRGB(const Tile& tile, const PaletteO& palette) const;
    std::vector<uint8_t> GetTileA(const Tile& tile, const PaletteO& palette) const;
    std::vector<uint32_t> GetTileRGBA(const Tile& tile, const PaletteO& palette) const;
    void SetColourIndicies(const std::vector<uint8_t>& colour_indicies);
    std::vector<uint8_t> GetColourIndicies() const;
    std::vector<uint8_t> GetDefaultColourIndicies() const;
    std::array<bool, 16> GetLockedColours() const;

	std::size_t GetTileCount() const;
    std::size_t GetTileSizeBytes() const;
    std::size_t GetTilesetUncompressedSizeBytes() const;
    std::size_t GetTileWidth() const;
    std::size_t GetTileHeight() const;
    std::size_t GetTileBitDepth() const;
    BlockType   GetTileBlockType() const;
    bool        GetCompressed() const;

    void DeleteTile(int tile_number);
    void InsertTilesBefore(int tile_number, int count = 1);
    void DuplicateTile(const Tile& src, const Tile& dst);
    void SwapTile(const Tile& lhs, const Tile& rhs);
    void SetTile(const Tile& src, const std::vector<uint8_t>& value);

private:
    void TransposeBlock();
    void UntransposeBlock(std::vector<uint8_t>& bits);

    std::size_t m_width;
    std::size_t m_height;
    std::size_t m_bit_depth;
    std::size_t m_tilewidth;
    std::size_t m_tileheight;
    bool m_compressed;
    
    BlockType m_blocktype;
    std::vector<std::vector<uint8_t>> m_tiles;
    std::vector<uint8_t> m_colour_indicies;
};

#endif // TILESET_H
