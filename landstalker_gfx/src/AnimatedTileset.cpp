#include "AnimatedTileset.h"

AnimatedTileset::AnimatedTileset(uint16_t base, uint16_t length, uint8_t speed, uint8_t frames)
	: m_base(base),
	  m_length(length),
	  m_speed(speed),
	  m_frames(frames)
{
}

AnimatedTileset::AnimatedTileset(const std::string& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames)
	: Tileset(filename),
	  m_base(base),
	  m_length(length),
	  m_speed(speed),
	  m_frames(frames)
{
}

std::vector<uint8_t> AnimatedTileset::GetTile(const Tile& tile, uint8_t frame) const
{
	auto t = tile.GetIndex() - GetStartTile().GetIndex();
	auto f_offset = frame * GetFrameSizeTiles();
	return Tileset::GetTile(t + f_offset);
}

std::vector<uint8_t>& AnimatedTileset::GetTilePixels(int tile_index, uint8_t frame)
{
	auto t = tile_index - GetStartTile().GetIndex();
	auto f_offset = frame * GetFrameSizeTiles();
	return Tileset::GetTilePixels(t + f_offset);
}

std::vector<uint8_t> AnimatedTileset::GetTileRGB(const Tile& tile, const Palette& palette, uint8_t frame) const
{
	auto t = tile.GetIndex() - GetStartTile().GetIndex();
	auto f_offset = frame * GetFrameSizeTiles();
	return Tileset::GetTileRGB(t + f_offset, palette);
}

std::vector<uint8_t> AnimatedTileset::GetTileA(const Tile& tile, const Palette& palette, uint8_t frame) const
{
	auto t = tile.GetIndex() - GetStartTile().GetIndex();
	auto f_offset = frame * GetFrameSizeTiles();
	return Tileset::GetTileA(t + f_offset, palette);
}

std::vector<uint32_t> AnimatedTileset::GetTileRGBA(const Tile& tile, const Palette& palette, uint8_t frame) const
{
	auto t = tile.GetIndex() - GetStartTile().GetIndex();
	auto f_offset = frame * GetFrameSizeTiles();
	return Tileset::GetTileRGBA(t + f_offset, palette);
}

uint16_t AnimatedTileset::GetBaseBytes() const
{
	return m_base;
}

Tile AnimatedTileset::GetStartTile() const
{
	return Tile(m_base / Tileset::GetTileSizeBytes());
}

uint16_t AnimatedTileset::GetFrameSizeBytes() const
{
	return m_length;
}

std::size_t AnimatedTileset::GetFrameSizeTiles() const
{
	return m_length / Tileset::GetTileSizeBytes();
}

uint8_t AnimatedTileset::GetAnimationSpeed() const
{
	return m_speed;
}

uint8_t AnimatedTileset::GetAnimationFrames() const
{
	return m_frames;
}

uint8_t AnimatedTileset::GetBaseTileset() const
{
	return m_base_tileset;
}

void AnimatedTileset::SetBaseBytes(uint16_t base)
{
	m_base = base;
}

void AnimatedTileset::SetStartTile(Tile tile)
{
	m_base = tile.GetIndex() * Tileset::GetTileSizeBytes();
}

void AnimatedTileset::SetFrameSizeBytes(uint16_t bytes)
{
	m_length = bytes;
}

void AnimatedTileset::SetFrameSizeTiles(std::size_t count)
{
	m_length = count * Tileset::GetTileSizeBytes();
}

void AnimatedTileset::SetAnimationSpeed(uint8_t speed)
{
	m_speed = speed;
}

void AnimatedTileset::SetAnimationFrames(uint8_t frames)
{
	m_frames = frames;
}

void AnimatedTileset::SetBaseTileset(uint8_t base_tileset)
{
	m_base_tileset = base_tileset;
}
