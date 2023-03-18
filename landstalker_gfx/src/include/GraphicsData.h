#ifndef _GRAPHICS_DATA_H_
#define _GRAPHICS_DATA_H_

#include "DataManager.h"
#include "DataTypes.h"
#include "LSString.h"
#include <memory>

/*  TODO
Fonts (Main, Menu, System, End Credits)
2D Maps (Sega Logo, Climax Logo, Island Map, Lithograph, Intro, Credits Logos, File Select)
HUD (Tiles, Textboxes, Inventory, etc.)
SwordGfx
StatusFx
Palettes
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

    bool RomLoadFonts(const Rom& rom);
    bool RomLoadStrings(const Rom& rom);
    bool RomLoadInventoryGraphics(const Rom& rom);
    bool RomLoadCompressedStringData(const Rom& rom);
    bool RomLoadPalettes(const Rom& rom);
    bool RomLoadTextGraphics(const Rom& rom);

    bool AsmSaveGraphics(const filesystem::path& dir);
    bool AsmSaveStrings(const filesystem::path& dir);
    bool AsmSaveInventoryGraphics(const filesystem::path& dir);
    bool AsmSaveCompressedStringData(const filesystem::path& dir);

    bool RomPrepareInjectFonts(const Rom& rom);
    bool RomPrepareInjectInvGraphics(const Rom& rom);
    bool RomPrepareInjectCompressedStringData(const Rom& rom);
    bool RomPrepareInjectPalettes(const Rom& rom);
    bool RomPrepareInjectTextGraphics(const Rom& rom);

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

    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_internal;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_misc_gfx_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_misc_gfx_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_misc_gfx_internal;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes_by_name;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes_by_name_orig;
    std::map<std::string, std::shared_ptr<PaletteEntry>> m_palettes_internal;
    std::array<std::string, 4> m_system_strings;
    std::array<std::string, 4> m_system_strings_orig;

    std::vector<std::vector<std::uint8_t>> m_strings;
    std::vector<std::vector<std::uint8_t>> m_strings_orig;
};

#endif // _GRAPHICS_DATA_H_
