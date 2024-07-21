#ifndef _ANIMATED_TILESET_H_
#define _ANIMATED_TILESET_H_

#include <landstalker/tileset/include/Tileset.h>

class AnimatedTileset : public Tileset
{
public:
    AnimatedTileset(uint16_t base, uint16_t length, uint8_t speed, uint8_t frames);
    AnimatedTileset(const std::vector<uint8_t>& bytes, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames);
    AnimatedTileset(const std::string& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames);
    AnimatedTileset();

    bool operator==(const AnimatedTileset& rhs) const;
    bool operator!=(const AnimatedTileset& rhs) const;

    std::vector<uint8_t> GetTile(const Tile& tile, uint8_t frame) const;
    std::vector<uint8_t>& GetTilePixels(int tile_index, uint8_t frame);

    uint16_t GetBaseBytes() const;
    Tile GetStartTile() const;
    uint16_t GetFrameSizeBytes() const;
    std::size_t GetFrameSizeTiles() const;
    uint8_t GetAnimationSpeed() const;
    uint8_t GetAnimationFrames() const;
    uint8_t GetBaseTileset() const;

    void SetBaseBytes(uint16_t base);
    void SetStartTile(Tile tile);
    void SetFrameSizeBytes(uint16_t bytes);
    void SetFrameSizeTiles(std::size_t count);
    void SetAnimationSpeed(uint8_t speed);
    void SetAnimationFrames(uint8_t frames);
    void SetBaseTileset(uint8_t base_tileset);

private:
    uint16_t m_base;
    uint16_t m_length;
    uint8_t m_speed;
    uint8_t m_frames;
    uint8_t m_base_tileset;
};

#endif // _ANIMATED_TILESET_H_