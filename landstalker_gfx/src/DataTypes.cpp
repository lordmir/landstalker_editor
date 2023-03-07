#include <DataTypes.h>

std::shared_ptr<TilesetEntry> TilesetEntry::Create(const ByteVector& b, const std::string& name, const filesystem::path& filename, bool compressed, std::size_t width, std::size_t height, uint8_t bit_depth, Tileset::BlockType blocktype)
{
	auto o = std::make_shared<TilesetEntry>(b, name, filename, compressed, width, height, bit_depth, blocktype);
	o->Initialise();
	return o;
}

bool TilesetEntry::Serialise(const std::shared_ptr<Tileset> in, ByteVectorPtr out)
{
	*out = in->GetBits(m_compressed);
	return true;
}

bool TilesetEntry::Deserialise(const ByteVectorPtr in, std::shared_ptr<Tileset>& out)
{
	out = std::make_shared<Tileset>(*in, m_compressed, m_width, m_height, m_bit_depth, m_blocktype);
	return true;
}

std::shared_ptr<PaletteEntry> PaletteEntry::Create(const ByteVector& b, const std::string& name, const filesystem::path& filename, Palette::Type type)
{
	auto o = std::make_shared<PaletteEntry>(b, name, filename, type);
	o->Initialise();
	return o;
}

bool PaletteEntry::Serialise(const std::shared_ptr<Palette> in, ByteVectorPtr out)
{
	*out = in->GetBytes();
	return true;
}

bool PaletteEntry::Deserialise(const ByteVectorPtr in, std::shared_ptr<Palette>& out)
{
	out = std::make_shared<Palette>(*in, m_type);
	return true;
}

std::shared_ptr<BlocksetEntry> BlocksetEntry::Create(const ByteVector& b, const std::string& name, const filesystem::path& filename)
{
	auto o = std::make_shared<BlocksetEntry>(b, name, filename);
	o->Initialise();
	return o;
}

bool BlocksetEntry::Serialise(const std::shared_ptr<Blockset> in, ByteVectorPtr out)
{
	out->resize(65536);
	auto len = BlocksetCmp::Encode(*in, &out->at(0), out->size());
	out->resize(len);
	return true;
}

bool BlocksetEntry::Deserialise(const ByteVectorPtr in, std::shared_ptr<Blockset>& out)
{
	out = std::make_shared<Blockset>();
	auto len = BlocksetCmp::Decode(&in->at(0), in->size(), *out);
	in->resize(len);
	return true;
}

std::shared_ptr<Tilemap3DEntry> Tilemap3DEntry::Create(const ByteVector& b, const std::string& name, const filesystem::path& filename)
{
	auto o = std::make_shared<Tilemap3DEntry>(b, name, filename);
	o->Initialise();
	return o;
}

bool Tilemap3DEntry::Serialise(const std::shared_ptr<Tilemap3D> in, ByteVectorPtr out)
{
	out->resize(65536);
	auto len = in->Encode(&out->at(0), out->size());
	out->resize(len);
	return true;
}

bool Tilemap3DEntry::Deserialise(const ByteVectorPtr in, std::shared_ptr<Tilemap3D>& out)
{
	out = std::make_shared<Tilemap3D>();
	uint16_t len = out->Decode(&in->at(0));
	in->resize(len);
	return true;
}

std::shared_ptr<AnimatedTilesetEntry> AnimatedTilesetEntry::Create(const ByteVector& b, const std::string& name, const filesystem::path& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames, uint8_t base_tileset)
{
	auto o = std::make_shared<AnimatedTilesetEntry>(b, name, filename, base, length, speed, frames, base_tileset);
	o->Initialise();
	return o;
}

bool AnimatedTilesetEntry::Serialise(const std::shared_ptr<AnimatedTileset> in, ByteVectorPtr out)
{
	*out = in->GetBits(false);
	m_base = in->GetBaseBytes();
	m_frames = in->GetAnimationFrames();
	m_length = in->GetFrameSizeBytes();
	m_speed = in->GetAnimationSpeed();
	m_base_tileset = in->GetBaseTileset();
	return true;
}

bool AnimatedTilesetEntry::Deserialise(const ByteVectorPtr in, std::shared_ptr<AnimatedTileset>& out)
{
	out = std::make_shared<AnimatedTileset>(*in, m_base, m_length, m_speed, m_frames);
	out->SetBaseTileset(m_base_tileset);
	return true;
}
