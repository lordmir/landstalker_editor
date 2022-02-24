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

    Tilemap3D()
    : foreground(),
      background(),
      heightmap(),
      left(0), top(0), width(0), height(0), tile_width(0), tile_height(0), hmwidth(0), hmheight(0)
    {
    }

    Tilemap3D(const uint8_t* src)
        : tile_width(0), tile_height(0)
    {
        Decode(src);
    }

    uint16_t Decode(const uint8_t* src);
    uint16_t Encode(uint8_t* dst, size_t size);

    uint8_t GetLeft() const;
    uint8_t GetTop() const;
    uint8_t GetWidth() const;
    uint8_t GetHeight() const;
    uint16_t GetSize() const;
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
    PixelPoint2D IsoToPixel(const IsoPoint2D& iso, Layer layer = Layer::FG) const;
    PixelPoint2D ToPixel(const Point2D& iso, Layer layer = Layer::FG) const;
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
