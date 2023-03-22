#ifndef _GAME_DATA_H_
#define _GAME_DATA_H_

#include "DataManager.h"
#include "RoomData.h"
#include "GraphicsData.h"
#include "StringData.h"
#include "DataTypes.h"

#include <vector>
#include <memory>
#include <list>

class GameData : public DataManager
{
public:
    GameData(const filesystem::path& asm_file);
    GameData(const Rom& rom);

    virtual ~GameData() {}

    virtual bool Save(const filesystem::path& dir);
    virtual bool Save();

    virtual PendingWrites GetPendingWrites() const;
    virtual bool WillFitInRom(const Rom& rom) const;
    virtual bool HasBeenModified() const;
    virtual bool InjectIntoRom(Rom& rom);
    virtual void RefreshPendingWrites(const Rom& rom);

    std::shared_ptr<RoomData> GetRoomData() const { return m_rd; }
    std::shared_ptr<GraphicsData> GetGraphicsData() const { return m_gd; }
    std::shared_ptr<StringData> GetStringData() const { return m_sd; }

    const std::map<std::string, std::shared_ptr<PaletteEntry>>& GetAllPalettes() const;
    const std::map<std::string, std::shared_ptr<TilesetEntry>>& GetAllTilesets() const;
    const std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>>& GetAllAnimatedTilesets() const;
    const std::map<std::string, std::shared_ptr<Tilemap2DEntry>>& GetAllTilemaps() const;

    std::shared_ptr<PaletteEntry> GetPalette(const std::string& name) const;
    std::shared_ptr<TilesetEntry> GetTileset(const std::string& name) const;
    std::shared_ptr<AnimatedTilesetEntry> GetAnimatedTileset(const std::string& name) const;
    std::shared_ptr<Tilemap2DEntry> GetTilemap(const std::string& name) const;

private:
    void CacheData();
    void SetDefaults();

    std::shared_ptr<RoomData> m_rd;
    std::shared_ptr<GraphicsData> m_gd;
    std::shared_ptr<StringData> m_sd;

    std::vector<std::shared_ptr<DataManager>> m_data;

    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_tilesets;
    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> m_anim_tilesets;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_tilemaps;
};

#endif // _GAME_DATA_H_
