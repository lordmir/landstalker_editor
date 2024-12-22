#ifndef _GAME_DATA_H_
#define _GAME_DATA_H_

#include <vector>
#include <memory>
#include <list>
#include <atomic>

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/RoomData.h>
#include <landstalker/main/include/GraphicsData.h>
#include <landstalker/main/include/StringData.h>
#include <landstalker/main/include/SpriteData.h>
#include <landstalker/main/include/ScriptData.h>
#include <landstalker/main/include/DataTypes.h>

class GameData : public DataManager
{
public:
    GameData();
    GameData(const std::filesystem::path& asm_file);
    GameData(const Rom& rom);

    virtual ~GameData() {}

    bool Open(const std::filesystem::path& asm_file);
    bool Open(const Rom& rom);

    virtual bool Save(const std::filesystem::path& dir);
    virtual bool Save();

    virtual PendingWrites GetPendingWrites() const;
    virtual bool WillFitInRom(const Rom& rom) const;
    virtual bool HasBeenModified() const;
    virtual bool InjectIntoRom(Rom& rom);
    virtual void RefreshPendingWrites(const Rom& rom);

    std::shared_ptr<RoomData> GetRoomData() const { return m_ready ? m_rd : nullptr; }
    std::shared_ptr<GraphicsData> GetGraphicsData() const { return m_ready ? m_gd : nullptr; }
    std::shared_ptr<StringData> GetStringData() const { return m_ready ? m_sd : nullptr; }
    std::shared_ptr<SpriteData> GetSpriteData() const { return m_ready ? m_spd : nullptr; }
    std::shared_ptr<ScriptData> GetScriptData() const { return m_ready ? m_scd : nullptr; }

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
    std::shared_ptr<SpriteData> m_spd;
    std::shared_ptr<ScriptData> m_scd;

    std::vector<std::shared_ptr<DataManager>> m_data;

    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_tilesets;
    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> m_anim_tilesets;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_tilemaps;
};

#endif // _GAME_DATA_H_
