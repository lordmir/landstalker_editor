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

class RoomManager
{
public:

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

    RoomManager(const filesystem::path& asm_file);
    RoomManager(const Rom& rom);

    bool Save(filesystem::path dir);
    bool Save();

    bool HasBeenModified();
    bool HasMapBeenModified(const std::string& map);

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

private:
    bool LoadRomRoomTable(const Rom& rom);
    bool LoadRomWarpData(const Rom& rom);
    bool LoadRoomPalettes(const Rom& rom);
    bool GetAsmFilenames();
    bool LoadAsmRoomTable();
    bool LoadAsmMapData();
    bool LoadAsmWarpData();
    bool SaveMapsToDisk(const filesystem::path& dir);
    bool SaveAsmMapFilenames(const filesystem::path& dir);
    bool SaveAsmRoomData(const filesystem::path& dir);

    filesystem::path m_asm_filename;
    filesystem::path m_base_path;
    filesystem::path m_room_data_filename;
    filesystem::path m_map_data_filename;
    filesystem::path m_warp_data_filename;
    filesystem::path m_fall_data_filename;
    filesystem::path m_climb_data_filename;
    filesystem::path m_transition_data_filename;

    std::vector<Room> m_roomlist;
    std::map<std::string, std::shared_ptr<MapEntry>> m_maps;
    WarpList m_warps;
    mutable std::map<std::string, std::vector<uint8_t>> m_pending_write;
    mutable bool m_roomlist_pending_modifications;
};

#endif // _ROOM_MANAGER_H_