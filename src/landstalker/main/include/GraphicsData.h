#ifndef _GRAPHICS_DATA_H_
#define _GRAPHICS_DATA_H_

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/text/include/LSString.h>
#include <memory>


class GraphicsData : public DataManager
{
public:
    GraphicsData(const std::filesystem::path& asm_file);
    GraphicsData(const Rom& rom);

    virtual ~GraphicsData() {}

    virtual bool Save(const std::filesystem::path& dir);
    virtual bool Save();

    virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    std::map<std::string, std::shared_ptr<TilesetEntry>> GetAllTilesets() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetFonts() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetUIGraphics() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetSwordEffects() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetStatusEffects() const;
    std::shared_ptr<TilesetEntry> GetEndCreditLogosTiles() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetIslandMapTiles() const;
    std::shared_ptr<TilesetEntry> GetLithographTiles() const;
    std::shared_ptr<TilesetEntry> GetSegaLogoTiles() const;
    std::shared_ptr<TilesetEntry> GetClimaxLogoTiles() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetTitleScreenTiles() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetGameLoadScreenTiles() const;

    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> GetAllMaps() const;
    std::shared_ptr<Tilemap2DEntry> GetMap(const std::string&) const;
    std::vector<std::shared_ptr<Tilemap2DEntry>> GetUIMaps() const;
    std::shared_ptr<Tilemap2DEntry> GetEndCreditLogosMaps() const;
    std::vector<std::shared_ptr<Tilemap2DEntry>> GetIslandMapMaps() const;
    std::shared_ptr<Tilemap2DEntry> GetLithographMap() const;
    std::shared_ptr<Tilemap2DEntry> GetClimaxLogoMap() const;
    std::vector<std::shared_ptr<Tilemap2DEntry>> GetTitleScreenMap() const;
    std::shared_ptr<Tilemap2DEntry> GetGameLoadScreenMap() const;

