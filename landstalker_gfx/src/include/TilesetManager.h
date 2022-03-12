#ifndef _TILESET_MANAGER_H_
#define _TILESET_MANAGER_H_

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <boost/filesystem.hpp>

#include <AsmFile.h>
#include <AnimatedTileset.h>
#include <Rom.h>

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
		boost::filesystem::path filename;
		std::string ptrname;
		std::shared_ptr<std::vector<uint8_t>> raw_data;
		std::shared_ptr<std::vector<uint8_t>> decompressed_data;
	};

	TilesetManager(const boost::filesystem::path& asm_file);
	TilesetManager(const Rom& rom);
	
	bool CheckDataWillFitInRom(const Rom& rom, int& tilesets_size, int& anim_table_size) const;
	bool HasTilesetBeenModified(const std::string& tileset) const;
	bool InjectIntoRom(Rom& rom);
	bool Save(boost::filesystem::path dir);
	bool Save();
	std::shared_ptr<Tileset> GetTileset(const std::string& name);

	std::shared_ptr<const TilesetEntry> GetTilesetByName(const std::string& name) const;
	std::shared_ptr<TilesetEntry> GetTilesetByName(const std::string& name);
	std::shared_ptr<const TilesetEntry> GetTilesetByPtr(std::shared_ptr<Tileset> ts) const;
	std::shared_ptr<TilesetEntry> GetTilesetByPtr(std::shared_ptr<Tileset> ts);
	std::vector<std::string> GetTilesetList() const;
	std::vector<std::string> GetTilesetList(Type type) const;
	bool RenameTileset(const std::string& origname, const std::string& newname);
	bool DeleteTileset(const std::string& name);
	std::shared_ptr<TilesetEntry> MakeNewTileset(const std::string& name, Type type);

	std::array<std::string, 32> GetTilesetOrder() const;
	void SetTilesetOrder(const std::array<std::string, 32>& order);
private:
	bool GetTilesetAsmFilenames();
	bool LoadAsmAnimatedTilesetData();
	bool LoadAsmTilesetPointerData();
	bool LoadAsmTilesetFilenames();
	void LoadAsmTilesetData();
	bool LoadRomAnimatedTilesetData(const Rom& rom);
	bool LoadRomTilesetPointerData(const Rom& rom);
	void LoadRomTilesetData(const Rom& rom);
	
	bool UpdateTilesetCache(const std::string& tileset);
	std::vector<uint8_t> GetTilesetBits(const std::string& tileset) const;

	void SaveTilesetsToDisk(const boost::filesystem::path& dir);
	void SaveTilesetsToRom(Rom& rom);
	void SaveTilesetToFile(const boost::filesystem::path& dir, const std::string& name) const;
	bool SaveAsmTilesetFilenames(const boost::filesystem::path& dir);
	bool SaveAsmAnimatedTilesetData(const boost::filesystem::path& dir);
	bool SaveRomAnimatedTilesetData(Rom& rom);
	bool SaveAsmTilesetPointerData(const boost::filesystem::path& dir);
	bool SaveRomTilesetPointerData(Rom& rom);

	boost::filesystem::path m_asm_filename;
	boost::filesystem::path m_base_path;
	boost::filesystem::path m_tileset_data_filename;
	boost::filesystem::path m_tileset_ptrtab_filename;
	boost::filesystem::path m_tileset_anim_filename;

	std::map<std::string, std::shared_ptr<TilesetEntry>> m_tilesets_by_name;
	std::map<std::shared_ptr<Tileset>, std::shared_ptr<TilesetEntry>> m_tilesets_by_ptr;
	std::array<std::shared_ptr<TilesetEntry>, 32> m_tileset_list_x;
	std::vector<std::shared_ptr<TilesetEntry>> m_animated_ts_ptrorder;
	std::vector<std::shared_ptr<TilesetEntry>> m_tilesets_listorder;
	mutable std::map<std::shared_ptr<TilesetEntry>, std::vector<uint8_t>> m_pending_write;
};

#endif // _TILESET_MANAGER_H_