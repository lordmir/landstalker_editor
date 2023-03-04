#ifndef _ROOM_MANAGER_H_
#define _ROOM_MANAGER_H_

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include "wjakob/filesystem/path.h"

#include <AsmFile.h>
#include <Rom.h>
#include <Room.h>
#include <Tilemap3DCmp.h>
#include <WarpList.h>
#include <Palette.h>
#include <Tileset.h>
#include <BlocksetCmp.h>

using SizeReport = std::vector<std::pair<std::string, int>>;

class RoomManager
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

    struct MapEntry
    {
        std::string name;
        std::shared_ptr<Tilemap3D> orig_map;
        std::shared_ptr<Tilemap3D> map;
        uint32_t start_address;
        uint32_t end_address;
        filesystem::path filename;
        std::shared_ptr<std::vector<uint8_t>> raw_data;
    };

    struct PaletteEntry
    {
        std::string name;
        uint16_t index;
        std::shared_ptr<Palette> orig_pal;
        std::shared_ptr<Palette> pal;
        uint32_t start_address;
        uint32_t end_address;
        filesystem::path filename;
        std::shared_ptr<std::vector<uint8_t>> raw_data;
    };

    RoomManager(const filesystem::path& asm_file);
    RoomManager(const Rom& rom);

    bool CheckDataWillFitInRom(const Rom& rom, int& tilesets_size, int& anim_table_size) const;
    bool HasTilesetBeenModified(const std::string& tileset) const;
    bool HasBeenModified() const;
    bool InjectIntoRom(Rom& rom);
    bool Save(filesystem::path dir);
    bool Save();
    std::shared_ptr<Tileset> GetTileset(const std::string& name);

    std::size_t GetRoomCount() const;
    const Room& GetRoom(uint16_t idx) const;
    Room& GetRoom(uint16_t idx);
    std::vector<std::string> GetMapList() const;
    std::shared_ptr<MapEntry> GetMap(const std::string& name);
    std::shared_ptr<MapEntry> GetMap(uint16_t roomnum);
    std::list<WarpList::Warp> GetWarpsForRoom(uint16_t roomnum);
    bool HasFallDestination(uint16_t room) const;
    uint16_t GetFallDestination(uint16_t room) const;
    bool HasClimbDestination(uint16_t room) const;
    uint16_t GetClimbDestination(uint16_t room) const;
    std::map<std::pair<uint16_t, uint16_t>, uint16_t> GetTransitions(uint16_t room) const;
    std::shared_ptr<const Palette> GetRoomPalette(unsigned int index) const;
    std::shared_ptr<Palette> GetRoomPalette(unsigned int index);

private:
    bool LoadRomRoomTable(const Rom& rom);
    bool LoadRomWarpData(const Rom& rom);
    bool LoadRomRoomPalettes(const Rom& rom);
    bool LoadRomMiscPalettes(const Rom& rom);
    bool GetAsmFilenames();
    bool LoadAsmRoomTable();
    bool LoadAsmMapData();
    bool LoadAsmWarpData();
    bool LoadAsmRoomPalettes();
    bool LoadAsmMiscPalettes();
    bool SaveMapsToDisk(const filesystem::path& dir);
    bool SaveAsmMapFilenames(const filesystem::path& dir);
    bool SaveAsmRoomData(const filesystem::path& dir);
    bool SaveAsmWarpData(const filesystem::path& dir);
    bool SaveAsmRoomPalettes(const filesystem::path& dir);
    bool SaveAsmMiscPalettes(const filesystem::path& dir);
    bool SaveAsmPaletteFilenames(const filesystem::path& dir);

    filesystem::path m_asm_filename;
    filesystem::path m_base_path;
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

    std::vector<Room> m_roomlist;
    std::vector<Room> m_roomlist_orig;
    std::map<std::string, std::shared_ptr<MapEntry>> m_maps;
    WarpList m_warps;
    WarpList m_warps_orig;
    std::vector<PaletteEntry> m_room_pals;
    std::vector<PaletteEntry> m_lava_palette;
    std::vector<PaletteEntry> m_warp_palette;
    PaletteEntry m_labrynth_lit_palette;
    mutable std::map<std::string, std::vector<uint8_t>> m_pending_write;
    mutable bool m_roomlist_pending_modifications;
};

#endif // _ROOM_MANAGER_H_