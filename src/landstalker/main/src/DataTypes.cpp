#include <landstalker/main/include/DataTypes.h>

std::shared_ptr<TilesetEntry> TilesetEntry::Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, bool compressed, std::size_t width, std::size_t height, uint8_t bit_depth, Tileset::BlockType blocktype)
{
	auto o = std::make_shared<TilesetEntry>(owner, b, name, filename, compressed, width, height, bit_depth, blocktype);
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
	out = std::make_shared<Tileset>();
	out->SetParams(m_width, m_height, m_bit_depth, m_blocktype);
	in->resize(out->SetBits(*in, m_compressed));
	return true;
}

std::shared_ptr<PaletteEntry> PaletteEntry::Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, Palette::Type type)
{
	auto o = std::make_shared<PaletteEntry>(owner, b, name, filename, type);
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
	out = std::make_shared<Palette>(GetName(), *in, m_type);
	return true;
}

std::shared_ptr<BlocksetEntry> BlocksetEntry::Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename)
{
	auto o = std::make_shared<BlocksetEntry>(owner, b, name, filename);
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

std::shared_ptr<Tilemap3DEntry> Tilemap3DEntry::Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename)
{
	auto o = std::make_shared<Tilemap3DEntry>(owner, b, name, filename);
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

std::shared_ptr<AnimatedTilesetEntry> AnimatedTilesetEntry::Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames, uint8_t base_tileset)
{
	auto o = std::make_shared<AnimatedTilesetEntry>(owner, b, name, filename, base, length, speed, frames, base_tileset);
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

std::shared_ptr<Tilemap2DEntry> Tilemap2DEntry::Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, Tilemap2D::Compression compression, uint16_t tile_base, uint16_t width, uint16_t height)
{
	auto o = std::make_shared<Tilemap2DEntry>(owner, b, name, filename, compression, tile_base, width, height);
	o->Initialise();
	return o;
}

bool Tilemap2DEntry::Serialise(const std::shared_ptr<Tilemap2D> in, ByteVectorPtr out)
{
	out->clear();
	in->GetBits(*out, in->GetCompression());
	return true;
}

bool Tilemap2DEntry::Deserialise(const ByteVectorPtr in, std::shared_ptr<Tilemap2D>& out)
{
	out = std::make_shared<Tilemap2D>(width, height);
	in->resize(out->Open(*in, compression, base));
	return true;
}

std::shared_ptr<SpriteFrameEntry> SpriteFrameEntry::Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename)
{
	auto o = std::make_shared<SpriteFrameEntry>(owner, b, name, filename);
	o->Initialise();
	return o;
}

std::shared_ptr<SpriteFrameEntry> SpriteFrameEntry::Create(DataManager* owner, const std::string& name, const std::filesystem::path& filename)
{
	return std::make_shared<SpriteFrameEntry>(owner, name, filename);
}

bool SpriteFrameEntry::Serialise(const std::shared_ptr<SpriteFrame> in, ByteVectorPtr out)
{
	*out = in->GetBits();
	return true;
}

bool SpriteFrameEntry::Deserialise(const ByteVectorPtr in, std::shared_ptr<SpriteFrame>& out)
{
	out = std::make_shared<SpriteFrame>();
	in->resize(out->SetBits(*in));
	return true;
}
