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

private:
    bool LoadRomRoomTable(const Rom& rom);
    bool GetAsmFilenames();
    bool LoadAsmRoomTable();
    bool LoadAsmMapData();
    bool SaveMapsToDisk(const filesystem::path& dir);
    bool SaveAsmMapFilenames(const filesystem::path& dir);
    bool SaveAsmRoomData(const filesystem::path& dir);

    filesystem::path m_asm_filename;
    filesystem::path m_base_path;
    filesystem::path m_room_data_filename;
    filesystem::path m_map_data_filename;

    std::vector<Room> m_roomlist;
    std::map<std::string, std::shared_ptr<MapEntry>> m_maps;
    mutable std::map<std::string, std::vector<uint8_t>> m_pending_write;
    mutable bool m_roomlist_pending_modifications;
};

#endif // _ROOM_MANAGER_H_