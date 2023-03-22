#ifndef _STRING_DATA_H_
#define _STRING_DATA_H_

#include "DataManager.h"
#include "DataTypes.h"

class StringData : public DataManager
{
public:
    StringData(const filesystem::path& asm_file);
    StringData(const Rom& rom);

    virtual ~StringData() {}

    virtual bool Save(const filesystem::path& dir);
    virtual bool Save();

    virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    std::map<std::string, std::shared_ptr<TilesetEntry>> GetAllTilesets() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetFonts() const;

    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> GetAllMaps() const;
    std::vector<std::shared_ptr<Tilemap2DEntry>> GetTextboxMaps() const;

protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const filesystem::path& dir);
    void InitCache();

    bool AsmLoadSystemFont();
    bool AsmLoadSystemStrings();
    bool AsmLoadCompressedStringData();
    bool AsmLoadHuffmanData();

    bool RomLoadSystemFont(const Rom& rom);
    bool RomLoadSystemStrings(const Rom& rom);
    bool RomLoadCompressedStringData(const Rom& rom);
    bool RomLoadHuffmanData(const Rom& rom);

    bool AsmSaveFonts(const filesystem::path& dir);
    bool AsmSaveSystemText(const filesystem::path& dir);
    bool AsmSaveCompressedStringData(const filesystem::path& dir);
    bool AsmSaveHuffmanData(const filesystem::path& dir);

    bool RomPrepareInjectSystemText(const Rom& rom);
    bool RomPrepareInjectCompressedStringData(const Rom& rom);
    bool RomPrepareInjectHuffmanData(const Rom& rom);

    filesystem::path m_region_check_filename;
    filesystem::path m_region_check_routine_filename;
    filesystem::path m_region_check_strings_filename;
    filesystem::path m_system_font_filename;
    filesystem::path m_strings_filename;
    filesystem::path m_string_ptr_filename;
    filesystem::path m_string_filename_path;
    filesystem::path m_huffman_offset_path;
    filesystem::path m_huffman_table_path;

    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_internal;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps_orig;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps_internal;

    std::vector<std::vector<std::uint8_t>> m_compressed_strings;
    std::vector<std::vector<std::uint8_t>> m_compressed_strings_orig;
    std::array<std::string, 4> m_system_strings;
    std::array<std::string, 4> m_system_strings_orig;
    std::vector<uint8_t> m_huffman_offsets;
    std::vector<uint8_t> m_huffman_offsets_orig;
    std::vector<uint8_t> m_huffman_tables;
    std::vector<uint8_t> m_huffman_tables_orig;
};

#endif // _STRING_DATA_H_
