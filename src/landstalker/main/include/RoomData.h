#ifndef _ROOM_DATA_H_
#define _ROOM_DATA_H_

#include <map>
#include <vector>
#include <memory>

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/rooms/include/Room.h>
#include <landstalker/rooms/include/WarpList.h>
#include <landstalker/rooms/include/Chests.h>
#include <landstalker/3d_maps/include/Doors.h>
#include <landstalker/3d_maps/include/TileSwaps.h>
#include <landstalker/rooms/include/Flags.h>

class GameData;

class RoomData : public DataManager
{
public:
    enum class MiscPaletteType
    {
        LAVA,
        WARP,
        LANTERN
    };

    RoomData(const std::filesystem::path& asm_file);
    RoomData(const Rom& rom);

    virtual ~RoomData() {}

    virtual bool Save(const std::filesystem::path& dir);
    virtual bool Save();

	virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    std::wstring GetTilesetDisplayName(uint8_t index) const;
    std::wstring GetAnimatedTilesetDisplayName(uint8_t tileset, uint8_t index) const;
    std::wstring GetRoomPaletteDisplayName(uint8_t index) const;
    std::wstring GetBlocksetDisplayName(uint8_t tileset, uint8_t pri, uint8_t sec) const;
    std::wstring GetRoomDisplayName(uint16_t room) const;
    std::wstring GetMapDisplayName(const std::string& map) const;

    std::vector<std::shared_ptr<TilesetEntry>> GetTilesets() const;
    std::vector<std::shared_ptr<AnimatedTilesetEntry>> GetAnimatedTilesets(const std::string& tileset) const;
    bool HasAnimatedTilesets(const std::string& tileset) const;
    std::shared_ptr<TilesetEntry> GetTileset(uint8_t index) const;
    std::shared_ptr<TilesetEntry> GetTileset(const std::string& name) const;
    std::map<std::string, std::shared_ptr<TilesetEntry>> GetAllTilesets() const;
    std::shared_ptr<AnimatedTilesetEntry> GetAnimatedTileset(uint8_t tileset, uint8_t idx) const;
    std::shared_ptr<AnimatedTilesetEntry> GetAnimatedTileset(const std::string& name) const;
    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> GetAllAnimatedTilesets() const;
    std::shared_ptr<TilesetEntry> GetIntroFont() const;

    std::shared_ptr<PaletteEntry> GetRoomPalette(const std::string& name) const;
    std::shared_ptr<PaletteEntry> GetRoomPalette(uint8_t index) const;
    const std::vector<std::shared_ptr<PaletteEntry>>& GetRoomPalettes() const;
    std::vector<std::shared_ptr<PaletteEntry>> GetMiscPalette(const MiscPaletteType& type) const;
    std::map<std::string, std::shared_ptr<PaletteEntry>> GetAllPalettes() const;
    std::shared_ptr<PaletteEntry> GetDefaultTilesetPalette(const std::string& name) const;
    std::shared_ptr<PaletteEntry> GetDefaultTilesetPalette(uint8_t index) const;
    std::list<std::shared_ptr<PaletteEntry>> GetTilesetRecommendedPalettes(const std::string& name) const;
    std::list<std::shared_ptr<PaletteEntry>> GetTilesetRecommendedPalettes(uint8_t index) const;

    std::vector<std::shared_ptr<BlocksetEntry>> GetBlocksetList(const std::string& tileset) const;
    std::map<std::string, std::shared_ptr<BlocksetEntry>> GetAllBlocksets() const;
    std::shared_ptr<BlocksetEntry> GetBlockset(const std::string& name) const;
    std::shared_ptr<BlocksetEntry> GetBlockset(uint8_t pri, uint8_t sec) const;
    std::shared_ptr<BlocksetEntry> GetBlockset(uint8_t tileset, uint8_t pri, uint8_t sec) const;
    std::shared_ptr<BlocksetEntry> GetBlockset(const std::string& tileset, uint8_t pri, uint8_t sec) const;

    const std::vector<std::shared_ptr<Room>>& GetRoomlist() const;
    std::size_t GetRoomCount() const;
    std::shared_ptr<Room> GetRoom(uint16_t index) const;
    std::shared_ptr<Room> GetRoom(const std::string& name) const;
    const std::map<std::string, std::shared_ptr<Tilemap3DEntry>>& GetMaps() const;
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
    std::vector<uint8_t> GetChestsForRoom(uint16_t roomnum) const;
    void SetChestsForRoom(uint16_t roomnum, const std::vector<uint8_t>& chests);
    bool GetNoChestFlagForRoom(uint16_t roomnum) const;
    void SetNoChestFlagForRoom(uint16_t roomnum, bool flag);
    bool CleanupChests(const GameData& g);

