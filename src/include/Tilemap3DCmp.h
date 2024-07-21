#ifndef TILEMAP3DCMP_H
#define TILEMAP3DCMP_H

#include <cstdint>
#include <cstdlib>
#include <vector>

struct Point2D
{
    int x;
    int y;
    Point2D(int xx, int yy) : x(xx), y(yy) {}
};

struct Point3D
{
    int x;
    int y;
    int z;
    Point3D(int xx, int yy, int zz) : x(xx), y(yy), z(zz) {}
};

typedef Point2D IsoPoint2D;
typedef Point2D PixelPoint2D;
typedef Point2D HMPoint2D;

struct BlockLoc
{
    uint16_t value;
    IsoPoint2D position;
};

class Tilemap3D
{
public:
    enum class Layer
    {
        BG,
        FG
    };
    enum class FloorType : uint8_t
    {
        NORMAL = 0,
        DOOR_NE = 1,
        DOOR_SE = 2,
        DOOR_SW = 3,
        DOOR_NW = 4,
        STAIRS = 5,
        DOOR_WARP = 6,
        PIT = 7,
        WARP = 8,
        LADDER_NW = 11,
        LADDER_NE = 12,
        COUNTER = 14,
        ELEVATOR = 15,
        SPIKES = 16,
        SIGN_NW1 = 17,
        SIGN_NW2 = 18,
        SIGN_NW3 = 19,
        SIGN_NW4 = 20,
        SIGN_NE1 = 21,
        SIGN_NE2 = 22,
        SIGN_NE3 = 23,
        SIGN_NE4 = 24,
        SWAMP = 25,
        LOCKED_DOOR_N = 26,
        SIGN_NW5 = 30,
        SIGN_NW6 = 31,
        SIGN_NW7 = 32,
        SIGN_NW8 = 33,
        SIGN_NE5 = 34,
        SIGN_NE6 = 35,
        SIGN_NE7 = 36,
        SIGN_NE8 = 37,
        LOCKED_DOOR_SE = 38,
        LOCKED_DOOR_SW = 39,
        NOLE_STAIRCASE = 40,
        LAVA = 41,
        ICE_NE = 42,
        ICE_SE = 43,
        ICE_SW = 44,
        ICE_NW = 45,
        HEALTH_RECOVER = 46
    };

    Tilemap3D()
    : foreground(),
      background(),
      heightmap(),
      hmwidth(0), hmheight(0), left(0), top(0), width(0), height(0), tile_width(8), tile_height(8)
    {
    }

    Tilemap3D(const uint8_t* src)
        : tile_width(8), tile_height(8)
    {
        Decode(src);
    }

    bool operator==(const Tilemap3D& rhs) const;
    bool operator!=(const Tilemap3D& rhs) const;

    uint16_t Decode(const uint8_t* src);
    uint16_t Encode(uint8_t* dst, size_t size);

    uint8_t GetLeft() const;
    uint8_t GetTop() const;
    uint8_t GetWidth() const;
    uint8_t GetHeight() const;
    uint16_t GetSize() const;
    void Resize(uint8_t w, uint8_t h);
    void ResizeHeightmap(uint8_t w, uint8_t h);
    void InsertHeightmapRow(uint8_t before);
    void InsertHeightmapColumn(uint8_t before);
    void DeleteHeightmapRow(uint8_t row);
    void DeleteHeightmapColumn(uint8_t col);
    void ClearTilemap();
    void InsertTilemapRow(int row);
    void InsertTilemapColumn(int col);
    void DeleteTilemapRow(int row);
    void DeleteTilemapColumn(int col);
    void SetLeft(uint8_t left);
    void SetTop(uint8_t top);
    uint8_t GetHeightmapWidth() const;
    uint8_t GetHeightmapHeight() const;
    uint16_t GetHeightmapSize() const;
    uint8_t GetTileWidth() const;
    uint8_t GetTileHeight() const;
    void SetTileDims(uint8_t tw, uint8_t th);
    std::size_t GetCartesianWidth() const;
    std::size_t GetCartesianHeight() const;
    std::size_t GetPixelWidth() const;
    std::size_t GetPixelHeight() const;
    bool IsIsoPointValid(const IsoPoint2D& iso) const;
    bool IsPointValid(const Point2D& p, Layer layer = Layer::FG) const;
    bool IsPixelPointValid(const PixelPoint2D& pix, Layer layer = Layer::FG) const;
    bool IsHMPointValid(const HMPoint2D& p) const;
    Point2D IsoToCartesian(const IsoPoint2D& iso, Layer layer = Layer::FG) const;
    Point2D PixelToCartesian(const PixelPoint2D& pix, Layer layer = Layer::FG) const;
    IsoPoint2D ToIsometric(const Point2D& p, Layer layer = Layer::FG) const;
    IsoPoint2D PixelToIsometric(const PixelPoint2D& pix, Layer layer = Layer::FG) const;
    PixelPoint2D IsoToPixel(const IsoPoint2D& iso, Layer layer = Layer::FG, bool offset = true) const;
    PixelPoint2D ToPixel(const Point2D& iso, Layer layer = Layer::FG) const;
    PixelPoint2D EntityPositionToPixel(uint16_t x, uint16_t y, uint16_t z) const;
    PixelPoint2D Iso3DToPixel(const Point3D& iso) const;
    Point3D PixelToIso3D(const PixelPoint2D& p) const;
    PixelPoint2D HMPointToPixel(const HMPoint2D& p) const;
    HMPoint2D PixelToHMPoint(const PixelPoint2D& p) const;
    bool IsBlockValid(uint16_t block) const;
    bool IsBlockValid(const IsoPoint2D& iso) const;
    BlockLoc GetBlock(uint16_t block, Layer layer = Layer::FG) const;
    uint16_t GetBlock(const IsoPoint2D& iso, Layer layer = Layer::FG) const;
    bool SetBlock(uint16_t block_value, uint16_t block_index, Layer layer = Layer::FG);
    bool SetBlock(const BlockLoc& loc, Layer layer = Layer::FG);
    uint8_t GetHeight(const HMPoint2D& iso) const;
    bool SetHeight(const HMPoint2D& iso, uint8_t height);
    uint8_t GetCellProps(const HMPoint2D& iso) const;
    bool SetCellProps(const HMPoint2D& iso, uint8_t props);
    uint8_t GetCellType(const HMPoint2D& iso) const;
    bool SetCellType(const HMPoint2D& iso, uint8_t type);
    uint16_t GetHeightmapCell(const HMPoint2D& iso) const;
    bool SetHeightmapCell(const HMPoint2D& iso, uint16_t value);
private:
    std::vector<uint16_t> foreground;
    std::vector<uint16_t> background;
    std::vector<uint16_t> heightmap;
    uint8_t hmwidth;
    uint8_t hmheight;
    uint8_t left;
    uint8_t top;
    uint8_t width;
    uint8_t height;
    uint8_t tile_width;
    uint8_t tile_height;
};

#endif // TILEMAP3DCMP_H