    std::map<std::string, std::shared_ptr<PaletteEntry>> GetAllPalettes() const;
    std::map<std::string, std::shared_ptr<PaletteEntry>> GetSwordPalettes() const;
    std::map<std::string, std::shared_ptr<PaletteEntry>> GetArmourPalettes() const;
    std::map<std::string, std::shared_ptr<PaletteEntry>> GetOtherPalettes() const;
    std::shared_ptr<PaletteEntry> GetPlayerPalette() const;
    std::shared_ptr<PaletteEntry> GetHudPalette() const;
protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const std::filesystem::path& dir);
    void InitCache();

    bool AsmLoadInventoryGraphics();
    bool AsmLoadPalettes();
    bool AsmLoadTextGraphics();
    bool AsmLoadSwordFx();
    bool AsmLoadStatusFx();
    bool AsmLoadHudData();
    bool AsmLoadEndCreditData();
    bool AsmLoadIslandMapData();
    bool AsmLoadLithographData();
    bool AsmLoadTitleScreenData();
    bool AsmLoadSegaLogoData();
    bool AsmLoadClimaxLogoData();
    bool AsmLoadLoadGameScreenData();

    bool RomLoadInventoryGraphics(const Rom& rom);
    bool RomLoadPalettes(const Rom& rom);
    bool RomLoadTextGraphics(const Rom& rom);
    bool RomLoadSwordFx(const Rom& rom);
    bool RomLoadStatusFx(const Rom& rom);
    bool RomLoadHudData(const Rom& rom);
    bool RomLoadEndCreditData(const Rom& rom);
    bool RomLoadIslandMapData(const Rom& rom);
    bool RomLoadLithographData(const Rom& rom);
    bool RomLoadTitleScreenData(const Rom& rom);
    bool RomLoadSegaLogoData(const Rom& rom);
    bool RomLoadClimaxLogoData(const Rom& rom);
    bool RomLoadGameLoadScreenData(const Rom& rom);

    bool AsmSaveGraphics(const std::filesystem::path& dir);
    bool AsmSaveInventoryGraphics(const std::filesystem::path& dir);
    bool AsmSaveSwordFx(const std::filesystem::path& dir);
    bool AsmSaveStatusFx(const std::filesystem::path& dir);
    bool AsmSaveEndCreditData(const std::filesystem::path& dir);
    bool AsmSaveIslandMapData(const std::filesystem::path& dir);
    bool AsmSaveLithographData(const std::filesystem::path& dir);
    bool AsmSaveTitleScreenData(const std::filesystem::path& dir);
    bool AsmSaveSegaLogoData(const std::filesystem::path& dir);
    bool AsmSaveClimaxLogoData(const std::filesystem::path& dir);
    bool AsmSaveGameLoadData(const std::filesystem::path& dir);

    bool RomPrepareInjectInvGraphics(const Rom& rom);
    bool RomPrepareInjectPalettes(const Rom& rom);
    bool RomPrepareInjectTextGraphics(const Rom& rom);
    bool RomPrepareInjectSwordFx(const Rom& rom);
    bool RomPrepareInjectStatusFx(const Rom& rom);
    bool RomPrepareInjectHudData(const Rom& rom);
    bool RomPrepareInjectEndCreditData(const Rom& rom);
    bool RomPrepareInjectIslandMapData(const Rom& rom);
    bool RomPrepareInjectLithographData(const Rom& rom);
    bool RomPrepareInjectTitleScreenData(const Rom& rom);
    bool RomPrepareInjectSegaLogoData(const Rom& rom);
    bool RomPrepareInjectClimaxLogoData(const Rom& rom);
    bool RomPrepareInjectGameLoadScreenData(const Rom& rom);

    void UpdateTilesetRecommendedPalettes();
    void ResetTilesetDefaultPalettes();

    std::filesystem::path m_inventory_graphics_filename;
    std::filesystem::path m_status_fx_path;
    std::filesystem::path m_status_fx_pointers_path;
    std::filesystem::path m_sword_fx_path;
    std::filesystem::path m_end_credits_path;
    std::filesystem::path m_island_map_path;
    std::filesystem::path m_title_path;
    std::filesystem::path m_title_routines_1_path;
    std::filesystem::path m_title_routines_2_path;
    std::filesystem::path m_title_routines_3_path;
    std::filesystem::path m_sega_logo_path;
    std::filesystem::path m_sega_logo_routines_1_path;
    std::filesystem::path m_sega_logo_routines_2_path;
    std::filesystem::path m_climax_logo_path;
    std::filesystem::path m_load_game_path;
    std::filesystem::path m_load_game_routines_1_path;
    std::filesystem::path m_load_game_routines_2_path;
    std::filesystem::path m_load_game_routines_3_path;
    std::filesystem::path m_lithograph_path;

    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_ui_gfx_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_ui_gfx_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_ui_gfx_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_sword_fx;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_sword_fx_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_sword_fx_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_status_fx_frames;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_status_fx_frames_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_status_fx_frames_internal;
    std::map<std::string, std::vector<std::string>> m_status_fx;
    std::map<std::string, std::vector<std::string>> m_status_fx_orig;
    std::map<std::string, std::vector<std::string>> m_status_fx_internal;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes_by_name;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes_by_name_orig;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes_internal;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_sword_palettes;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_armour_palettes;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_misc_palettes;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps_orig;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps_internal;
    std::shared_ptr<Tilemap2DEntry> m_end_credits_map;
    std::shared_ptr<TilesetEntry> m_end_credits_tileset;
    std::shared_ptr<PaletteEntry> m_end_credits_palette;
    std::shared_ptr<TilesetEntry> m_sega_logo_tileset;
    std::shared_ptr<PaletteEntry> m_sega_logo_palette;
    std::shared_ptr<Tilemap2DEntry> m_climax_logo_map;
    std::shared_ptr<TilesetEntry> m_climax_logo_tileset;
    std::shared_ptr<PaletteEntry> m_climax_logo_palette;
    std::shared_ptr<Tilemap2DEntry> m_load_game_map;
    std::shared_ptr<Tilemap2DEntry> m_lithograph_map;
    std::shared_ptr<TilesetEntry> m_lithograph_tileset;
    std::shared_ptr<PaletteEntry> m_lithograph_palette;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_island_map_tilemaps;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_island_map_tilemaps_orig;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_island_map_tilemaps_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_island_map_tiles;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_island_map_tiles_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_island_map_tiles_internal;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_island_map_pals;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_island_map_pals_orig;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_island_map_pals_internal;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_title_tilemaps;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_title_tilemaps_orig;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_title_tilemaps_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_title_tiles;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_title_tiles_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_title_tiles_internal;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_title_pals;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_title_pals_orig;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_title_pals_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_load_game_tiles;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_load_game_tiles_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_load_game_tiles_internal;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_load_game_pals;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_load_game_pals_orig;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_load_game_pals_internal;
};

#endif // _GRAPHICS_DATA_H_
