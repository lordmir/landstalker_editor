#ifndef _DATA_TYPES_H_
#define _DATA_TYPES_H_

#include <DataManager.h>
#include <Tileset.h>
#include <Palette.h>
#include <BlocksetCmp.h>
#include <Tilemap3DCmp.h>
#include <AnimatedTileset.h>


class TilesetEntry : public DataManager::Entry<Tileset>
{
public:
	TilesetEntry(const ByteVector& b, const std::string& name, const filesystem::path& filename, bool compressed = true, std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, Tileset::BlockType blocktype = Tileset::BlockType::NORMAL)
		: Entry(b, name, filename),
		m_compressed(compressed),
		m_width(width),
		m_height(height),
		m_bit_depth(bit_depth),
		m_blocktype(blocktype),
		m_index(-1)
	{}

	static std::shared_ptr<TilesetEntry> Create(const ByteVector& b, const std::string& name, const filesystem::path& filename, bool compressed = true, std::size_t width = 8, std::size_t height = 8, uint8_t bit_depth = 4, Tileset::BlockType blocktype = Tileset::BlockType::NORMAL);

	virtual bool Serialise(const std::shared_ptr<Tileset> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Tileset>& out);

	const std::string& GetPointerName() const { return m_ptrname; }
	void SetPointerName(const std::string& name) { m_ptrname = name; }

	int GetIndex() const { return m_index; }
	void SetIndex(int index) { m_index = index; }
private:
	bool m_compressed;
	std::size_t m_width;
	std::size_t m_height;
	uint8_t m_bit_depth;
	Tileset::BlockType m_blocktype;
	std::string m_ptrname;
	int m_index;
};

class AnimatedTilesetEntry : public DataManager::Entry<AnimatedTileset>
{
public:
	AnimatedTilesetEntry(const ByteVector& b, const std::string& name, const filesystem::path& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames, uint8_t base_tileset)
		: Entry(b, name, filename),
		  m_base(base),
		  m_length(length),
		  m_speed(speed),
		  m_frames(frames),
		  m_base_tileset(base_tileset),
		  m_index({0,0})
	{}

	static std::shared_ptr<AnimatedTilesetEntry> Create(const ByteVector& b, const std::string& name, const filesystem::path& filename, uint16_t base, uint16_t length, uint8_t speed, uint8_t frames, uint8_t base_tileset);

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
	PaletteEntry(const ByteVector& b, const std::string& name, const filesystem::path& filename, Palette::Type type)
		: Entry(b, name, filename),
		m_type(type)
	{}

	static std::shared_ptr<PaletteEntry> Create(const ByteVector& b, const std::string& name, const filesystem::path& filename, Palette::Type type);

	virtual bool Serialise(const std::shared_ptr<Palette> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Palette>& out);
private:
	Palette::Type m_type;
};

class BlocksetEntry : public DataManager::Entry<Blockset>
{
public:
	BlocksetEntry(const ByteVector& b, const std::string& name, const filesystem::path& filename)
		: Entry(b, name, filename), pri(0), sec(0)
	{}

	static std::shared_ptr<BlocksetEntry> Create(const ByteVector & b, const std::string & name, const filesystem::path & filename);

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

class Tilemap3DEntry : public DataManager::Entry<Tilemap3D>
{
public:
	Tilemap3DEntry(const ByteVector& b, const std::string& name, const filesystem::path& filename)
		: Entry(b, name, filename)
	{}

	static std::shared_ptr<Tilemap3DEntry> Create(const ByteVector& b, const std::string& name, const filesystem::path& filename);

	virtual bool Serialise(const std::shared_ptr<Tilemap3D> in, ByteVectorPtr out);
	virtual bool Deserialise(const ByteVectorPtr in, std::shared_ptr<Tilemap3D>& out);
};

#endif // _DATA_TYPES_H_
