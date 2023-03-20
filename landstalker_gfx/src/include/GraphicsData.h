#ifndef _GRAPHICS_DATA_H_
#define _GRAPHICS_DATA_H_

#include "DataManager.h"
#include "DataTypes.h"
#include "LSString.h"
#include <memory>

/*  TODO
2D Maps (Sega Logo, Climax Logo, Lithograph, Intro, File Select)
*/

class GraphicsData : public DataManager
{
public:
    GraphicsData(const filesystem::path& asm_file);
    GraphicsData(const Rom& rom);

    virtual ~GraphicsData() {}

    virtual bool Save(const filesystem::path& dir);
    virtual bool Save();

    virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    std::map<std::string, std::shared_ptr<TilesetEntry>> GetAllTilesets() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetFonts() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetMiscGraphics() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetSwordEffects() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetStatusEffects() const;
    std::shared_ptr<TilesetEntry> GetEndCreditLogos() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetIslandMapTiles() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetTitleScreenTiles() const;

    std::map<std::string, std::shared_ptr<PaletteEntry>> GetAllPalettes() const;
protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const filesystem::path& dir);
    void InitCache();

    bool AsmLoadFonts();
    bool AsmLoadStrings();
    bool AsmLoadInventoryGraphics();
    bool AsmLoadCompressedStringData();
    bool AsmLoadPalettes();
    bool AsmLoadTextGraphics();
    bool AsmLoadSwordFx();
    bool AsmLoadStatusFx();
    bool AsmLoadHuffmanData();
    bool AsmLoadHudData();
    bool AsmLoadEndCreditData();
    bool AsmLoadIslandMapData();
    bool AsmLoadTitleScreenData();

    bool RomLoadFonts(const Rom& rom);
    bool RomLoadStrings(const Rom& rom);
    bool RomLoadInventoryGraphics(const Rom& rom);
    bool RomLoadCompressedStringData(const Rom& rom);
    bool RomLoadPalettes(const Rom& rom);
    bool RomLoadTextGraphics(const Rom& rom);
    bool RomLoadSwordFx(const Rom& rom);
    bool RomLoadStatusFx(const Rom& rom);
    bool RomLoadHuffmanData(const Rom& rom);
    bool RomLoadHudData(const Rom& rom);
    bool RomLoadEndCreditData(const Rom& rom);
    bool RomLoadIslandMapData(const Rom& rom);
    bool RomLoadTitleScreenData(const Rom& rom);

    bool AsmSaveGraphics(const filesystem::path& dir);
    bool AsmSaveStrings(const filesystem::path& dir);
    bool AsmSaveInventoryGraphics(const filesystem::path& dir);
    bool AsmSaveCompressedStringData(const filesystem::path& dir);
    bool AsmSaveSwordFx(const filesystem::path& dir);
    bool AsmSaveStatusFx(const filesystem::path& dir);
    bool AsmSaveHuffmanData(const filesystem::path& dir);
    bool AsmSaveEndCreditData(const filesystem::path& dir);
    bool AsmSaveIslandMapData(const filesystem::path& dir);
    bool AsmSaveTitleScreenData(const filesystem::path& dir);

    bool RomPrepareInjectFonts(const Rom& rom);
    bool RomPrepareInjectInvGraphics(const Rom& rom);
    bool RomPrepareInjectCompressedStringData(const Rom& rom);
    bool RomPrepareInjectPalettes(const Rom& rom);
    bool RomPrepareInjectTextGraphics(const Rom& rom);
    bool RomPrepareInjectSwordFx(const Rom& rom);
    bool RomPrepareInjectStatusFx(const Rom& rom);
    bool RomPrepareInjectHuffmanData(const Rom& rom);
    bool RomPrepareInjectHudData(const Rom& rom);
    bool RomPrepareInjectEndCreditData(const Rom& rom);
    bool RomPrepareInjectIslandMapData(const Rom& rom);
    bool RomPrepareInjectTitleScreenData(const Rom& rom);

    void UpdateTilesetRecommendedPalettes();
    void ResetTilesetDefaultPalettes();

    filesystem::path m_region_check_filename;
    filesystem::path m_region_check_routine_filename;
    filesystem::path m_region_check_strings_filename;
    filesystem::path m_system_font_filename;
    filesystem::path m_strings_filename;
    filesystem::path m_string_ptr_filename;
    filesystem::path m_inventory_graphics_filename;
    filesystem::path m_string_filename_path;
    filesystem::path m_status_fx_path;
    filesystem::path m_status_fx_pointers_path;
    filesystem::path m_sword_fx_path;
    filesystem::path m_huffman_offset_path;
    filesystem::path m_huffman_table_path;
    filesystem::path m_end_credits_path;
    filesystem::path m_island_map_path;
    filesystem::path m_island_map_routine_path;
    filesystem::path m_title_path;
    filesystem::path m_title_routines_1_path;
    filesystem::path m_title_routines_2_path;
    filesystem::path m_title_routines_3_path;

    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_misc_gfx_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_misc_gfx_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_misc_gfx_internal;
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
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_misc_tilemaps;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_misc_tilemaps_orig;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_misc_tilemaps_internal;
    std::array<std::string, 4> m_system_strings;
    std::array<std::string, 4> m_system_strings_orig;
    std::shared_ptr<Tilemap2DEntry> m_end_credits_map;
    std::shared_ptr<TilesetEntry> m_end_credits_tileset;
    std::shared_ptr<PaletteEntry> m_end_credits_palette;
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

    std::vector<std::vector<std::uint8_t>> m_strings;
    std::vector<std::vector<std::uint8_t>> m_strings_orig;
    std::vector<uint8_t> m_huffman_offsets;
    std::vector<uint8_t> m_huffman_offsets_orig;
    std::vector<uint8_t> m_huffman_tables;
    std::vector<uint8_t> m_huffman_tables_orig;
};

#endif // _GRAPHICS_DATA_H_
