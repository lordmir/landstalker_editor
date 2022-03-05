#ifndef _TILESET_MANAGER_H_
#define _TILESET_MANAGER_H_

#include <map>
#include <vector>
#include <list>
#include <memory>
#include <cstdint>
#include <string>

#include <Tileset.h>
#include <Rom.h>

class TilesetManager
{
public:
	TilesetManager(Rom& rom);
	TilesetManager(const std::string& pointer_asm_file, const std::string& tileset_dir);

	std::shared_ptr<Tileset> GetTileset(std::size_t index);
	std::shared_ptr<Tileset> GetTileset(uint32_t address);
	std::shared_ptr<Tileset> GetTileset(const std::string name);

	std::size_t GetTilesetIndex(std::shared_ptr<Tileset> ts);
	std::string GetTilesetName(std::shared_ptr<Tileset> ts);
	std::string GetTilesetName(std::size_t index);
	std::size_t GetTilesetCount();

	bool MakeNewTileset(const std::string& name);
	void SetTilesetAtIndex(std::size_t index, const std::string& name = std::string());
	void AssociatePaletteWithTileset(std::size_t index, const std::string& name);

	void MarkDirty(std::shared_ptr<Tileset>);
	bool SaveChanges();
	bool SaveChanges(const std::string& pointer_asm_file, const std::string& tileset_dir);
	bool InjectIntoRom();
	bool InjectIntoRom(Rom& rom);

private:
	std::shared_ptr<Rom> rom;
	uint32_t m_ptbladdr;
	uint32_t m_ts_start;
	uint32_t m_ts_end;

	static const std::size_t MAX_TILESETS = 32;

	std::string m_pointerfile;
	std::string m_tilesetdir;

	std::array<std::string, MAX_TILESETS> m_tlist;
	std::map<std::string, std::shared_ptr<Tileset>> m_tilesets;
	std::map<std::shared_ptr<Tileset>, std::string> m_names;
	std::map<std::shared_ptr<Tileset>, uint32_t> m_addrs;
	std::map<std::shared_ptr<Tileset>, std::size_t> m_sizes;
	std::map<std::shared_ptr<Tileset>, bool> m_dirty;
	std::map<std::shared_ptr<Tileset>, std::vector<uint8_t>> m_orig;
	std::unordered_multimap<std::shared_ptr<Tileset>, std::string> m_pals;
};

#endif // _TILESET_MANAGER_H_