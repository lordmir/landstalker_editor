#ifndef _ROOM_DATA_H_
#define _ROOM_DATA_H_

#include <map>
#include <vector>

#include <DataManager.h>
#include <DataTypes.h>
#include <Room.h>
#include <WarpList.h>

class RoomData : public DataManager
{
public:
    enum class MiscPaletteType
    {
        LAVA,
        WARP,
        LANTERN
    };

    RoomData(const filesystem::path& asm_file);
    RoomData(const Rom& rom);

    virtual ~RoomData() {}

    virtual bool Save(const filesystem::path& dir);
    virtual bool Save();

	virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    std::vector<std::shared_ptr<TilesetEntry>> GetTilesets() const;
    std::vector<std::shared_ptr<AnimatedTilesetEntry>> GetAnimatedTilesets(const std::string& tileset) const;
    bool HasAnimatedTilesets(const std::string& tileset) const;
    std::shared_ptr<TilesetEntry> GetTileset(uint8_t index) const;
    std::shared_ptr<TilesetEntry> GetTileset(const std::string& name) const;
    std::shared_ptr<AnimatedTilesetEntry> GetAnimatedTileset(uint8_t tileset, uint8_t idx) const;
    std::shared_ptr<AnimatedTilesetEntry> GetAnimatedTileset(const std::string& name) const;
    std::shared_ptr<TilesetEntry> GetIntroFont() const;

    std::shared_ptr<PaletteEntry> GetRoomPalette(const std::string& name) const;
    std::shared_ptr<PaletteEntry> GetRoomPalette(uint8_t index) const;
    std::vector<std::shared_ptr<PaletteEntry>> GetMiscPalette(const MiscPaletteType& type) const;
    std::shared_ptr<PaletteEntry> GetDefaultTilesetPalette(const std::string& name) const;
    std::shared_ptr<PaletteEntry> GetDefaultTilesetPalette(uint8_t index) const;
    std::list<std::shared_ptr<PaletteEntry>> GetTilesetRecommendedPalettes(const std::string& name) const;
    std::list<std::shared_ptr<PaletteEntry>> GetTilesetRecommendedPalettes(uint8_t index) const;

    std::vector<std::shared_ptr<BlocksetEntry>> GetBlocksetList(const std::string& tileset) const;
    std::shared_ptr<BlocksetEntry> GetBlockset(const std::string& name) const;
    std::shared_ptr<BlocksetEntry> GetBlockset(uint8_t pri, uint8_t sec) const;
    std::shared_ptr<BlocksetEntry> GetBlockset(uint8_t tileset, uint8_t pri, uint8_t sec) const;
    std::shared_ptr<BlocksetEntry> GetBlockset(const std::string& tileset, uint8_t pri, uint8_t sec) const;

    const std::vector<std::shared_ptr<Room>>& GetRoomlist() const;
    std::size_t GetRoomCount() const;
    std::shared_ptr<Room> GetRoom(uint16_t index) const;
    std::shared_ptr<Room> GetRoom(const std::string& name) const;
    std::shared_ptr<Tilemap3DEntry> GetMap(const std::string& name) const;
    std::shared_ptr<PaletteEntry> GetPaletteForRoom(const std::string& name) const;
    std::shared_ptr<PaletteEntry> GetPaletteForRoom(uint16_t roomnum) const;
    std::shared_ptr<TilesetEntry> GetTilesetForRoom(const std::string& name) const;
    std::shared_ptr<TilesetEntry> GetTilesetForRoom(uint16_t roomnum) const;
    std::list<std::shared_ptr<BlocksetEntry>> GetBlocksetsForRoom(const std::string& name) const;
    std::list<std::shared_ptr<BlocksetEntry>> GetBlocksetsForRoom(uint16_t roomnum) const;
    std::shared_ptr<Blockset> GetCombinedBlocksetForRoom(const std::string& name) const;
    std::shared_ptr<Blockset> GetCombinedBlocksetForRoom(uint16_t roomnum) const;
    std::shared_ptr<Tilemap3DEntry> GetMapForRoom(const std::string& name) const;
    std::shared_ptr<Tilemap3DEntry> GetMapForRoom(uint16_t roomnum) const;

