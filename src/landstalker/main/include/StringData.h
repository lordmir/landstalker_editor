#ifndef _STRING_DATA_H_
#define _STRING_DATA_H_

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/text/include/HuffmanString.h>
#include <landstalker/text/include/IntroString.h>
#include <landstalker/text/include/EndCreditString.h>
#include <landstalker/text/include/Charset.h>
#include <landstalker/rooms/include/RoomDialogueTable.h>

class StringData : public DataManager
{
public:
    enum class Type
    {
        MAIN,
        INTRO,
        NAMES,
        SPECIAL_NAMES,
        DEFAULT_NAME,
        ITEM_NAMES,
        MENU,
        END_CREDITS,
        SYSTEM
    };

    StringData(const std::filesystem::path& asm_file);
    StringData(const Rom& rom);

    virtual ~StringData() {}

    virtual bool Save(const std::filesystem::path& dir);
    virtual bool Save();

    virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    std::wstring GetItemDisplayName(int item) const;
    std::wstring GetCharacterDisplayName(int character) const;
    std::wstring GetGlobalCharacterDisplayName(int character) const;

    std::map<std::string, std::shared_ptr<TilesetEntry>> GetAllTilesets() const;
    std::vector<std::shared_ptr<TilesetEntry>> GetFonts() const;

    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> GetAllMaps() const;
    std::vector<std::shared_ptr<Tilemap2DEntry>> GetTextboxMaps() const;

    std::size_t GetStringCount(Type type) const;
    const LSString::StringType& GetString(Type type, std::size_t index) const;
    LSString::StringType GetOrigString(Type type, std::size_t index) const;
    void SetString(Type type, std::size_t index, const LSString::StringType& value);
    void InsertString(Type type, std::size_t index, const LSString::StringType& value);
    void DeleteString(Type type, std::size_t index);
    void SwapStrings(Type type, std::size_t i1, std::size_t i2);
    bool HasStringChanged(Type type, std::size_t index) const;

    std::size_t GetMainStringCount() const;
    const LSString::StringType& GetMainString(std::size_t index) const;
    LSString::StringType GetOrigMainString(std::size_t index) const;
    void SetMainString(std::size_t index, const LSString::StringType& value);
    void InsertMainString(std::size_t index, const LSString::StringType& value);
    bool HasMainStringChanged(std::size_t index) const;

    std::size_t GetSystemStringCount() const;
    const LSString::StringType& GetSystemString(std::size_t index) const;
    LSString::StringType GetOrigSystemString(std::size_t index) const;
    void SetSystemString(std::size_t index, const LSString::StringType& value);
    bool HasSystemStringChanged(std::size_t index) const;

    std::size_t GetCharNameCount() const;
    const LSString::StringType& GetCharName(std::size_t index) const;
    LSString::StringType GetOrigCharName(std::size_t index) const;
    void SetCharName(std::size_t index, const LSString::StringType& value);
    bool HasCharNameChanged(std::size_t index) const;

    std::size_t GetSpecialCharNameCount() const;
    const LSString::StringType& GetSpecialCharName(std::size_t index) const;
    LSString::StringType GetOrigSpecialCharName(std::size_t index) const;
    void SetSpecialCharName(std::size_t index, const LSString::StringType& value);
    bool HasSpecialCharNameChanged(std::size_t index) const;
    uint8_t GetSpecialCharTalkSfx(std::size_t index) const;
    void SetSpecialCharTalkSfx(std::size_t index, uint8_t sound);
    std::string ExportSpecialCharTalkSfxYaml() const;
    void ImportSpecialCharTalkSfxYaml(std::string yaml);

    const LSString::StringType& GetDefaultCharName() const;
    const LSString::StringType& GetOrigDefaultCharName() const;
    void SetDefaultCharName(const LSString::StringType& value);
    bool HasDefaultCharNameChanged() const;

    std::size_t GetItemNameCount() const;
    const LSString::StringType& GetItemName(std::size_t index) const;
    LSString::StringType GetOrigItemName(std::size_t index) const;
    void SetItemName(std::size_t index, const LSString::StringType& value);
    bool HasItemNameChanged(std::size_t index) const;

    std::size_t GetMenuStrCount() const;
    const LSString::StringType& GetMenuStr(std::size_t index) const;
    LSString::StringType GetOrigMenuStr(std::size_t index) const;
    void SetMenuStr(std::size_t index, const LSString::StringType& value);
    bool HasMenuStrChanged(std::size_t index) const;

