#ifndef _DATA_TYPES_H_
#define _DATA_TYPES_H_

#include <landstalker/main/include/DataManager.h>
#include <landstalker/tileset/include/Tileset.h>
#include <landstalker/palettes/include/Palette.h>
#include <landstalker/blockset/include/BlocksetCmp.h>
#include <landstalker/3d_maps/include/Tilemap3DCmp.h>
#include <landstalker/2d_maps/include/Tilemap2DRLE.h>
#include <landstalker/tileset/include/AnimatedTileset.h>
#include <landstalker/sprites/include/SpriteFrame.h>
#include <landstalker/main/include/EntryPreferences.h>

class TilesetEntry : public DataManager::Entry<Tileset>, public PalettePreferences
{
public:
	TilesetEntry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, bool compressed = true, std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, Tileset::BlockType blocktype = Tileset::BlockType::NORMAL)
		: Entry(owner, b, name, filename),
		m_compressed(compressed),
		m_width(width),
		m_height(height),
		m_bit_depth(bit_depth),
		m_blocktype(blocktype),
		m_index(-1)
	{}

	static std::shared_ptr<TilesetEntry> Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, bool compressed = true, std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, Tileset::BlockType blocktype = Tileset::BlockType::NORMAL);

	virtual bool Serialise(const std::shared_ptr<Tileset> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Tileset>& out);

	const std::string& GetPointerName() const { return m_ptrname; }
	void SetPointerName(const std::string& name) { m_ptrname = name; }

	int GetIndex() const { return m_index; }
	void SetIndex(int index) { m_index = index; }

	std::string GetPaletteIndicies() const { return m_palindicies; }
	void SetPalIndicies(const std::string& value) { m_palindicies = value; }
private:
	bool m_compressed;
	std::size_t m_width;
	std::size_t m_height;
	uint8_t m_bit_depth;
	Tileset::BlockType m_blocktype;
	std::string m_ptrname;
	std::string m_palindicies;
	int m_index;
};

class AnimatedTilesetEntry : public DataManager::Entry<AnimatedTileset>, public PalettePreferences
{
public:
	AnimatedTilesetEntry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames, uint8_t base_tileset)
		: Entry(owner, b, name, filename),
		  m_base(base),
		  m_length(length),
		  m_speed(speed),
		  m_frames(frames),
		  m_base_tileset(base_tileset),
		  m_index(std::make_pair<uint8_t, uint8_t>(0,0))
	{}

	static std::shared_ptr<AnimatedTilesetEntry> Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames, uint8_t base_tileset);

	virtual bool Serialise(const std::shared_ptr<AnimatedTileset> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<AnimatedTileset>& out);

	const std::string& GetPointerName() const { return m_ptrname; }
	void SetPointerName(const std::string& name) { m_ptrname = name; }

	std::pair<uint8_t, uint8_t> GetIndex() const { return m_index; }
	void SetIndex(uint8_t src_idx, uint8_t anim_idx) { m_index = { src_idx, anim_idx }; }
private:
	uint16_t m_base;
	uint16_t m_length;
	uint8_t m_speed;
	uint8_t m_frames;
	uint8_t m_base_tileset;
	std::string m_ptrname;
	std::pair<uint8_t, uint8_t> m_index;
};

class PaletteEntry : public DataManager::Entry<Palette>
{
public:
	PaletteEntry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, Palette::Type type)
		: Entry(owner, b, name, filename),
		m_type(type),
		m_index(-1)
	{}

	static std::shared_ptr<PaletteEntry> Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, Palette::Type type);

	virtual bool Serialise(const std::shared_ptr<Palette> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Palette>& out);

	int GetIndex() const { return m_index; }
	void SetIndex(int val) { m_index = val; }
private:
	Palette::Type m_type;
	int m_index;
};

class BlocksetEntry : public DataManager::Entry<Blockset>, public PalettePreferences
{
public:
	BlocksetEntry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename)
		: Entry(owner, b, name, filename), pri(0), sec(0)
	{}

	static std::shared_ptr<BlocksetEntry> Create(DataManager* owner, const ByteVector & b, const std::string & name, const std::filesystem::path & filename);

	virtual bool Serialise(const std::shared_ptr<Blockset> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Blockset>& out);

	std::pair<uint8_t, uint8_t> GetIndex() const { return { pri, sec }; }
	void SetIndex(std::pair<uint8_t, uint8_t> idx) { pri = idx.first; sec = idx.second; }
	uint8_t GetPrimary() const { return pri >> 5; }
	uint8_t GetSecondary() const { return sec; }
	uint8_t GetTileset() const { return pri & 0x1F; }
private:
	uint8_t pri;
	uint8_t sec;
};

class Tilemap3DEntry : public DataManager::Entry<Tilemap3D>, public PalettePreferences
{
public:
	Tilemap3DEntry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename)
		: Entry(owner, b, name, filename)
	{}

	static std::shared_ptr<Tilemap3DEntry> Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename);

	virtual bool Serialise(const std::shared_ptr<Tilemap3D> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Tilemap3D>& out);
};

class Tilemap2DEntry : public DataManager::Entry<Tilemap2D>, public PalettePreferences
{
public:
	Tilemap2DEntry(DataManager * owner, const ByteVector & b, const std::string & name, const std::filesystem::path & filename, Tilemap2D::Compression map_compression, uint16_t tile_base, uint16_t width = 0, uint16_t height = 0)
		: Entry(owner, b, name, filename),
		compression(map_compression),
		base(tile_base),
		width(width),
		height(height)
	{}

	static std::shared_ptr<Tilemap2DEntry> Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename, Tilemap2D::Compression compression, uint16_t tile_base, uint16_t width = 0, uint16_t height = 0);

	virtual bool Serialise(const std::shared_ptr<Tilemap2D> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Tilemap2D>& out);

	std::string GetTileset() const { return tileset; }
	void SetTileset(const std::string& name) { tileset = name; }
	Tilemap2D::Compression GetCompression() const { return compression; }
	void SetCompression(Tilemap2D::Compression value) { compression = value; }
	uint16_t GetBase() const { return base; }
	void SetBase(uint16_t value) { base = value; GetData()->SetBase(value); }
private:
	std::string tileset;
	Tilemap2D::Compression compression;
	uint16_t base;
	uint16_t width;
	uint16_t height;
};

class SpriteFrameEntry : public DataManager::Entry<SpriteFrame>, public PalettePreferences
{
public:
	SpriteFrameEntry(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename)
		: Entry(owner, b, name, filename),
		  m_sprite(0)
	{}
	SpriteFrameEntry(DataManager * owner, const std::string & name, const std::filesystem::path & filename)
		: Entry(owner, name, filename),
		  m_sprite(0)
	{}

	static std::shared_ptr<SpriteFrameEntry> Create(DataManager* owner, const ByteVector& b, const std::string& name, const std::filesystem::path& filename);
	static std::shared_ptr<SpriteFrameEntry> Create(DataManager* owner, const std::string& name, const std::filesystem::path& filename);

	virtual bool Serialise(const std::shared_ptr<SpriteFrame> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<SpriteFrame>& out);

	uint8_t GetSprite() const { return m_sprite; }
	void SetSprite(uint8_t val) { m_sprite = val; }
private:
	uint8_t m_sprite;
};

#endif // _DATA_TYPES_H_