    std::vector<WarpList::Warp> GetWarpsForRoom(uint16_t roomnum);
    void SetWarpsForRoom(uint16_t roomnum, const std::vector<WarpList::Warp>& warps);
    bool HasFallDestination(uint16_t room) const;
    uint16_t GetFallDestination(uint16_t room) const;
    void SetHasFallDestination(uint16_t room, bool enabled);
    void SetFallDestination(uint16_t room, uint16_t dest);
    bool HasClimbDestination(uint16_t room) const;
    uint16_t GetClimbDestination(uint16_t room) const;
    void SetHasClimbDestination(uint16_t room, bool enabled);
    void SetClimbDestination(uint16_t room, uint16_t dest);
    std::vector<WarpList::Transition> GetTransitions(uint16_t room) const;
    std::vector<WarpList::Transition> GetSrcTransitions(uint16_t room) const;
    void SetSrcTransitions(uint16_t room, const std::vector<WarpList::Transition>& data);

    bool IsShop(uint16_t room) const;
    void SetShop(uint16_t room, bool is_shop);
    bool IsTree(uint16_t room) const;
    void SetTree(uint16_t room, bool is_tree);
    bool HasLifestockSaleFlag(uint16_t room) const;
    uint16_t GetLifestockSaleFlag(uint16_t room) const;
    void SetLifestockSaleFlag(uint16_t room, uint16_t flag);
    void ClearLifestockSaleFlag(uint16_t room);
    bool HasLanternFlag(uint16_t room) const;
    uint16_t GetLanternFlag(uint16_t room) const;
    void SetLanternFlag(uint16_t room, uint16_t flag);
    void ClearLanternFlag(uint16_t room);
    bool HasTreeWarpFlag(uint16_t room) const;
    TreeWarpFlag GetTreeWarp(uint16_t room) const;
    void SetTreeWarp(const TreeWarpFlag& flag);
    void ClearTreeWarp(uint16_t room);

