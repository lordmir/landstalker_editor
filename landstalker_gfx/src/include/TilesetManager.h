#ifndef _TILESET_MANAGER_H_
#define _TILESET_MANAGER_H_

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include "wjakob/filesystem/path.h"

#include <AsmFile.h>
#include <AnimatedTileset.h>
#include <Rom.h>
#include <BlocksetCmp.h>

class TilesetManager
{
public:
	enum class Type
	{
		MAP,
		ANIMATED_MAP,
		FONT,
		MISC
	};

	struct TilesetEntry
	{
		TilesetEntry(const std::string& pname, std::shared_ptr<Tileset> ptileset, Type ptype)
			: name(pname), tileset(ptileset), type(ptype) {}
		std::string name;
		std::shared_ptr<Tileset> tileset;
		Type type;
		uint32_t start_address;
		uint32_t end_address;
		filesystem::path filename;
		std::string ptrname;
		std::shared_ptr<std::vector<uint8_t>> raw_data;
		std::shared_ptr<std::vector<uint8_t>> decompressed_data;
	};

	struct BlocksetEntry
	{
		std::string name;
		uint8_t tileset;
		uint8_t primary_idx;
		uint8_t secondary_idx;
		std::shared_ptr<Blockset> blockset;
		std::shared_ptr<Blockset> blockset_orig;
		uint32_t start_address;
		uint32_t end_address;
		filesystem::path filename;
		std::shared_ptr<std::vector<uint8_t>> raw_data;

		uint8_t GetPrimary() const { return (primary_idx << 5) | tileset; }
		uint8_t GetSecondary() const { return secondary_idx; }
	};

	TilesetManager(const filesystem::path& asm_file);
	TilesetManager(const Rom& rom);
	
	bool CheckDataWillFitInRom(const Rom& rom, int& tilesets_size, int& anim_table_size) const;
	bool HasTilesetBeenModified(const std::string& tileset) const;
	bool HasBeenModified() const;
	bool InjectIntoRom(Rom& rom);
	bool Save(filesystem::path dir);
	bool Save();
	std::shared_ptr<Tileset> GetTileset(const std::string& name);

	std::shared_ptr<const TilesetEntry> GetTilesetByName(const std::string& name) const;
	std::shared_ptr<TilesetEntry> GetTilesetByName(const std::string& name);
	std::shared_ptr<const TilesetEntry> GetTilesetByPtr(std::shared_ptr<Tileset> ts) const;
	std::shared_ptr<TilesetEntry> GetTilesetByPtr(std::shared_ptr<Tileset> ts);
	std::shared_ptr<const TilesetEntry> GetTilesetByIdx(uint8_t ts) const;
	std::shared_ptr<TilesetEntry> GetTilesetByIdx(uint8_t ts);
	std::vector<std::string> GetTilesetList() const;
	std::vector<std::string> GetTilesetList(Type type) const;
	std::vector<std::pair<uint8_t, std::string>> GetPriBlocksetList() const;
	std::vector<std::string> GetSecBlocksetList(uint8_t pri) const;
	std::shared_ptr<BlocksetEntry> GetBlockset(const std::string& name) const;
	std::shared_ptr<BlocksetEntry> GetBlockset(uint8_t pri, uint8_t sec) const;
	bool RenameTileset(const std::string& origname, const std::string& newname);
	bool DeleteTileset(const std::string& name);
	std::shared_ptr<TilesetEntry> MakeNewTileset(const std::string& name, Type type);

	std::array<std::string, 32> GetTilesetOrder() const;
	void SetTilesetOrder(const std::array<std::string, 32>& order);

	std::string GetTilesetSavedPalette(const std::shared_ptr<TilesetEntry> ts) const;
	void SetTilesetSavedPalette(std::shared_ptr<TilesetEntry> ts, const std::string& palette_name);
private:
	bool GetTilesetAsmFilenames();
	bool LoadAsmAnimatedTilesetData();
	bool LoadAsmTilesetPointerData();
	bool LoadAsmTilesetFilenames();
	void LoadAsmTilesetData();
	bool LoadAsmBlocksetPointerData();
	bool LoadAsmBlocksetData();
	bool LoadRomAnimatedTilesetData(const Rom& rom);
	bool LoadRomTilesetPointerData(const Rom& rom);
	void LoadRomTilesetData(const Rom& rom);
	void LoadRomBlocksetData(const Rom& rom);
	void LoadRomBlockset(const Rom& rom, uint8_t pri, uint8_t sec, uint32_t begin, uint32_t end);
	
	bool UpdateTilesetCache(const std::string& tileset);
	std::vector<uint8_t> GetTilesetBits(const std::string& tileset) const;

	void SaveTilesetsToDisk(const filesystem::path& dir);
	void SaveTilesetToFile(const filesystem::path& dir, const std::string& name) const;
	bool SaveAsmTilesetFilenames(const filesystem::path& dir);
	bool SaveAsmAnimatedTilesetData(const filesystem::path& dir);
	bool SaveAsmTilesetPointerData(const filesystem::path& dir);
	bool SaveAsmBlocksetPointerData(const filesystem::path& dir);
	bool SaveAsmBlocksetData(const filesystem::path& dir);

	void SaveTilesetsToRom(Rom& rom);
	bool SaveRomAnimatedTilesetData(Rom& rom);
	bool SaveRomTilesetPointerData(Rom& rom);

	filesystem::path m_asm_filename;
	filesystem::path m_base_path;
	filesystem::path m_tileset_data_filename;
	filesystem::path m_tileset_ptrtab_filename;
	filesystem::path m_tileset_anim_filename;
	filesystem::path m_blockset_pri_ptr_filename;
	filesystem::path m_blockset_sec_ptr_filename;
	filesystem::path m_blockset_data_filename;

	std::map<std::string, std::shared_ptr<TilesetEntry>> m_tilesets_by_name;
	std::map<std::shared_ptr<Tileset>, std::shared_ptr<TilesetEntry>> m_tilesets_by_ptr;
	std::array<std::shared_ptr<TilesetEntry>, 32> m_tileset_list_x;
	std::vector<std::shared_ptr<TilesetEntry>> m_animated_ts_ptrorder;
	std::vector<std::shared_ptr<TilesetEntry>> m_tilesets_listorder;
	std::unordered_map<std::shared_ptr<TilesetEntry>, std::string> m_tilesets_defaultpal;
	std::map<std::string, uint8_t> m_blockset_pri_ptrs;
	std::map<std::string, std::pair<uint8_t, uint8_t>> m_blockset_sec_ptrs;
	std::map<std::string, std::shared_ptr<BlocksetEntry>> m_blocksets_by_ptr;
	std::map<std::pair<uint8_t, uint8_t>, std::shared_ptr<BlocksetEntry>> m_blocksets;
	mutable std::map<std::shared_ptr<TilesetEntry>, std::vector<uint8_t>> m_pending_write;
};

#endif // _TILESET_MANAGER_H_