    std::size_t GetIntroStringCount() const;
    const IntroString& GetIntroString(std::size_t index) const;
    IntroString GetOrigIntroString(std::size_t index) const;
    void SetIntroString(std::size_t index, const IntroString& value);
    bool HasIntroStringChanged(std::size_t index) const;

    std::size_t GetEndCreditStringCount() const;
    const EndCreditString& GetEndCreditString(std::size_t index) const;
    EndCreditString GetOrigEndCreditString(std::size_t index) const;
    void SetEndCreditString(std::size_t index, const EndCreditString& value);
    bool HasEndCreditStringChanged(std::size_t index) const;

    uint16_t GetRoomVisitFlag(uint16_t room) const;
    void SetRoomVisitFlag(uint16_t room, uint16_t flag);
    std::vector<uint16_t> GetRoomCharacters(uint16_t room) const;
    void SetRoomCharacters(uint16_t room, const std::vector<uint16_t>& characters);

    uint8_t GetSaveLocation(uint16_t room);
    void SetSaveLocation(uint16_t room, uint8_t name);
    uint8_t GetMapLocation(uint16_t room);
    uint8_t GetMapPosition(uint16_t room);
    void SetMapLocation(uint16_t room, uint8_t name, uint8_t position);

    uint8_t GetEntityTalkSound(uint8_t entity_id) const;
    void SetEntityTalkSound(uint8_t entity_id, uint8_t sound);
    uint8_t GetCharacterTalkSound(uint8_t character_id) const;
    void SetCharacterTalkSound(uint8_t character_id, uint8_t sound);

protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const std::filesystem::path& dir);
    void InitCache();
    bool DecompressStrings();
    bool CompressStrings();
    bool DecodeStrings(const std::vector<uint8_t>& bytes, std::vector<LSString::StringType>& strings);
    bool DecodeString(const std::vector<uint8_t>& bytes, LSString::StringType& string);
    bool EncodeStrings(const std::vector<LSString::StringType>& strings, std::vector<uint8_t>& bytes);
    bool EncodeString(const LSString::StringType& string, std::vector<uint8_t>& bytes);
    std::map<uint16_t, std::pair<uint8_t, uint8_t>> DeserialiseLocationMap(const std::vector<uint8_t>& bytes);

    std::map<uint8_t, uint8_t> DeserialiseSfxMap(const ByteVector& bytes);
    ByteVector SerialiseSfxMap(const std::map<uint8_t, uint8_t>& map);
    std::vector<uint8_t> SerialiseLocationMap(const std::map<uint16_t, std::pair<uint8_t, uint8_t>>& locs);
    void DeserialiseVisitFlags(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> SerialiseVisitFlags();
    uint32_t GetCharsetSize() const;

    bool AsmLoadSystemFont();
    bool AsmLoadSystemStrings();
    bool AsmLoadCompressedStringData();
    bool AsmLoadHuffmanData();
    bool AsmLoadStringTables();
    bool AsmLoadIntroStrings();
    bool AsmLoadEndCreditStrings();
    bool AsmLoadTalkSfx();
    bool AsmLoadScriptData();

    bool RomLoadSystemFont(const Rom& rom);
    bool RomLoadSystemStrings(const Rom& rom);
    bool RomLoadCompressedStringData(const Rom& rom);
    bool RomLoadHuffmanData(const Rom& rom);
    bool RomLoadStringTables(const Rom& rom);
    bool RomLoadIntroStrings(const Rom& rom);
    bool RomLoadEndCreditStrings(const Rom& rom);
    bool RomLoadTalkSfx(const Rom& rom);
    bool RomLoadScriptData(const Rom& rom);

    bool AsmSaveFonts(const std::filesystem::path& dir);
    bool AsmSaveSystemText(const std::filesystem::path& dir);
    bool AsmSaveCompressedStringData(const std::filesystem::path& dir);
    bool AsmSaveHuffmanData(const std::filesystem::path& dir);
    bool AsmSaveStringTables(const std::filesystem::path& dir);
    bool AsmSaveIntroStrings(const std::filesystem::path& dir);
    bool AsmSaveEndCreditStrings(const std::filesystem::path& dir);
    bool AsmSaveTalkSfx(const std::filesystem::path& dir);
    bool AsmSaveScriptData(const std::filesystem::path& dir);

    bool RomPrepareInjectSystemText(const Rom& rom);
    bool RomPrepareInjectCompressedStringData(const Rom& rom);
    bool RomPrepareInjectHuffmanData(const Rom& rom);
    bool RomPrepareInjectStringTables(const Rom& rom);
    bool RomPrepareInjectIntroStrings(const Rom& rom);
    bool RomPrepareInjectEndCreditStrings(const Rom& rom);
    bool RomPrepareInjectTalkSfx(const Rom& rom);
    bool RomPrepareInjectScriptData(const Rom& rom);

    std::filesystem::path m_region_check_filename;
    std::filesystem::path m_region_check_routine_filename;
    std::filesystem::path m_region_check_strings_filename;
    std::filesystem::path m_system_font_filename;
    std::filesystem::path m_strings_filename;
    std::filesystem::path m_string_ptr_filename;
    std::filesystem::path m_string_filename_path;
    std::filesystem::path m_huffman_offset_path;
    std::filesystem::path m_huffman_table_path;
    std::filesystem::path m_string_table_path;
    std::filesystem::path m_save_loc_path;
    std::filesystem::path m_map_loc_path;
    std::filesystem::path m_char_table_path;
    std::filesystem::path m_schar_table_path;
    std::filesystem::path m_dchar_table_path;
    std::filesystem::path m_item_table_path;
    std::filesystem::path m_menu_table_path;
    std::filesystem::path m_intro_string_data_path;
    std::filesystem::path m_intro_string_ptrtable_path;
    std::vector<std::filesystem::path> m_intro_strings_path;
    std::filesystem::path m_room_visit_flags_path;
    std::filesystem::path m_end_credit_strings_path;
    std::filesystem::path m_char_talk_sfx_path;
    std::filesystem::path m_sprite_talk_sfx_path;
    std::filesystem::path m_room_dialogue_table_path;

    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_by_name_orig;
    std::map<std::string, std::shared_ptr<TilesetEntry>> m_fonts_internal;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps_orig;
    std::map<std::string, std::shared_ptr<Tilemap2DEntry>> m_ui_tilemaps_internal;

    std::vector<std::vector<std::uint8_t>> m_compressed_strings;
    std::vector<std::vector<std::uint8_t>> m_compressed_strings_orig;
    std::vector<LSString::StringType> m_decompressed_strings;
    std::vector<LSString::StringType> m_decompressed_strings_orig;
    std::array<LSString::StringType, 4> m_system_strings;
    std::array<LSString::StringType, 4> m_system_strings_orig;
    std::vector<LSString::StringType> m_character_names;
    std::vector<LSString::StringType> m_character_names_orig;
    std::vector<LSString::StringType> m_special_character_names;
    std::vector<LSString::StringType> m_special_character_names_orig;
    LSString::StringType m_default_character_name;
    LSString::StringType m_default_character_name_orig;
    std::vector<LSString::StringType> m_item_names;
    std::vector<LSString::StringType> m_item_names_orig;
    std::vector<LSString::StringType> m_menu_strings;
    std::vector<LSString::StringType> m_menu_strings_orig;
    std::vector<EndCreditString> m_ending_strings;
    std::vector<EndCreditString> m_ending_strings_orig;
    std::vector<IntroString> m_intro_strings;
    std::vector<IntroString> m_intro_strings_orig;
    std::vector<uint8_t> m_huffman_offsets;
    std::vector<uint8_t> m_huffman_offsets_orig;
    std::vector<uint8_t> m_huffman_tables;
    std::vector<uint8_t> m_huffman_tables_orig;

    std::map<uint16_t, std::pair<uint8_t, uint8_t>> m_island_map_locations;
    std::map<uint16_t, std::pair<uint8_t, uint8_t>> m_island_map_locations_orig;
    std::map<uint16_t, std::pair<uint8_t, uint8_t>> m_save_game_locations;
    std::map<uint16_t, std::pair<uint8_t, uint8_t>> m_save_game_locations_orig;
    std::vector<uint16_t> m_room_visit_flags;
    std::vector<uint16_t> m_room_visit_flags_orig;
    std::vector<uint8_t> m_char_talk_sfx;
    std::vector<uint8_t> m_char_talk_sfx_orig;
    std::map<uint8_t, uint8_t> m_sprite_talk_sfx;
    std::map<uint8_t, uint8_t> m_sprite_talk_sfx_orig;
    RoomDialogueTable m_room_dialogue_table;
    RoomDialogueTable m_room_dialogue_table_orig;

    RomOffsets::Region m_region;
    bool m_has_region_check;
};

#endif // _STRING_DATA_H_