    bool HasNormalTileSwaps(uint16_t room) const;
    std::vector<TileSwapFlag> GetNormalTileSwaps(uint16_t room) const;
    void SetNormalTileSwaps(uint16_t room, const std::vector<TileSwapFlag>& swaps);
    bool HasLockedDoorTileSwaps(uint16_t room) const;
    std::vector<TileSwapFlag> GetLockedDoorTileSwaps(uint16_t room) const;
    void SetLockedDoorTileSwaps(uint16_t room, const std::vector<TileSwapFlag>& swaps);
    bool HasTileSwaps(uint16_t room) const;
    std::vector<TileSwap> GetTileSwaps(uint16_t room) const;
    void SetTileSwaps(uint16_t room, const std::vector<TileSwap>& swaps);
    bool HasDoors(uint16_t room) const;
    std::vector<Door> GetDoors(uint16_t room) const;
    void SetDoors(uint16_t room, const std::vector<Door>& swaps);


protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const std::filesystem::path& dir);

    bool AsmLoadRoomTable();
    bool AsmLoadMaps();
    bool AsmLoadRoomPalettes();
    bool AsmLoadWarpData();
    bool AsmLoadMiscPaletteData();
    bool AsmLoadBlocksetData();
    bool AsmLoadBlocksetPtrData();
    bool AsmLoadAnimatedTilesetData();
    bool AsmLoadTilesetData();
    bool AsmLoadChestData();
    bool AsmLoadDoorData();
    bool AsmLoadGfxSwapData();
    bool AsmLoadMiscData();

    bool RomLoadRoomData(const Rom& rom);
    bool RomLoadRoomPalettes(const Rom& rom);
    bool RomLoadWarpData(const Rom& rom);
    bool RomLoadMiscPaletteData(const Rom& rom);
    bool RomLoadBlocksetData(const Rom& rom);
    bool RomLoadBlockset(const Rom& rom, uint8_t pri, uint8_t sec, uint32_t begin, uint32_t end);
    bool RomLoadAllTilesetData(const Rom& rom);
    bool RomLoadChestData(const Rom& rom);
    bool RomLoadDoorData(const Rom& rom);
    bool RomLoadGfxSwapData(const Rom& rom);
    bool RomLoadMiscData(const Rom& rom);

    bool AsmSaveMaps(const std::filesystem::path& dir);
    bool AsmSaveRoomData(const std::filesystem::path& dir);
    bool AsmSaveWarpData(const std::filesystem::path& dir);
    bool AsmSaveRoomPalettes(const std::filesystem::path& dir);
    bool AsmSaveMiscPaletteData(const std::filesystem::path& dir);
    bool AsmSaveBlocksetPointerData(const std::filesystem::path& dir);
    bool AsmSaveBlocksetData(const std::filesystem::path& dir);
    bool AsmSaveTilesetData(const std::filesystem::path& dir);
    bool AsmSaveTilesetPointerData(const std::filesystem::path& dir);
    bool AsmSaveAnimatedTilesetData(const std::filesystem::path& dir);
    bool AsmSaveChestData(const std::filesystem::path& dir);
    bool AsmSaveDoorData(const std::filesystem::path& dir);
    bool AsmSaveGfxSwapData(const std::filesystem::path& dir);
    bool AsmSaveMiscData(const std::filesystem::path& dir);

    bool RomPrepareInjectMiscWarp(const Rom& rom);
    bool RomPrepareInjectRoomData(const Rom& rom);
    bool RomPrepareInjectMiscPaletteData(const Rom& rom);
    bool RomPrepareInjectBlocksetData(const Rom& rom);
    bool RomPrepareInjectTilesetData(const Rom& rom);
    bool RomPrepareInjectAnimatedTilesetData(const Rom& rom);
    bool RomPrepareInjectChestData(const Rom& rom);
    bool RomPrepareInjectDoorData(const Rom& rom);
    bool RomPrepareInjectGfxSwapData(const Rom& rom);
    bool RomPrepareInjectMiscData(const Rom& rom);

    void UpdateTilesetRecommendedPalettes();
    void ResetTilesetDefaultPalettes();

    std::filesystem::path m_room_data_filename;
    std::filesystem::path m_map_data_filename;
    std::filesystem::path m_warp_data_filename;
    std::filesystem::path m_fall_data_filename;
    std::filesystem::path m_climb_data_filename;
    std::filesystem::path m_transition_data_filename;
    std::filesystem::path m_palette_data_filename;
    std::filesystem::path m_lava_pal_data_filename;
    std::filesystem::path m_warp_pal_data_filename;
    std::filesystem::path m_lantern_pal_data_filename;
    std::filesystem::path m_tileset_data_filename;
    std::filesystem::path m_tileset_ptrtab_filename;
    std::filesystem::path m_tileset_anim_filename;
    std::filesystem::path m_blockset_pri_ptr_filename;
    std::filesystem::path m_blockset_sec_ptr_filename;
    std::filesystem::path m_blockset_data_filename;
    std::filesystem::path m_chest_offset_data_filename;
    std::filesystem::path m_chest_data_filename;
    std::filesystem::path m_door_offset_data_filename;
    std::filesystem::path m_door_table_data_filename;
    std::filesystem::path m_gfxswap_flag_data_filename;
    std::filesystem::path m_gfxswap_locked_door_flag_data_filename;
    std::filesystem::path m_gfxswap_big_tree_flag_data_filename;
    std::filesystem::path m_gfxswap_table_data_filename;
    std::filesystem::path m_shop_table_data_filename;
    std::filesystem::path m_lifestock_sold_flag_data_filename;
    std::filesystem::path m_bigtree_data_filename;
    std::filesystem::path m_lantern_flag_data_filename;

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

    Chests m_chests;
    Chests m_chests_orig;
    Doors m_doors;
    Doors m_doors_orig;
    TileSwaps m_gfxswaps;
    TileSwaps m_gfxswaps_orig;

    std::map<uint16_t, std::vector<TileSwapFlag>> m_gfxswap_flags;
    std::map<uint16_t, std::vector<TileSwapFlag>> m_gfxswap_flags_orig;
    std::map<uint16_t, std::vector<TileSwapFlag>> m_gfxswap_locked_door_flags;
    std::map<uint16_t, std::vector<TileSwapFlag>> m_gfxswap_locked_door_flags_orig;
    std::vector<TreeWarpFlag> m_gfxswap_big_tree_flags;
    std::vector<TreeWarpFlag> m_gfxswap_big_tree_flags_orig;
    std::set<uint16_t> m_shop_list;
    std::set<uint16_t> m_shop_list_orig;
    std::map<uint16_t, uint16_t> m_lifestock_sold_flags;
    std::map<uint16_t, uint16_t> m_lifestock_sold_flags_orig;
    std::set<uint16_t> m_big_tree_list;
    std::set<uint16_t> m_big_tree_list_orig;
    std::map<uint16_t, uint16_t> m_lantern_flag_list;
    std::map<uint16_t, uint16_t> m_lantern_flag_list_orig;
};

#endif // _ROOM_DATA_H_