    std::list<WarpList::Warp> GetWarpsForRoom(uint16_t roomnum);
    bool HasFallDestination(uint16_t room) const;
    uint16_t GetFallDestination(uint16_t room) const;
    bool HasClimbDestination(uint16_t room) const;
    uint16_t GetClimbDestination(uint16_t room) const;
    std::map<std::pair<uint16_t, uint16_t>, uint16_t> GetTransitions(uint16_t room) const;

protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const filesystem::path& dir);

    bool AsmLoadRoomTable();
    bool AsmLoadMaps();
    bool AsmLoadRoomPalettes();
    bool AsmLoadWarpData();
    bool AsmLoadMiscPaletteData();
    bool AsmLoadBlocksetData();
    bool AsmLoadBlocksetPtrData();
    bool AsmLoadAnimatedTilesetData();
    bool AsmLoadTilesetData();

    bool RomLoadRoomData(const Rom& rom);
    bool RomLoadRoomPalettes(const Rom& rom);
    bool RomLoadWarpData(const Rom& rom);
    bool RomLoadMiscPaletteData(const Rom& rom);
    bool RomLoadBlocksetData(const Rom& rom);
    bool RomLoadBlockset(const Rom& rom, uint8_t pri, uint8_t sec, uint32_t begin, uint32_t end);
    bool RomLoadAllTilesetData(const Rom& rom);

    bool AsmSaveMaps(const filesystem::path& dir);
    bool AsmSaveRoomData(const filesystem::path& dir);
    bool AsmSaveWarpData(const filesystem::path& dir);
    bool AsmSaveRoomPalettes(const filesystem::path& dir);
    bool AsmSaveMiscPaletteData(const filesystem::path& dir);
    bool AsmSaveBlocksetPointerData(const filesystem::path& dir);
    bool AsmSaveBlocksetData(const filesystem::path& dir);
    bool AsmSaveTilesetData(const filesystem::path& dir);
    bool AsmSaveTilesetPointerData(const filesystem::path& dir);
    bool AsmSaveAnimatedTilesetData(const filesystem::path& dir);

    bool RomPrepareInjectMiscWarp(const Rom& rom);
    bool RomPrepareInjectRoomData(const Rom& rom);
    bool RomPrepareInjectMiscPaletteData(const Rom& rom);
    bool RomPrepareInjectBlocksetData(const Rom& rom);
    bool RomPrepareInjectTilesetData(const Rom& rom);
    bool RomPrepareInjectAnimatedTilesetData(const Rom& rom);

    void UpdateTilesetRecommendedPalettes();
    void ResetTilesetDefaultPalettes();

    filesystem::path m_room_data_filename;
    filesystem::path m_map_data_filename;
    filesystem::path m_warp_data_filename;
    filesystem::path m_fall_data_filename;
    filesystem::path m_climb_data_filename;
    filesystem::path m_transition_data_filename;
    filesystem::path m_palette_data_filename;
    filesystem::path m_lava_pal_data_filename;
    filesystem::path m_warp_pal_data_filename;
    filesystem::path m_lantern_pal_data_filename;
    filesystem::path m_tileset_data_filename;
    filesystem::path m_tileset_ptrtab_filename;
    filesystem::path m_tileset_anim_filename;
    filesystem::path m_blockset_pri_ptr_filename;
    filesystem::path m_blockset_sec_ptr_filename;
    filesystem::path m_blockset_data_filename;

    std::map<std::string, std::shared_ptr<TilesetEntry>> m_tilesets_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_tilesets_by_name_orig;
    std::map<uint8_t, std::shared_ptr<TilesetEntry>> m_tilesets;
    std::map<uint8_t, std::shared_ptr<TilesetEntry>> m_tilesets_orig;

    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> m_animated_ts_by_name;
    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> m_animated_ts_by_name_orig;
    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> m_animated_ts_by_ptr;
    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> m_animated_ts_by_ptr_orig;
    std::map<std::pair<uint8_t, uint8_t>, std::shared_ptr<AnimatedTilesetEntry>> m_animated_ts;
    std::map<std::pair<uint8_t, uint8_t>, std::shared_ptr<AnimatedTilesetEntry>> m_animated_ts_orig;

    std::unordered_map<std::string, std::shared_ptr<PaletteEntry>> m_tileset_defaultpal;
    std::unordered_map<std::string, std::list<std::shared_ptr<PaletteEntry>>> m_tileset_pals;

    std::shared_ptr<TilesetEntry> m_intro_font;

    std::map<std::string, std::shared_ptr<BlocksetEntry>> m_blocksets_by_name;
    std::map<std::string, std::shared_ptr<BlocksetEntry>> m_blocksets_by_name_orig;
    std::map<std::pair<uint8_t, uint8_t>, std::shared_ptr<BlocksetEntry>> m_blocksets;
    std::map<std::pair<uint8_t, uint8_t>, std::shared_ptr<BlocksetEntry>> m_blocksets_orig;

    std::vector<std::shared_ptr<Room>> m_roomlist;
    std::vector<std::shared_ptr<Room>> m_roomlist_orig;
    std::map<std::string, std::shared_ptr<Room>> m_roomlist_by_name;

    std::map<std::string, std::shared_ptr<Tilemap3DEntry>> m_maps;
    std::map<std::string, std::shared_ptr<Tilemap3DEntry>> m_maps_orig;

    WarpList m_warps;
    WarpList m_warps_orig;

    std::map<std::string, std::shared_ptr<PaletteEntry>> m_room_pals_by_name;
    std::vector<std::shared_ptr<PaletteEntry>> m_room_pals;
    std::vector<std::shared_ptr<PaletteEntry>> m_room_pals_orig;
    std::vector<std::shared_ptr<PaletteEntry>> m_lava_palette;
    std::vector<std::shared_ptr<PaletteEntry>> m_lava_palette_orig;
    std::vector<std::shared_ptr<PaletteEntry>> m_warp_palette;
    std::vector<std::shared_ptr<PaletteEntry>> m_warp_palette_orig;
    std::shared_ptr<PaletteEntry> m_labrynth_lit_palette;
};

#endif // _ROOM_DATA_H_