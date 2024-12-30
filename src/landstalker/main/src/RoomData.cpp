#include <landstalker/main/include/RoomData.h>

#include <set>
#include <cassert>
#include <algorithm>

#include <landstalker/misc/include/Utils.h>
#include <landstalker/main/include/AsmUtils.h>
#include <landstalker/main/include/GameData.h>
#include <landstalker/main/include/RomLabels.h>
#include <landstalker/misc/include/Literals.h>
#include <landstalker/misc/include/Labels.h>

std::map<uint16_t, std::vector<TileSwapFlag>> DecodeGfxSwap(const ByteVector& data)
{
    std::map<uint16_t, std::vector<TileSwapFlag>> flags;
    assert(data.size() % (sizeof(uint16_t) * 2) == 2);
    for (std::size_t i = 0; i < data.size() && data[i] != 0xFF; i += 4)
    {
        auto flag = TileSwapFlag({ data[i], data[i + 1], data[i + 2], data[i + 3] });
        if (flags.find(flag.room) == flags.cend())
        {
            flags.insert({ flag.room, { flag } });
        }
        else
        {
            flags[flag.room].push_back(flag);
        }
    }
    return flags;
}

ByteVector EncodeGfxSwap(const std::map<uint16_t, std::vector<TileSwapFlag>>& data)
{
    ByteVector ret;
    ret.reserve(data.size() * 4 + 2);
    for (const auto& room : data)
    {
        for (const auto& e : room.second)
        {
            auto bytes = e.GetData();
            ret.insert(ret.end(), bytes.cbegin(), bytes.cend());
        }
    }
    ret.push_back(0xFF);
    ret.push_back(0xFF);
    return ret;
}

std::vector<TreeWarpFlag> DecodeTreeWarp(const ByteVector& data)
{
    std::vector<TreeWarpFlag> flags;
    assert(data.size() % TreeWarpFlag::SIZE == 2);
    for (std::size_t i = 0; i < data.size() && data[i] != 0xFF; i += 8)
    {
        std::array<uint8_t, TreeWarpFlag::SIZE> bytes{ 0 };
        std::copy_n(data.begin() + i, TreeWarpFlag::SIZE, bytes.begin());
        flags.push_back(TreeWarpFlag(bytes));
    }
    return flags;
}

ByteVector EncodeTreeWarp(const std::vector<TreeWarpFlag>& data)
{
    ByteVector ret;
    ret.reserve(data.size() * TreeWarpFlag::SIZE + 2);
    for (const auto& e : data)
    {
        auto bytes = e.GetData();
        ret.insert(ret.end(), bytes.cbegin(), bytes.cend());
    }
    ret.push_back(0xFF);
    ret.push_back(0xFF);
    return ret;
}

std::map<uint16_t, uint16_t> DecodeToMap(const ByteVector& data)
{
    std::map<uint16_t, uint16_t> map;
    assert(data.size() % (sizeof(uint16_t) * 2) == 2);
    for (std::size_t i = 0; i < data.size() && data[i] != 0xFF; i += 4)
    {
        uint16_t room = (data[i] << 8) | data[i + 1];
        uint16_t flag = (data[i + 2] << 3) | (data[i + 3] & 0x07);
        map.insert({ room, flag });
    }
    return map;
}

ByteVector EncodeFromMap(const std::map<uint16_t, uint16_t>& data)
{
    ByteVector ret;
    ret.reserve(data.size() * 4 + 2);
    for (const auto& e : data)
    {
        ret.push_back(e.first >> 8);
        ret.push_back(e.first & 0xFF);
        ret.push_back((e.second >> 3) & 0xFF);
        ret.push_back(e.second & 0x07);
    }
    ret.push_back(0xFF);
    ret.push_back(0xFF);
    return ret;
}

std::set<uint16_t> DecodeToSet(const ByteVector& data, bool terminate = true)
{
    std::set<uint16_t> set;
    assert(data.size() % sizeof(uint16_t) == 0);
    for (std::size_t i = 0; i < data.size(); i += 2)
    {
        if (terminate && data[i] == 0xFF)
        {
            break;
        }
        set.insert((data[i] << 8) | data[i + 1]);
    }
    return set;
}

ByteVector EncodeFromSet(const std::set<uint16_t>& data, bool terminate = true)
{
    ByteVector ret;
    ret.reserve(data.size() * 2 + 2);
    for (const auto& e : data)
    {
        ret.push_back(e >> 8);
        ret.push_back(e & 0xFF);
    }
    if (terminate)
    {
        ret.push_back(0xFF);
        ret.push_back(0xFF);
    }
    return ret;
}

RoomData::RoomData(const std::filesystem::path& asm_file)
    : DataManager("Room Data", asm_file)
{
    if (!LoadAsmFilenames())
    {
        throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.string() + '\'');
    }
    if (!AsmLoadRoomTable())
    {
        throw std::runtime_error(std::string("Unable to load room data from \'") + m_room_data_filename.string() + '\'');
    }
    if (!AsmLoadMaps())
    {
        throw std::runtime_error(std::string("Unable to load map data from \'") + m_map_data_filename.string() + '\'');
    }
    if (!AsmLoadWarpData())
    {
        throw std::runtime_error(std::string("Unable to load warp data from \'") + m_warp_data_filename.string() + '\'');
    }
    if (!AsmLoadRoomPalettes())
    {
        throw std::runtime_error(std::string("Unable to load room palette data from \'") + m_palette_data_filename.string() + '\'');
    }
    if (!AsmLoadMiscPaletteData())
    {
        throw std::runtime_error(std::string("Unable to load misc palette data from \'") + asm_file.string() + '\'');
    }
    if (!AsmLoadBlocksetData())
    {
        throw std::runtime_error(std::string("Unable to load blockset data from \'") + m_blockset_data_filename.string() + '\'');
    }
    if (!AsmLoadBlocksetPtrData())
    {
        throw std::runtime_error(std::string("Unable to load blockset pointers from \'") + m_blockset_pri_ptr_filename.string() + '\'');
    }
    if (!AsmLoadAnimatedTilesetData())
    {
        throw std::runtime_error(std::string("Unable to load animated tileset data from \'") + m_tileset_anim_filename.string() + '\'');
    }
    if (!AsmLoadTilesetData())
    {
        throw std::runtime_error(std::string("Unable to load tileset data from \'") + m_tileset_data_filename.string() + '\'');
    }
    if (!AsmLoadChestData())
    {
        throw std::runtime_error(std::string("Unable to load chest data from \'") + m_chest_data_filename.string() + '\'');
    }
    if (!AsmLoadDoorData())
    {
        throw std::runtime_error(std::string("Unable to load door data from \'") + m_door_table_data_filename.string() + '\'');
    }
    if (!AsmLoadGfxSwapData())
    {
        throw std::runtime_error(std::string("Unable to load tile swap data from \'") + m_gfxswap_table_data_filename.string() + '\'');
    }
    if (!AsmLoadMiscData())
    {
        throw std::runtime_error(std::string("Unable to load miscellaneous room data from \'") + asm_file.string() + '\'');
    }
    UpdateTilesetRecommendedPalettes();
    ResetTilesetDefaultPalettes();
}

RoomData::RoomData(const Rom& rom)
    : DataManager("Room Data", rom)
{
    SetDefaultFilenames();
    if (!RomLoadRoomData(rom))
    {
        throw std::runtime_error(std::string("Unable to load room data from ROM"));
    }
    if (!RomLoadWarpData(rom))
    {
        throw std::runtime_error(std::string("Unable to load warp data from ROM"));
    }
    if (!RomLoadRoomPalettes(rom))
    {
        throw std::runtime_error(std::string("Unable to load room palette data from ROM"));
    }
    if (!RomLoadMiscPaletteData(rom))
    {
        throw std::runtime_error(std::string("Unable to load misc palette data from ROM"));
    }
    if (!RomLoadBlocksetData(rom))
    {
        throw std::runtime_error(std::string("Unable to load blockset data from ROM"));
    }
    if (!RomLoadAllTilesetData(rom))
    {
        throw std::runtime_error(std::string("Unable to load tileset data from ROM"));
    }
    if (!RomLoadChestData(rom))
    {
        throw std::runtime_error(std::string("Unable to load chest data from ROM"));
    }
    if (!RomLoadDoorData(rom))
    {
        throw std::runtime_error(std::string("Unable to load door data from ROM"));
    }
    if (!RomLoadGfxSwapData(rom))
    {
        throw std::runtime_error(std::string("Unable to load graphic swap data from ROM"));
    }
    if (!RomLoadMiscData(rom))
    {
        throw std::runtime_error(std::string("Unable to load miscellaneous room data from ROM"));
    }
    UpdateTilesetRecommendedPalettes();
    ResetTilesetDefaultPalettes();
}

bool RoomData::Save(const std::filesystem::path& dir)
{
    std::filesystem::path directory = dir;
    if (std::filesystem::exists(directory) && std::filesystem::is_regular_file(directory))
    {
        directory = directory.parent_path();
    }
    if (!CreateDirectoryStructure(directory))
    {
        throw std::runtime_error(std::string("Unable to create directory structure at \'") + directory.string() + '\'');
    }
    if (!AsmSaveMaps(dir))
    {
        throw std::runtime_error(std::string("Unable to save map file list to \'") + m_map_data_filename.string() + '\'');
    }
    if (!AsmSaveRoomData(dir))
    {
        throw std::runtime_error(std::string("Unable to save room data to \'") + m_room_data_filename.string() + '\'');
    }
    if (!AsmSaveWarpData(dir))
    {
        throw std::runtime_error(std::string("Unable to save warp data to \'") + m_warp_data_filename.string() + '\'');
    }
    if (!AsmSaveRoomPalettes(dir))
    {
        throw std::runtime_error(std::string("Unable to save palette data to \'") + m_palette_data_filename.string() + '\'');
    }
    if (!AsmSaveMiscPaletteData(dir))
    {
        throw std::runtime_error(std::string("Unable to save misc palette data"));
    }
    if (!AsmSaveBlocksetData(dir))
    {
        throw std::runtime_error(std::string("Unable to save blockset data to \'") + m_blockset_data_filename.string() + '\'');
    }
    if (!AsmSaveBlocksetPointerData(dir))
    {
        throw std::runtime_error(std::string("Unable to save blockset pointer data to \'") + m_blockset_pri_ptr_filename.string() + '\'');
    }
    if (!AsmSaveTilesetData(dir))
    {
        throw std::runtime_error(std::string("Unable to save tileset data to \'") + m_tileset_data_filename.string() + '\'');
    }
    if (!AsmSaveTilesetPointerData(dir))
    {
        throw std::runtime_error(std::string("Unable to save tileset pointer data to \'") + m_tileset_ptrtab_filename.string() + '\'');
    }
    if (!AsmSaveAnimatedTilesetData(dir))
    {
        throw std::runtime_error(std::string("Unable to save animated tileset data to \'") + m_tileset_anim_filename.string() + '\'');
    }
    if (!AsmSaveChestData(dir))
    {
        throw std::runtime_error(std::string("Unable to save chest data to \'") + m_chest_data_filename.string() + '\'');
    }
    if (!AsmSaveDoorData(dir))
    {
        throw std::runtime_error(std::string("Unable to save door data to \'") + m_door_table_data_filename.string() + '\'');
    }
    if (!AsmSaveGfxSwapData(dir))
    {
        throw std::runtime_error(std::string("Unable to save tile swap data to \'") + m_gfxswap_table_data_filename.string() + '\'');
    }
    if (!AsmSaveMiscData(dir))
    {
        throw std::runtime_error(std::string("Unable to save miscellaneous room data to \'") + directory.string() + '\'');
    }
    CommitAllChanges();
    return true;
}

bool RoomData::Save()
{
    return Save(GetBasePath());
}

bool RoomData::HasBeenModified() const
{
    auto entry_pred = [](const auto& e) {return e != nullptr && e->HasSavedDataChanged(); };
    auto pair_pred = [](const auto& e) {return e.second != nullptr && e.second->HasSavedDataChanged(); };
    if (std::any_of(m_animated_ts.begin(), m_animated_ts.end(), pair_pred))
    {
        return true;
    }
    if ((m_animated_ts != m_animated_ts_orig) ||
        (m_animated_ts_by_name != m_animated_ts_by_name_orig) ||
        (m_animated_ts_by_ptr != m_animated_ts_by_ptr_orig))
    {
        return true;
    }
    if (std::any_of(m_blocksets.begin(), m_blocksets.end(), pair_pred))
    {
        return true;
    }
    if ((m_blocksets != m_blocksets_orig) ||
        (m_blocksets_by_name != m_blocksets_by_name_orig))
    {
        return true;
    }
    if (std::any_of(m_lava_palette.begin(), m_lava_palette.end(), entry_pred))
    {
        return true;
    }
    if (m_lava_palette != m_lava_palette_orig)
    {
        return true;
    }
    if (std::any_of(m_maps.begin(), m_maps.end(), pair_pred))
    {
        return true;
    }
    if (m_maps != m_maps_orig)
    {
        return true;
    }
    if (std::any_of(m_room_pals.begin(), m_room_pals.end(), entry_pred))
    {
        return true;
    }
    if (m_room_pals != m_room_pals_orig)
    {
        return true;
    }
    if (std::any_of(m_tilesets.begin(), m_tilesets.end(), pair_pred))
    {
        return true;
    }
    if ((m_tilesets != m_tilesets_orig) ||
        (m_tilesets_by_name != m_tilesets_by_name_orig))
    {
        return true;
    }
    if (std::any_of(m_warp_palette.begin(), m_warp_palette.end(), entry_pred))
    {
        return true;
    }
    if (m_warp_palette != m_warp_palette_orig)
    {
        return true;
    }
    if (entry_pred(m_labrynth_lit_palette) == true)
    {
        return true;
    }
    if (entry_pred(m_intro_font) == true)
    {
        return true;
    }
    for (std::size_t i = 0; i < m_roomlist.size(); ++i)
    {
        if (*m_roomlist[i] != *m_roomlist_orig[i])
        {
            return true;
        }
    }
    if (m_warps != m_warps_orig)
    {
        return true;
    }
    if (m_chests != m_chests_orig)
    {
        return true;
    }
    if (m_doors != m_doors_orig)
    {
        return true;
    }
    if (m_gfxswap_flags != m_gfxswap_flags_orig)
    {
        return true;
    }
    if (m_gfxswap_locked_door_flags != m_gfxswap_locked_door_flags_orig)
    {
        return true;
    }
    if (m_gfxswap_big_tree_flags != m_gfxswap_big_tree_flags_orig)
    {
        return true;
    }
    if (m_gfxswaps != m_gfxswaps_orig)
    {
        return true;
    }
    if (m_shop_list != m_shop_list_orig)
    {
        return true;
    }
    if (m_lifestock_sold_flags != m_lifestock_sold_flags_orig)
    {
        return true;
    }
    if (m_big_tree_list != m_big_tree_list_orig)
    {
        return true;
    }
    if (m_lantern_flag_list != m_lantern_flag_list_orig)
    {
        return true;
    }
    
    return false;
}

void RoomData::RefreshPendingWrites(const Rom& rom)
{
    DataManager::RefreshPendingWrites(rom);
    if (!RomPrepareInjectMiscWarp(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare misc warps for ROM injection"));
    }
    if (!RomPrepareInjectRoomData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare room data for ROM injection"));
    }
    if (!RomPrepareInjectMiscPaletteData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare misc palette data for ROM injection"));
    }
    if (!RomPrepareInjectBlocksetData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare blockset data for ROM injection"));
    }
    if (!RomPrepareInjectAnimatedTilesetData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare animated tileset data for ROM injection"));
    }
    if (!RomPrepareInjectTilesetData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare tileset data for ROM injection"));
    }
    if (!RomPrepareInjectChestData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare chest data for ROM injection"));
    }
    if (!RomPrepareInjectDoorData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare door data for ROM injection"));
    }
    if (!RomPrepareInjectGfxSwapData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare tile swap data for ROM injection"));
    }
    if (!RomPrepareInjectMiscData(rom))
    {
        throw std::runtime_error(std::string("Unable to prepare miscellaneous room data for ROM injection"));
    }
}

std::wstring RoomData::GetTilesetDisplayName(uint8_t index) const
{
    return Labels::Get(Labels::C_TILESETS, index).value_or(StrWPrintf(RomLabels::Tilesets::LABEL_FORMAT_STRING, index + 1));
}

std::wstring RoomData::GetAnimatedTilesetDisplayName(uint8_t tileset, uint8_t index) const
{
    return Labels::Get(Labels::C_ANIM_TILESETS, (tileset << 8) | index).value_or(StrWPrintf(RomLabels::Tilesets::ANIM_LABEL_FORMAT_STRING, tileset + 1, index));
}

std::wstring RoomData::GetRoomPaletteDisplayName(uint8_t index) const
{
    return Labels::Get(Labels::C_ROOM_PALETTES, index).value_or(StrWPrintf(RomLabels::Rooms::ROOM_PAL_NAME, index + 1));
}

std::wstring RoomData::GetBlocksetDisplayName(uint8_t tileset, uint8_t pri, uint8_t sec) const
{
    return Labels::Get(Labels::C_ANIM_TILESETS, (tileset << 16) | (pri << 8) | sec).value_or(StrWPrintf(RomLabels::Blocksets::BLOCKSET_LABEL, tileset + (pri << 5), sec));
}

std::wstring RoomData::GetRoomDisplayName(uint16_t room) const
{
    return m_roomlist.at(room)->GetDisplayName();
}

std::wstring RoomData::GetMapDisplayName(const std::string& map) const
{
    int index = std::distance(m_maps.cbegin(), m_maps.find(map));
    return Labels::Get(Labels::C_MAPS, index).value_or(std::wstring(map.cbegin(), map.cend()));
}

std::vector<std::shared_ptr<TilesetEntry>> RoomData::GetTilesets() const
{
    std::vector<std::shared_ptr<TilesetEntry>> retval;
    for (const auto& t : m_tilesets)
    {
        retval.push_back(t.second);
    }
    return retval;
}

std::vector<std::shared_ptr<AnimatedTilesetEntry>> RoomData::GetAnimatedTilesets(const std::string& tileset) const
{
    std::vector<std::shared_ptr<AnimatedTilesetEntry>> retval;
    auto it = m_tilesets_by_name.find(tileset);
    if (it != m_tilesets_by_name.cend())
    {
        uint8_t idx = static_cast<uint8_t>(it->second->GetIndex());
        for (const auto& ts : m_animated_ts)
        {
            if (ts.first.first == idx)
            {
                retval.push_back(ts.second);
            }
        }
    }
    return retval;
}

bool RoomData::HasAnimatedTilesets(const std::string& tileset) const
{
    auto it = m_tilesets_by_name.find(tileset);
    if (it != m_tilesets_by_name.cend())
    {
        uint8_t idx = static_cast<uint8_t>(it->second->GetIndex());
        for (const auto& ts : m_animated_ts)
        {
            if (ts.first.first == idx)
            {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<TilesetEntry> RoomData::GetTileset(uint8_t index) const
{
    assert(m_tilesets.find(index) != m_tilesets.cend());
    return m_tilesets.find(index)->second;
}

std::shared_ptr<TilesetEntry> RoomData::GetTileset(const std::string& name) const
{
    assert(m_tilesets_by_name.find(name) != m_tilesets_by_name.cend());
    return m_tilesets_by_name.find(name)->second;
}

std::map<std::string, std::shared_ptr<TilesetEntry>> RoomData::GetAllTilesets() const
{
    std::map<std::string, std::shared_ptr<TilesetEntry>> result;
    for (auto& t : m_tilesets_by_name)
    {
        result.insert(t);
    }
    result.insert({ m_intro_font->GetName(), m_intro_font });
    return result;
}

std::shared_ptr<AnimatedTilesetEntry> RoomData::GetAnimatedTileset(uint8_t tileset, uint8_t idx) const
{
    assert(m_animated_ts.find({ tileset, idx }) != m_animated_ts.cend());
    return m_animated_ts.find({ tileset, idx })->second;
}

std::shared_ptr<AnimatedTilesetEntry> RoomData::GetAnimatedTileset(const std::string& name) const
{
    assert(m_animated_ts_by_name.find(name) != m_animated_ts_by_name.cend());
    return m_animated_ts_by_name.find(name)->second;
}

std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> RoomData::GetAllAnimatedTilesets() const
{
    std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>> result;
    for (auto& t : m_animated_ts_by_name)
    {
        result.insert(t);
    }
    return result;
}

std::shared_ptr<TilesetEntry> RoomData::GetIntroFont() const
{
    return m_intro_font;
}

std::shared_ptr<PaletteEntry> RoomData::GetRoomPalette(const std::string& name) const
{
    assert(m_room_pals_by_name.find(name) != m_room_pals_by_name.cend());
    return m_room_pals_by_name.find(name)->second;
}

std::shared_ptr<PaletteEntry> RoomData::GetRoomPalette(uint8_t index) const
{
    assert(index < m_room_pals.size());
    return m_room_pals.at(index);
}

const std::vector<std::shared_ptr<PaletteEntry>>& RoomData::GetRoomPalettes() const
{
    return m_room_pals;
}

std::vector<std::shared_ptr<PaletteEntry>> RoomData::GetMiscPalette(const MiscPaletteType& type) const
{
    switch (type)
    {
    case MiscPaletteType::LANTERN:
        return std::vector<std::shared_ptr<PaletteEntry>>({ m_labrynth_lit_palette });
    case MiscPaletteType::LAVA:
        return m_lava_palette;
    case MiscPaletteType::WARP:
        return m_warp_palette;
    default:
        return std::vector<std::shared_ptr<PaletteEntry>>();
    }
}

std::map<std::string, std::shared_ptr<PaletteEntry>> RoomData::GetAllPalettes() const
{
    std::map<std::string, std::shared_ptr<PaletteEntry>> result;
    for (auto& p : m_room_pals_by_name)
    {
        result.insert(p);
    }
    for (auto& p : m_lava_palette)
    {
        result.insert({p->GetName(), p});
    }
    for (auto& p : m_warp_palette)
    {
        result.insert({p->GetName(), p});
    }
    result.insert({ m_labrynth_lit_palette->GetName(), m_labrynth_lit_palette });
    return result;
}

std::shared_ptr<PaletteEntry> RoomData::GetDefaultTilesetPalette(const std::string& name) const
{
    auto p = m_tileset_defaultpal.find(name);
    if (p != m_tileset_defaultpal.cend())
    {
        return p->second;
    }
    else
    {
        return m_room_pals.front();
    }
}

std::shared_ptr<PaletteEntry> RoomData::GetDefaultTilesetPalette(uint8_t index) const
{
    return GetDefaultTilesetPalette(GetTileset(index)->GetName());
}

std::list<std::shared_ptr<PaletteEntry>> RoomData::GetTilesetRecommendedPalettes(const std::string& name) const
{
    auto p = m_tileset_pals.find(name);
    if (p != m_tileset_pals.cend())
    {
        return p->second;
    }
    else
    {
        return std::list<std::shared_ptr<PaletteEntry>>(m_room_pals.cbegin(), m_room_pals.cend());
    }
}

std::list<std::shared_ptr<PaletteEntry>> RoomData::GetTilesetRecommendedPalettes(uint8_t index) const
{
    return GetTilesetRecommendedPalettes(GetTileset(index)->GetName());
}

std::vector<std::shared_ptr<BlocksetEntry>> RoomData::GetBlocksetList(const std::string& tileset) const
{
    std::vector<std::shared_ptr<BlocksetEntry>> retval;
    for (const auto& bs : m_blocksets)
    {
        if (bs.second->GetTileset() == GetTileset(tileset)->GetIndex())
        {
            retval.push_back(bs.second);
        }
    }
    return retval;
}

std::map<std::string, std::shared_ptr<BlocksetEntry>> RoomData::GetAllBlocksets() const
{
    std::map<std::string, std::shared_ptr<BlocksetEntry>> result;
    for (const auto& b : m_blocksets_by_name)
    {
        result.insert(b);
    }
    return result;
}

std::shared_ptr<BlocksetEntry> RoomData::GetBlockset(const std::string& name) const
{
    assert(m_blocksets_by_name.find(name) != m_blocksets_by_name.cend());
    return m_blocksets_by_name.find(name)->second;
}

std::shared_ptr<BlocksetEntry> RoomData::GetBlockset(uint8_t pri, uint8_t sec) const
{
    if (m_blocksets.find({ pri, sec }) == m_blocksets.cend())
    {
        return nullptr;
    }
    return m_blocksets.find({ pri, sec })->second;
}

std::shared_ptr<BlocksetEntry> RoomData::GetBlockset(uint8_t tileset, uint8_t pri, uint8_t sec) const
{
    return GetBlockset(pri << 5 | tileset, sec);
}

std::shared_ptr<BlocksetEntry> RoomData::GetBlockset(const std::string& tileset, uint8_t pri, uint8_t sec) const
{
    return GetBlockset(static_cast<uint8_t>(pri << 5 | GetTileset(tileset)->GetIndex()), sec);
}

const std::vector<std::shared_ptr<Room>>& RoomData::GetRoomlist() const
{
    return m_roomlist;
}

std::size_t RoomData::GetRoomCount() const
{
    return m_roomlist.size();
}

std::shared_ptr<Room> RoomData::GetRoom(uint16_t index) const
{
    assert(index < m_roomlist.size());
    return m_roomlist.at(index);
}

std::shared_ptr<Room> RoomData::GetRoom(const std::string& name) const
{
    assert(m_roomlist_by_name.find(name) != m_roomlist_by_name.cend());
    return m_roomlist_by_name.find(name)->second;
}

const std::map<std::string, std::shared_ptr<Tilemap3DEntry>>& RoomData::GetMaps() const
{
    return m_maps;
}

std::shared_ptr<Tilemap3DEntry> RoomData::GetMap(const std::string& name) const
{
    assert(m_maps.find(name) != m_maps.cend());
    return m_maps.find(name)->second;
}

std::shared_ptr<PaletteEntry> RoomData::GetPaletteForRoom(const std::string& name) const
{
    auto rm = GetRoom(name);
    return GetRoomPalette(rm->room_palette);
}

std::shared_ptr<PaletteEntry> RoomData::GetPaletteForRoom(uint16_t roomnum) const
{
    auto rm = GetRoom(roomnum);
    return GetRoomPalette(rm->room_palette);
}

std::shared_ptr<TilesetEntry> RoomData::GetTilesetForRoom(const std::string& name) const
{
    auto rm = GetRoom(name);
    return GetTileset(rm->tileset);
}

std::shared_ptr<TilesetEntry> RoomData::GetTilesetForRoom(uint16_t roomnum) const
{
    auto rm = GetRoom(roomnum);
    return GetTileset(rm->tileset);
}

std::list<std::shared_ptr<BlocksetEntry>> RoomData::GetBlocksetsForRoom(const std::string& name) const
{
    std::list<std::shared_ptr<BlocksetEntry>> retval;
    auto rm = GetRoom(name);
    retval.push_back(GetBlockset(rm->GetBlocksetId(), 0));
    retval.push_back(GetBlockset(rm->GetBlocksetId(), rm->sec_blockset + 1));
    return retval;
}

std::list<std::shared_ptr<BlocksetEntry>> RoomData::GetBlocksetsForRoom(uint16_t roomnum) const
{
    auto rm = GetRoom(roomnum);
    return GetBlocksetsForRoom(rm->name);
}

std::shared_ptr<Blockset> RoomData::GetCombinedBlocksetForRoom(const std::string& name) const
{
    auto blockset = std::make_shared<Blockset>();
    for (const auto& b : GetBlocksetsForRoom(name))
    {
        blockset->insert(blockset->end(), b->GetData()->cbegin(), b->GetData()->cend());
    }
    return blockset;
}

std::shared_ptr<Blockset> RoomData::GetCombinedBlocksetForRoom(uint16_t roomnum) const
{
    auto blockset = std::make_shared<Blockset>();
    for (const auto& b : GetBlocksetsForRoom(roomnum))
    {
        blockset->insert(blockset->end(), b->GetData()->cbegin(), b->GetData()->cend());
    }
    return blockset;
}

std::shared_ptr<Tilemap3DEntry> RoomData::GetMapForRoom(const std::string& name) const
{
    auto rm = GetRoom(name);
    return GetMap(rm->map);
}

std::shared_ptr<Tilemap3DEntry> RoomData::GetMapForRoom(uint16_t roomnum) const
{
    auto rm = GetRoom(roomnum);
    return GetMap(rm->map);
}

std::vector<uint8_t> RoomData::GetChestsForRoom(uint16_t roomnum) const
{
    return m_chests.GetChestsForRoom(roomnum);
}

void RoomData::SetChestsForRoom(uint16_t roomnum, const std::vector<uint8_t>& chests)
{
    m_chests.SetRoomChests(roomnum, chests);
}

bool RoomData::GetNoChestFlagForRoom(uint16_t roomnum) const
{
    return m_chests.RoomHasNoChestsSet(roomnum);
}

void RoomData::SetNoChestFlagForRoom(uint16_t roomnum, bool flag)
{
    if (flag)
    {
        m_chests.SetRoomNoChestsFlag(roomnum);
    }
    else
    {
        m_chests.ClearRoomNoChestsFlag(roomnum);
    }
}

bool RoomData::CleanupChests(const GameData& g)
{
    return m_chests.CleanupRoomChests(g);
}

std::vector<WarpList::Warp> RoomData::GetWarpsForRoom(uint16_t roomnum)
{
    return m_warps.GetWarpsForRoom(roomnum);
}

void RoomData::SetWarpsForRoom(uint16_t roomnum, const std::vector<WarpList::Warp>& warps)
{
    m_warps.UpdateWarpsForRoom(roomnum, warps);
}

bool RoomData::HasFallDestination(uint16_t room) const
{
    return m_warps.HasFallDestination(room);
}

uint16_t RoomData::GetFallDestination(uint16_t room) const
{
    return m_warps.GetFallDestination(room);
}

void RoomData::SetHasFallDestination(uint16_t room, bool enabled)
{
    m_warps.SetHasFallDestination(room, enabled);
}

void RoomData::SetFallDestination(uint16_t room, uint16_t dest)
{
    m_warps.SetFallDestination(room, dest);
}

bool RoomData::HasClimbDestination(uint16_t room) const
{
    return m_warps.HasClimbDestination(room);
}

uint16_t RoomData::GetClimbDestination(uint16_t room) const
{
    return m_warps.GetClimbDestination(room);
}

void RoomData::SetHasClimbDestination(uint16_t room, bool enabled)
{
    m_warps.SetHasClimbDestination(room, enabled);
}

void RoomData::SetClimbDestination(uint16_t room, uint16_t dest)
{
    return m_warps.SetClimbDestination(room, dest);
}

std::vector<WarpList::Transition> RoomData::GetTransitions(uint16_t room) const
{
    return m_warps.GetAllTransitionsForRoom(room);
}

std::vector<WarpList::Transition> RoomData::GetSrcTransitions(uint16_t room) const
{
    return m_warps.GetSrcTransitionsForRoom(room);
}

void RoomData::SetSrcTransitions(uint16_t room, const std::vector<WarpList::Transition>& data)
{
    m_warps.SetSrcTransitionsForRoom(room, data);
}

bool RoomData::IsShop(uint16_t room) const
{
    return m_shop_list.count(room) > 0;
}

void RoomData::SetShop(uint16_t room, bool is_shop)
{
    if (is_shop && !IsShop(room))
    {
        m_shop_list.insert(room);
    }
    else if (!is_shop && IsShop(room))
    {
        m_shop_list.erase(room);
    }
}

bool RoomData::IsTree(uint16_t room) const
{
    return m_big_tree_list.count(room) > 0;
}

void RoomData::SetTree(uint16_t room, bool is_tree)
{
    if (is_tree && !IsTree(room))
    {
        m_big_tree_list.insert(room);
    }
    else if (!is_tree && IsTree(room))
    {
        m_big_tree_list.erase(room);
    }
}

bool RoomData::HasLifestockSaleFlag(uint16_t room) const
{
    return m_lifestock_sold_flags.count(room) > 0;
}

uint16_t RoomData::GetLifestockSaleFlag(uint16_t room) const
{
    if (HasLifestockSaleFlag(room))
    {
        return m_lifestock_sold_flags.at(room);
    }
    else
    {
        return 0xFFFF;
    }
}

void RoomData::SetLifestockSaleFlag(uint16_t room, uint16_t flag)
{
    if (flag != 0xFFFF && !HasLifestockSaleFlag(room))
    {
        m_lifestock_sold_flags.insert({ room, flag });
    }
    else if (flag == 0xFFFF && HasLifestockSaleFlag(room))
    {
        m_lifestock_sold_flags.erase(room);
    }
}

void RoomData::ClearLifestockSaleFlag(uint16_t room)
{
    SetLifestockSaleFlag(room, 0xFFFF);
}

bool RoomData::HasLanternFlag(uint16_t room) const
{
    return m_lantern_flag_list.count(room) > 0;
}

uint16_t RoomData::GetLanternFlag(uint16_t room) const
{
    if (HasLanternFlag(room))
    {
        return m_lantern_flag_list.at(room);
    }
    else
    {
        return 0xFFFF;
    }
}

void RoomData::SetLanternFlag(uint16_t room, uint16_t flag)
{
    if (flag != 0xFFFF && !HasLanternFlag(room))
    {
        m_lantern_flag_list.insert({ room, flag });
    }
    else if (flag == 0xFFFF && HasLanternFlag(room))
    {
        m_lantern_flag_list.erase(room);
    }
}

void RoomData::ClearLanternFlag(uint16_t room)
{
    SetLanternFlag(room, 0xFFFF);
}

bool RoomData::HasTreeWarpFlag(uint16_t room) const
{
    return std::any_of(m_gfxswap_big_tree_flags.cbegin(), m_gfxswap_big_tree_flags.cend(), [&](const auto& w) {
        return w.room1 == room || w.room2 == room;
        });
}

TreeWarpFlag RoomData::GetTreeWarp(uint16_t room) const
{
    auto result = std::find_if(m_gfxswap_big_tree_flags.cbegin(), m_gfxswap_big_tree_flags.cend(), [&](const auto& w) {
        return w.room1 == room || w.room2 == room;
        });
    if (result == m_gfxswap_big_tree_flags.cend())
    {
        return { 0_u16, 0_u16, 0_u16 };
    }
    return {room, result->room1 == room ? result->room2 : result->room1, result->flag};
}

void RoomData::SetTreeWarp(const TreeWarpFlag& flags)
{
    TreeWarpFlag new_flags = flags;
    if (flags.room2 < flags.room1)
    {
        new_flags = { flags.room2, flags.room1, flags.flag };
    }
    auto result = std::find_if(m_gfxswap_big_tree_flags.begin(), m_gfxswap_big_tree_flags.end(), [&](const auto& w) {
        return w.room1 == new_flags.room1 || w.room2 == new_flags.room1 || w.room1 == new_flags.room2 || w.room2 == new_flags.room2;
        });
    if (result == m_gfxswap_big_tree_flags.cend())
    {
        m_gfxswap_big_tree_flags.push_back(new_flags);
    }
    else
    {
        *result = new_flags;
    }
}

void RoomData::ClearTreeWarp(uint16_t room)
{
    m_gfxswap_big_tree_flags.erase(std::remove_if(m_gfxswap_big_tree_flags.begin(), m_gfxswap_big_tree_flags.end(), [&](const auto& w) {
        return w.room1 == room || w.room2 == room;
        }), m_gfxswap_big_tree_flags.end());
}

bool RoomData::HasNormalTileSwaps(uint16_t room) const
{
    return m_gfxswap_flags.find(room) != m_gfxswap_flags.cend();
}

std::vector<TileSwapFlag> RoomData::GetNormalTileSwaps(uint16_t room) const
{
    if (m_gfxswap_flags.find(room) != m_gfxswap_flags.cend())
    {
        return m_gfxswap_flags.at(room);
    }
    return std::vector<TileSwapFlag>();
}

void RoomData::SetNormalTileSwaps(uint16_t room, const std::vector<TileSwapFlag>& swaps)
{
    if (swaps.empty() == false)
    {
        m_gfxswap_flags[room] = swaps;
    }
    else
    {
        m_gfxswap_flags.erase(room);
    }
}

bool RoomData::HasLockedDoorTileSwaps(uint16_t room) const
{
    return m_gfxswap_locked_door_flags.find(room) != m_gfxswap_locked_door_flags.cend();
}

std::vector<TileSwapFlag> RoomData::GetLockedDoorTileSwaps(uint16_t room) const
{
    if (m_gfxswap_locked_door_flags.find(room) != m_gfxswap_locked_door_flags.cend())
    {
        return m_gfxswap_locked_door_flags.at(room);
    }
    return std::vector<TileSwapFlag>();
}

void RoomData::SetLockedDoorTileSwaps(uint16_t room, const std::vector<TileSwapFlag>& swaps)
{
    if (swaps.empty() == false)
    {
        m_gfxswap_locked_door_flags[room] = swaps;
    }
    else
    {
        m_gfxswap_locked_door_flags.erase(room);
    }
}

bool RoomData::HasTileSwaps(uint16_t room) const
{
    return m_gfxswaps.RoomHasSwaps(room);
}

std::vector<TileSwap> RoomData::GetTileSwaps(uint16_t room) const
{
    return m_gfxswaps.GetSwapsForRoom(room);
}

void RoomData::SetTileSwaps(uint16_t room, const std::vector<TileSwap>& swaps)
{
    m_gfxswaps.SetRoomSwaps(room, swaps);
}

bool RoomData::HasDoors(uint16_t room) const
{
    return m_doors.RoomHasDoors(room);
}

std::vector<Door> RoomData::GetDoors(uint16_t room) const
{
    return m_doors.GetDoorsForRoom(room);
}

void RoomData::SetDoors(uint16_t room, const std::vector<Door>& doors)
{
    m_doors.SetRoomDoors(room, doors);
}

void RoomData::CommitAllChanges()
{
    auto entry_commit = [](const auto& e) {return e->Commit(); };
    auto pair_commit = [](const auto& e) {return e.second->Commit(); };
    std::for_each(m_animated_ts.begin(), m_animated_ts.end(), pair_commit);
    std::for_each(m_blocksets.begin(), m_blocksets.end(), pair_commit);
    std::for_each(m_lava_palette.begin(), m_lava_palette.end(), entry_commit);
    std::for_each(m_maps.begin(), m_maps.end(), pair_commit);
    std::for_each(m_room_pals.begin(), m_room_pals.end(), entry_commit);
    std::for_each(m_tilesets.begin(), m_tilesets.end(), pair_commit);
    std::for_each(m_warp_palette.begin(), m_warp_palette.end(), entry_commit);
    entry_commit(m_labrynth_lit_palette);
    m_roomlist_orig.clear();
    for (std::size_t i = 0; i < m_roomlist.size(); ++i)
    {
        m_roomlist_orig.push_back(std::make_shared<Room>(*m_roomlist[i]));
    }
    m_warps_orig = m_warps;
    m_animated_ts_orig = m_animated_ts;
    m_animated_ts_by_name_orig = m_animated_ts_by_name;
    m_animated_ts_by_ptr_orig = m_animated_ts_by_ptr;
    m_blocksets_orig = m_blocksets;
    m_lava_palette_orig = m_lava_palette;
    m_maps_orig = m_maps;
    m_room_pals_orig = m_room_pals;
    m_tilesets_orig = m_tilesets;
    m_warp_palette_orig = m_warp_palette;
    m_chests_orig = m_chests;
    m_doors_orig = m_doors;
    m_gfxswap_flags_orig = m_gfxswap_flags;
    m_gfxswap_locked_door_flags_orig = m_gfxswap_locked_door_flags;
    m_gfxswap_big_tree_flags_orig = m_gfxswap_big_tree_flags;
    m_gfxswaps_orig = m_gfxswaps;
    m_shop_list_orig = m_shop_list;
    m_lifestock_sold_flags_orig = m_lifestock_sold_flags;
    m_big_tree_list_orig = m_big_tree_list;
    m_lantern_flag_list_orig = m_lantern_flag_list;
    m_pending_writes.clear();
}

bool RoomData::LoadAsmFilenames()
{
    try
    {
        bool retval = true;
        AsmFile f(GetAsmFilename().string());
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::ROOM_DATA, m_room_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::MAP_DATA, m_map_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::ROOM_EXITS, m_warp_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::ROOM_FALL_DEST, m_fall_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::ROOM_CLIMB_DEST, m_climb_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::ROOM_TRANSITIONS, m_transition_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::ROOM_PALS, m_palette_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::PALETTE_WARP, m_warp_pal_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::PALETTE_LAVA, m_lava_pal_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::PALETTE_LANTERN, m_lantern_pal_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Tilesets::DATA_LOC, m_tileset_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Tilesets::PTRTAB_LOC, m_tileset_ptrtab_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Tilesets::ANIM_DATA_LOC, m_tileset_anim_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Blocksets::PRI_PTRS, m_blockset_pri_ptr_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Blocksets::SEC_PTRS, m_blockset_sec_ptr_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Blocksets::DATA, m_blockset_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::CHEST_OFFSETS, m_chest_offset_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::CHEST_CONTENTS, m_chest_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::DOOR_OFFSET_TABLE, m_door_offset_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::DOOR_TABLE, m_door_table_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::GFX_SWAP_FLAGS, m_gfxswap_flag_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::GFX_SWAP_LOCKED_DOOR_FLAGS_LEA1, m_gfxswap_locked_door_flag_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::GFX_SWAP_TREE_FLAGS, m_gfxswap_big_tree_flag_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::GFX_SWAP_TABLE, m_gfxswap_table_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::SHOP_LIST_LEA1, m_shop_table_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::LIFESTOCK_SOLD_FLAGS, m_lifestock_sold_flag_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::BIG_TREE_LOCATIONS, m_bigtree_data_filename);
        retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::LANTERN_ROOM_FLAGS, m_lantern_flag_data_filename);
        return retval;
    }
    catch (...)
    {
    }
    return false;
}

void RoomData::SetDefaultFilenames()
{
    if (m_room_data_filename.empty())                     m_room_data_filename                     = RomLabels::Rooms::ROOM_DATA_FILE;
    if (m_map_data_filename.empty())                      m_map_data_filename                      = RomLabels::Rooms::MAP_DATA_FILE;
    if (m_warp_data_filename.empty())                     m_warp_data_filename                     = RomLabels::Rooms::WARP_FILENAME;
    if (m_fall_data_filename.empty())                     m_fall_data_filename                     = RomLabels::Rooms::FALL_DEST_FILENAME;
    if (m_climb_data_filename.empty())                    m_climb_data_filename                    = RomLabels::Rooms::CLIMB_DEST_FILENAME;
    if (m_transition_data_filename.empty())               m_transition_data_filename               = RomLabels::Rooms::TRANSITION_FILENAME;
    if (m_palette_data_filename.empty())                  m_palette_data_filename                  = RomLabels::Rooms::PALETTE_DATA_FILE;
    if (m_lava_pal_data_filename.empty())                 m_lava_pal_data_filename                 = RomLabels::Rooms::PALETTE_LAVA_FILENAME;
    if (m_warp_pal_data_filename.empty())                 m_warp_pal_data_filename                 = RomLabels::Rooms::PALETTE_WARP_FILENAME;
    if (m_lantern_pal_data_filename.empty())              m_lantern_pal_data_filename              = RomLabels::Rooms::PALETTE_LANTERN_FILENAME;
    if (m_tileset_data_filename.empty())                  m_tileset_data_filename                  = RomLabels::Tilesets::INCLUDE_FILE;
    if (m_tileset_ptrtab_filename.empty())                m_tileset_ptrtab_filename                = RomLabels::Tilesets::PTRTAB_FILE;
    if (m_tileset_anim_filename.empty())                  m_tileset_anim_filename                  = RomLabels::Tilesets::ANIM_FILE;
    if (m_blockset_pri_ptr_filename.empty())              m_blockset_pri_ptr_filename              = RomLabels::Blocksets::PRI_PTR_FILE;
    if (m_blockset_sec_ptr_filename.empty())              m_blockset_sec_ptr_filename              = RomLabels::Blocksets::SEC_PTR_FILE;
    if (m_blockset_data_filename.empty())                 m_blockset_data_filename                 = RomLabels::Blocksets::DATA_FILE;
    if (m_chest_data_filename.empty())                    m_chest_data_filename                    = RomLabels::Rooms::CHEST_CONTENTS_FILENAME;
    if (m_chest_offset_data_filename.empty())             m_chest_offset_data_filename             = RomLabels::Rooms::CHEST_OFFSETS_FILENAME;
    if (m_door_offset_data_filename.empty())              m_door_offset_data_filename              = RomLabels::Rooms::DOOR_OFFSET_TABLE_FILE;
    if (m_door_table_data_filename.empty())               m_door_table_data_filename               = RomLabels::Rooms::DOOR_TABLE_FILE;
    if (m_gfxswap_flag_data_filename.empty())             m_gfxswap_flag_data_filename             = RomLabels::Rooms::GFX_SWAP_FLAGS_FILE;
    if (m_gfxswap_locked_door_flag_data_filename.empty()) m_gfxswap_locked_door_flag_data_filename = RomLabels::Rooms::GFX_SWAP_LOCKED_DOOR_FLAGS_FILE;
    if (m_gfxswap_big_tree_flag_data_filename.empty())    m_gfxswap_big_tree_flag_data_filename    = RomLabels::Rooms::GFX_SWAP_TREE_FLAGS_FILE;
    if (m_gfxswap_table_data_filename.empty())            m_gfxswap_table_data_filename            = RomLabels::Rooms::GFX_SWAP_TABLE_FILE;
    if (m_shop_table_data_filename.empty())               m_shop_table_data_filename               = RomLabels::Rooms::SHOP_LIST_FILE;
    if (m_lifestock_sold_flag_data_filename.empty())      m_lifestock_sold_flag_data_filename      = RomLabels::Rooms::LIFESTOCK_SOLD_FLAGS_FILE;
    if (m_bigtree_data_filename.empty())                  m_bigtree_data_filename                  = RomLabels::Rooms::BIG_TREE_LOCATIONS_FILE;
    if (m_lantern_flag_data_filename.empty())             m_lantern_flag_data_filename             = RomLabels::Rooms::LANTERN_ROOM_FILE;
}

bool RoomData::CreateDirectoryStructure(const std::filesystem::path& dir)
{
    bool retval = true;

    retval = retval && CreateDirectoryTree(dir / m_room_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_map_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_warp_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_fall_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_climb_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_transition_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_palette_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_lava_pal_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_warp_pal_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_lantern_pal_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_tileset_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_tileset_ptrtab_filename);
    retval = retval && CreateDirectoryTree(dir / m_tileset_anim_filename);
    retval = retval && CreateDirectoryTree(dir / m_blockset_pri_ptr_filename);
    retval = retval && CreateDirectoryTree(dir / m_blockset_sec_ptr_filename);
    retval = retval && CreateDirectoryTree(dir / m_blockset_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_chest_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_chest_offset_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_door_offset_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_door_table_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_gfxswap_flag_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_gfxswap_locked_door_flag_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_gfxswap_big_tree_flag_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_gfxswap_table_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_shop_table_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_lifestock_sold_flag_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_bigtree_data_filename);
    retval = retval && CreateDirectoryTree(dir / m_lantern_flag_data_filename);

    for (const auto& m : m_maps)
    {
        retval = retval && CreateDirectoryTree(dir / m.second->GetFilename());
    }
    return retval;
}

bool RoomData::AsmLoadRoomTable()
{
    try
    {
        AsmFile file(GetBasePath() / m_room_data_filename);
        std::string map;
        uint8_t params[4];
        uint16_t index = 0;
        while (file.IsGood())
        {
            std::string name = StrPrintf("Room%03d", index);
            if (file.IsLabel())
            {
                AsmFile::Label l;
                file >> l;
                name = l;
            }
            file >> map >> params;
            auto room = std::make_shared<Room>(name, map, index, params);
            m_roomlist.push_back(room);
            m_roomlist_by_name.insert({ name, room });
            m_roomlist_orig.push_back(std::make_shared<Room>(*room));
            index++;
        }
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmLoadMaps()
{
    try
    {
        AsmFile file(GetBasePath() / m_map_data_filename);
        AsmFile::IncludeFile inc;
        AsmFile::Label lbl;
        while (file.IsGood())
        {
            file >> lbl >> inc;
            auto mapfile = GetBasePath() / inc.path;
            auto raw_data = std::make_shared<std::vector<uint8_t>>(ReadBytes(mapfile));
            auto map_entry = Tilemap3DEntry::Create(this, ReadBytes(mapfile), lbl, inc.path);
            m_maps[lbl] = map_entry;
        }
        m_maps_orig = m_maps;
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmLoadRoomPalettes()
{
    try
    {
        AsmFile file(GetBasePath() / m_palette_data_filename);
        AsmFile::IncludeFile inc;
        unsigned int i = 0;
        while (file.IsGood())
        {
            std::string name = StrPrintf(RomLabels::Rooms::ROOM_PAL_NAME, i + 1);
            if (file.IsLabel())
            {
                AsmFile::Label l;
                file >> l;
                name = l;
            }
            file >> inc;
            auto palfile = GetBasePath() / inc.path;
            auto e = PaletteEntry::Create(this, ReadBytes(palfile), name, inc.path, Palette::Type::ROOM);
            m_room_pals.push_back(e);
            m_room_pals_by_name.insert({ name, e });
            i++;
        }
        m_room_pals_orig = m_room_pals;
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmLoadWarpData()
{
    m_warps = WarpList(GetBasePath() / m_warp_data_filename,
        GetBasePath() / m_fall_data_filename,
        GetBasePath() / m_climb_data_filename,
        GetBasePath() / m_transition_data_filename);
    m_warps_orig = m_warps;
    return true;
}

bool RoomData::AsmLoadMiscPaletteData()
{
    auto load_pal_array = [&](const std::string& name, const std::filesystem::path& fname, const Palette::Type& ptype)
    {
        auto path = GetBasePath() / fname;
        auto data = ReadBytes(path);
        auto size = Palette::GetSizeBytes(ptype);
        assert(data.size() % size == 0);
        std::vector<std::shared_ptr<PaletteEntry>> ret;
        auto it = data.begin();
        int idx = 0;
        for(it = data.begin(); it != data.end(); it += size)
        {
            std::vector<uint8_t> bytes(it, it + size);
            auto e = PaletteEntry::Create(this, bytes, name + ":" + std::to_string(idx++), fname, ptype);
            ret.push_back(e);
        }
        return ret;
    };

    m_lava_palette = load_pal_array(RomLabels::Rooms::PALETTE_LAVA, m_lava_pal_data_filename, Palette::Type::LAVA);
    m_warp_palette = load_pal_array(RomLabels::Rooms::PALETTE_WARP, m_warp_pal_data_filename, Palette::Type::WARP);
    m_labrynth_lit_palette = load_pal_array(RomLabels::Rooms::PALETTE_LANTERN, m_lantern_pal_data_filename, Palette::Type::ROOM).front();
    
    m_lava_palette_orig = m_lava_palette;
    m_warp_palette_orig = m_warp_palette;
    
    return true;
}

bool RoomData::AsmLoadBlocksetData()
{
    try
    {
        m_blocksets_by_name.clear();
        m_blocksets.clear();
        AsmFile dfile(GetBasePath() / m_blockset_data_filename);
        AsmFile::Label lbl;
        AsmFile::IncludeFile inc;
        while (dfile.IsGood())
        {
            dfile >> lbl >> inc;
            auto ep = BlocksetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), lbl, inc.path);
            m_blocksets_by_name.insert({ ep->GetName(), ep});
        }
        m_blocksets_by_name_orig = m_blocksets_by_name;
        m_blocksets_orig = m_blocksets;
        return true;
    }
    catch (const std::exception& e)
    {
        Debug(e.what());
    }
    return false;
}

bool RoomData::AsmLoadBlocksetPtrData()
{
    try
    {
        std::map<std::string, uint8_t> blockset_pri_ptrs;
        std::map<std::string, std::pair<uint8_t, uint8_t>> m_blockset_sec_ptrs;

        AsmFile pri_file(GetBasePath() / m_blockset_pri_ptr_filename);
        AsmFile sec_file(GetBasePath() / m_blockset_sec_ptr_filename);

        pri_file.Goto(RomLabels::Blocksets::POINTER);
        // First item is always a pointer to the start of the regular tileset list.
        // We can safely ignore this
        std::string name;
        AsmFile::Label lbl;
        pri_file >> lbl >> name;
        uint8_t p = 0, s = 0;
        while (pri_file.IsGood())
        {
            pri_file >> name;
            if (m_blocksets_by_name.find(name) == m_blocksets_by_name.end())
            {
                if (blockset_pri_ptrs.find(name) == blockset_pri_ptrs.end())
                {
                    blockset_pri_ptrs.insert({ name, p });
                }
                else
                {
                    blockset_pri_ptrs[name] = p;
                }
            }
            p++;
        }
        while (sec_file.IsGood())
        {
            if (sec_file.IsLabel())
            {
                sec_file >> lbl;
                p = blockset_pri_ptrs[lbl];
                s = 0;
            }
            sec_file >> name;
            if (m_blocksets_by_name.find(name) != m_blocksets_by_name.end())
            {
                m_blockset_sec_ptrs.insert({ name, {p, s++} });
            }
        }
        for (const auto& ptr : m_blockset_sec_ptrs)
        {
            if (m_blocksets_by_name.find(ptr.first) != m_blocksets_by_name.end())
            {
                auto& e = m_blocksets_by_name[ptr.first];
                e->SetIndex(ptr.second);
                m_blocksets.insert({ ptr.second, e });
            }
        }
        m_blocksets_orig = m_blocksets;
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool RoomData::AsmLoadAnimatedTilesetData()
{
    try
    {
        AsmFile animfile(GetBasePath() / m_tileset_anim_filename);
        AsmFile ptrfile(GetBasePath() / m_tileset_ptrtab_filename);
        AsmFile datafile(GetBasePath() / m_tileset_data_filename);
        uint16_t base;
        uint16_t length;
        uint8_t speed;
        uint8_t frames;
        std::string ptrname;
        std::string name;
        AsmFile::IncludeFile inc;

        std::vector<uint8_t> ts_idxs;
        std::map<uint8_t, uint8_t> ts_counts;
        animfile.Goto(RomLabels::Tilesets::ANIM_IDX_LOC);
        uint8_t cur_byte = 0;
        do
        {
            animfile >> cur_byte;
            if (cur_byte != 0xFF)
            {
                ts_idxs.push_back(cur_byte);
            }
        } while (cur_byte != 0xFF);

        auto ts_idx_it = ts_idxs.begin();
        animfile.Goto(RomLabels::Tilesets::ANIM_LIST_LOC);
        while (animfile.IsGood())
        {
            animfile >> base >> length >> speed >> frames >> ptrname;
            ptrfile.Goto(ptrname);
            ptrfile >> name;
            datafile.Goto(name);
            datafile >> inc;
            auto fpath = GetBasePath() / inc.path;
            auto e = AnimatedTilesetEntry::Create(this, ReadBytes(fpath), name, inc.path, base, length, speed, frames, *ts_idx_it);
            e->SetPointerName(ptrname);
            e->SetIndex(*ts_idx_it, ts_counts[*ts_idx_it]);
            m_animated_ts_by_name.insert({ name, e });
            m_animated_ts_by_ptr.insert({ ptrname, e });
            m_animated_ts.insert({ e->GetIndex() , e});
            ts_counts[*ts_idx_it++]++;
        }
        m_animated_ts_by_name_orig = m_animated_ts_by_name;
        m_animated_ts_by_ptr_orig = m_animated_ts_by_ptr;
        m_animated_ts_orig = m_animated_ts;
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmLoadTilesetData()
{
    try
    {
        AsmFile ptrfile(GetBasePath() / m_tileset_ptrtab_filename);
        AsmFile datafile(GetBasePath() / m_tileset_data_filename);
        ptrfile.Goto(RomLabels::Tilesets::PTR_LOC);
        // First item is a pointer to the start of the regular tileset list.
        std::string tileset_begin_label;
        ptrfile >> tileset_begin_label;
        std::size_t i = 0;
        while (ptrfile.IsGood())
        {
            AsmFile::Label lbl;
            std::string name;
            // If it has a label, then it is either the intro font, or an animated tileset.
            if (ptrfile.IsLabel() && !ptrfile.IsLabel(tileset_begin_label))
            {
                ptrfile >> lbl >> name;
                auto it = m_animated_ts_by_ptr.find(lbl);
                if (it == m_animated_ts_by_ptr.end())
                {
                    // Must be the intro font
                    AsmFile::IncludeFile inc;
                    datafile.Goto(name);
                    datafile >> inc;
                    m_intro_font = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, false, 8, 16);
                    m_intro_font->SetPointerName(lbl.label);
                }
            }
            else
            {
                // Regular tileset
                ptrfile >> name;
                if (m_animated_ts_by_name.find(name) == m_animated_ts_by_name.end())
                {
                    AsmFile::IncludeFile inc;
                    datafile.Goto(name);
                    datafile >> inc;
                    auto e = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, true);
                    m_tilesets_by_name.insert({ name, e });
                    e->SetIndex(static_cast<int>(i++));
                    m_tilesets.insert({ static_cast<uint8_t>(e->GetIndex()), e});
                }
                else
                {
                    // This is a dummy entry and can be ignored
                    i++;
                }
            }
        }
        m_tilesets_orig = m_tilesets;
        m_tilesets_by_name_orig = m_tilesets_by_name;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
    }
    return false;
}

bool RoomData::AsmLoadChestData()
{
    auto contents = ReadBytes(GetBasePath() / m_chest_data_filename);
    auto offsets = ReadBytes(GetBasePath() / m_chest_offset_data_filename);
    while (contents.back() == 0xFF)
    {
        contents.pop_back();
    }
    m_chests = Chests(offsets, contents);
    m_chests_orig = m_chests;
    return true;
}

bool RoomData::AsmLoadDoorData()
{
    auto door_offsets = ReadBytes(GetBasePath() / m_door_offset_data_filename);
    auto door_table = ReadBytes(GetBasePath() / m_door_table_data_filename);
    m_doors = Doors(door_offsets, door_table);
    m_doors_orig = m_doors;
    return true;
}

bool RoomData::AsmLoadGfxSwapData()
{
    m_gfxswap_flags = DecodeGfxSwap(ReadBytes(GetBasePath() / m_gfxswap_flag_data_filename));
    m_gfxswap_locked_door_flags = DecodeGfxSwap(ReadBytes(GetBasePath() / m_gfxswap_locked_door_flag_data_filename));
    m_gfxswap_big_tree_flags = DecodeTreeWarp(ReadBytes(GetBasePath() / m_gfxswap_big_tree_flag_data_filename));
    m_gfxswaps = TileSwaps(ReadBytes(GetBasePath() / m_gfxswap_table_data_filename));

    m_gfxswap_flags_orig = m_gfxswap_flags;
    m_gfxswap_locked_door_flags_orig = m_gfxswap_locked_door_flags;
    m_gfxswap_big_tree_flags_orig = m_gfxswap_big_tree_flags;
    m_gfxswaps_orig = m_gfxswaps;
    return true;
}

bool RoomData::AsmLoadMiscData()
{
    m_shop_list = DecodeToSet(ReadBytes(GetBasePath() / m_shop_table_data_filename), true);
    m_lifestock_sold_flags = DecodeToMap(ReadBytes(GetBasePath() / m_lifestock_sold_flag_data_filename));
    m_big_tree_list = DecodeToSet(ReadBytes(GetBasePath() / m_bigtree_data_filename), false);
    m_lantern_flag_list = DecodeToMap(ReadBytes(GetBasePath() / m_lantern_flag_data_filename));

    m_shop_list_orig = m_shop_list;
    m_lifestock_sold_flags_orig = m_lifestock_sold_flags;
    m_big_tree_list_orig = m_big_tree_list;
    m_lantern_flag_list_orig = m_lantern_flag_list;
    return true;
}

bool RoomData::RomLoadRoomData(const Rom& rom)
{
    try
    {
        uint32_t addr = rom.read<uint32_t>(rom.get_address(RomLabels::Rooms::ROOM_DATA_PTR));
        const uint32_t map_end_addr = rom.read<uint32_t>(rom.get_address(RomLabels::Rooms::ROOM_PALS_PTR));

        uint32_t table_end_addr = map_end_addr;
        uint32_t map_addr = map_end_addr;
        std::set<uint32_t> map_list;
        uint16_t index = 0;
        do
        {
            map_addr = rom.read<uint32_t>(addr);
            table_end_addr = std::min(map_addr, table_end_addr);
            map_list.insert(map_addr);
            std::string name = StrPrintf("Room%03d", index);
            auto room = std::make_shared<Room>(name, Hex(map_addr), index++, rom.read_array<uint8_t>(addr + 4, 4));
            m_roomlist.push_back(room);
            m_roomlist_by_name.insert({ name, room });
            addr += 8;
        } while (addr < table_end_addr);
        std::map<std::string, std::string> map_names;
        unsigned int count = 0;
        for (auto it = map_list.begin(); it != map_list.end(); ++it)
        {
            uint32_t begin = *it;
            uint32_t end;
            if (std::next(it) == map_list.end())
            {
                end = map_end_addr;
            }
            else
            {
                end = *std::next(it);
            }
            std::string name = StrPrintf(RomLabels::Rooms::MAP_FORMAT_STRING, ++count);
            map_names[Hex(begin)] = name;
            auto fname = StrPrintf(RomLabels::Rooms::MAP_FILENAME_FORMAT_STRING, name.c_str());
            std::transform(fname.begin(), fname.end(), fname.begin(), [](const unsigned char i) { return static_cast<unsigned char>(std::tolower(i)); });
            auto map_entry = Tilemap3DEntry::Create(this, rom.read_array<uint8_t>(begin, end - begin), name, fname);
            map_entry->SetStartAddress(begin);
            m_maps.insert(std::make_pair(name, map_entry));
        }
        for (auto& room : m_roomlist)
        {
            room->map = map_names[room->map];
            m_roomlist_orig.push_back(std::make_shared<Room>(*room));
        }
        m_maps_orig = m_maps;
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool RoomData::RomLoadRoomPalettes(const Rom& rom)
{
    uint32_t palettes_begin = rom.read<uint32_t>(rom.get_address(RomLabels::Rooms::ROOM_PALS_PTR));
    uint32_t palettes_end = rom.read<uint32_t>(rom.get_address(RomLabels::Rooms::ROOM_EXITS_PTR));
    assert(palettes_end > palettes_begin);
    uint32_t size = Palette::GetSizeBytes(Palette::Type::ROOM);
    assert((palettes_end - palettes_begin) % size == 0);
    unsigned int i = 0;
    unsigned int addr = palettes_begin;
    while (addr < palettes_end)
    {
        std::string name = StrPrintf(RomLabels::Rooms::ROOM_PAL_NAME, i + 1);
        auto fname = StrPrintf(RomLabels::Rooms::PALETTE_FORMAT_STRING, i + 1);
        auto fpath = StrPrintf(RomLabels::Rooms::PALETTE_FILENAME_FORMAT_STRING, fname.c_str());
        std::transform(fpath.begin(), fpath.end(), fpath.begin(), [](const unsigned char i) { return static_cast<unsigned char>(std::tolower(i)); });
        auto e = PaletteEntry::Create(this, rom.read_array<uint8_t>(addr, size), name, fpath, Palette::Type::ROOM);
        e->SetStartAddress(addr);
        m_room_pals.push_back(e);
        m_room_pals_by_name.insert({ name, e });
        addr += size;
        i++;
    }
    m_room_pals_orig = m_room_pals;
    return true;
}

bool RoomData::RomLoadWarpData(const Rom& rom)
{
    m_warps = WarpList(rom);
    m_warps_orig = m_warps;
    return true;
}

bool RoomData::RomLoadMiscPaletteData(const Rom& rom)
{
    auto load_pal_array = [&](const std::string& name, const std::filesystem::path& fname, const Palette::Type& ptype)
    {
        uint32_t addr = rom.get_section(name).begin;
        uint32_t end = rom.get_section(name).end;
        uint32_t size = Palette::GetSizeBytes(ptype);
        std::vector<std::shared_ptr<PaletteEntry>> ret;
        int idx = 0;
        for (; addr < end; addr += size)
        {
            auto bytes = rom.read_array<uint8_t>(addr, size);
            auto e = PaletteEntry::Create(this, bytes, name + ":" + std::to_string(idx++), fname, ptype);
            e->SetStartAddress(addr);
            ret.push_back(e);
        }
        return ret;
    };
    m_lava_palette = load_pal_array(RomLabels::Rooms::PALETTE_LAVA, RomLabels::Rooms::PALETTE_LAVA_FILENAME, Palette::Type::LAVA);
    m_warp_palette = load_pal_array(RomLabels::Rooms::PALETTE_WARP, RomLabels::Rooms::PALETTE_WARP_FILENAME, Palette::Type::WARP);
    m_labrynth_lit_palette = load_pal_array(RomLabels::Rooms::PALETTE_LANTERN, RomLabels::Rooms::PALETTE_LANTERN_FILENAME, Palette::Type::ROOM).front();
    
    m_lava_palette_orig = m_lava_palette;
    m_warp_palette_orig = m_warp_palette;
    
    return true;
}

bool RoomData::RomLoadBlocksetData(const Rom& rom)
{
    uint32_t blockset_begin = rom.read<uint32_t>(RomLabels::Blocksets::POINTER);
    std::map<uint32_t, uint8_t> pri_ptrs;
    std::map<uint32_t, std::pair<uint8_t, uint8_t>> sec_ptrs;
    std::set<uint32_t> b_ptrs;
    for (uint8_t i = 0; i < 64; ++i)
    {
        uint32_t ptr = rom.read<uint32_t>(blockset_begin + i * 4);
        pri_ptrs[ptr] = i;
    }
    auto p_it = pri_ptrs.cbegin();
    while (p_it != std::prev(pri_ptrs.cend()))
    {
        uint8_t pri = p_it->second;
        uint8_t sec = 0;
        uint32_t begin = p_it->first;
        ++p_it;
        uint32_t end = p_it->first;
        for (uint32_t addr = begin; addr < end; addr += 4)
        {
            sec_ptrs[addr] = { pri, sec++ };
        }
    }
    for (auto s_it = sec_ptrs.cbegin(); s_it != sec_ptrs.cend(); ++s_it)
    {
        uint32_t begin = rom.read<uint32_t>(s_it->first);
        uint32_t end;
        if (std::next(s_it) != sec_ptrs.cend())
        {
            end = rom.read<uint32_t>(std::next(s_it)->first);
        }
        else
        {
            end = rom.get_section(RomLabels::Blocksets::SECTION).end;
        }
        if (!RomLoadBlockset(rom, s_it->second.first, s_it->second.second, begin, end))
        {
            return false;
        }
    }
    m_blocksets_by_name_orig = m_blocksets_by_name;
    m_blocksets_orig = m_blocksets;
    return true;
}

bool RoomData::RomLoadBlockset(const Rom& rom, uint8_t pri, uint8_t sec, uint32_t begin, uint32_t end)
{
    std::string name = StrPrintf(RomLabels::Blocksets::BLOCKSET_LABEL, (pri & 0x1F) + 1, sec + 10 * (pri >> 5));
    std::filesystem::path filename = StrPrintf(RomLabels::Blocksets::BLOCKSET_FILE, (pri & 0x1F) + 1, sec + 10 * (pri >> 5));
    auto e = BlocksetEntry::Create(this, rom.read_array<uint8_t>(begin, end - begin), name, filename);
    e->SetStartAddress(begin);
    e->SetIndex({ pri, sec });
    m_blocksets_by_name.insert({ name, e });
    m_blocksets.insert({ {pri, sec}, e });
    return true;
}

bool RoomData::RomLoadAllTilesetData(const Rom& rom)
{
    uint32_t anim_addr = rom.get_section(RomLabels::Tilesets::ANIM_DATA_LOC).begin;
    uint32_t data_addr = rom.get_section(RomLabels::Tilesets::DATA_LOC).begin;
    uint32_t introfont_ptr = rom.read<uint32_t>(rom.get_address(RomLabels::Tilesets::INTRO_FONT_PTR));
    uint32_t introfont_begin = rom.read<uint32_t>(introfont_ptr);
    uint32_t introfont_size = rom.get_section(RomLabels::Tilesets::INTRO_FONT).size();
    std::map<uint32_t, uint32_t> anim_tileset_ptr_list;
    std::map<uint32_t, uint32_t> room_tileset_ptr_list;
    std::set<uint32_t> ts_ptrs;
    std::set<uint32_t> anim_ts_ptrs;
    std::map<uint32_t, std::vector<uint8_t>> tilesets;
    std::vector<uint32_t> room_tileset_ptrs;

    uint32_t ts_ptr_start = rom.inc_read<uint32_t>(data_addr);
    uint32_t ts_ptr_end = ts_ptr_start + 32 * sizeof(uint32_t);
    while (data_addr < ts_ptr_end)
    {
        uint32_t addr = rom.read<uint32_t>(data_addr);
        ts_ptrs.insert(addr);
        if (data_addr >= ts_ptr_start)
        {
            if (anim_ts_ptrs.find(addr) == anim_ts_ptrs.cend())
            {
                room_tileset_ptrs.push_back(addr);
                room_tileset_ptr_list.insert({ data_addr, addr });
            }
            else
            {
                room_tileset_ptrs.push_back(0);
            }
        }
        else if (addr != introfont_begin)
        {
            anim_tileset_ptr_list.insert({ data_addr, addr });
            anim_ts_ptrs.insert(addr);
        }
        data_addr += sizeof(uint32_t);
    }
    for (auto it = ts_ptrs.cbegin(); it != std::prev(ts_ptrs.cend()); ++it)
    {
        uint32_t start_addr = *it;
        uint32_t end_addr = *std::next(it);
        auto bytes = rom.read_array<uint8_t>(start_addr, end_addr - start_addr);
        tilesets.insert({ start_addr, bytes });
    }
    auto introfont_bytes = rom.read_array<uint8_t>(introfont_begin, introfont_size);
    m_intro_font = TilesetEntry::Create(this, introfont_bytes, RomLabels::Tilesets::INTRO_FONT,
        RomLabels::Tilesets::INTRO_FONT_FILENAME, false, 8, 16);
    m_intro_font->SetStartAddress(introfont_begin);
    m_intro_font->SetPointerName(RomLabels::Tilesets::INTRO_FONT_PTR);

    std::map<uint32_t, std::string> ptr_names;
    std::map<uint32_t, std::string> tileset_names;

    for (uint8_t i = 0; i < static_cast<uint8_t>(room_tileset_ptrs.size()); ++i)
    {
        uint32_t addr = room_tileset_ptrs[i];
        if (addr == 0)
        {
            continue;
        }
        std::string name = StrPrintf(RomLabels::Tilesets::LABEL_FORMAT_STRING, i + 1);
        std::string fname = StrPrintf(RomLabels::Tilesets::FILENAME_FORMAT_STRING, i + 1);
        auto e = TilesetEntry::Create(this, tilesets[addr], name, fname);
        e->SetStartAddress(addr);
        e->SetIndex(i);
        m_tilesets.insert({ i, e });
        m_tilesets_by_name.insert({ name, e });
    }

    std::map<int, std::set<uint32_t>> animts_ptr_list;
    std::vector<uint8_t> idx_list;
    // Read in animation index table
    for (uint8_t next = rom.inc_read<uint8_t>(anim_addr); next != 0xFF; next = rom.inc_read<uint8_t>(anim_addr))
    {
        idx_list.push_back(next);
    };
    anim_addr += anim_addr & 1; // Fix alignment
    // Next, read in the animation list
    uint16_t base, length;
    uint8_t speed, frames;
    uint32_t ts_addr;

    for (auto idx = idx_list.begin(); idx != idx_list.end(); idx++)
    {
        base = rom.inc_read<uint16_t>(anim_addr);
        length = rom.inc_read<uint16_t>(anim_addr);
        speed = rom.inc_read<uint8_t>(anim_addr);
        frames = rom.inc_read<uint8_t>(anim_addr);
        ts_addr = rom.inc_read<uint32_t>(anim_addr);
        uint32_t start_address = ts_addr;
        if (animts_ptr_list.find(*idx) == animts_ptr_list.end())
        {
            animts_ptr_list.insert({ *idx, std::set<uint32_t>() });
        }
        animts_ptr_list[*idx].insert(start_address);
        std::size_t subts = animts_ptr_list[*idx].size();
        auto name = StrPrintf(RomLabels::Tilesets::ANIM_LABEL_FORMAT_STRING, *idx + 1, subts);
        auto ptrname = StrPrintf(RomLabels::Tilesets::ANIM_PTR_LABEL_FORMAT_STRING, *idx + 1, subts);
        auto filename = StrPrintf(RomLabels::Tilesets::ANIM_FILENAME_FORMAT_STRING, *idx + 1, subts);
        uint32_t addr = rom.read<uint32_t>(ts_addr);
        auto e = AnimatedTilesetEntry::Create(this, tilesets[addr], name, filename, base, length, speed, frames, *idx);
        e->SetStartAddress(addr);
        e->SetPointerName(ptrname);
        e->SetIndex(*idx, static_cast<uint8_t>(subts - 1));
        m_animated_ts.insert({ e->GetIndex(), e });
        m_animated_ts_by_name.insert({ name, e });
        m_animated_ts_by_ptr.insert({ ptrname, e });
    }
    m_tilesets_orig = m_tilesets;
    m_tilesets_by_name_orig = m_tilesets_by_name;
    m_animated_ts_orig = m_animated_ts;
    m_animated_ts_by_name_orig = m_animated_ts_by_name;
    m_animated_ts_by_ptr_orig = m_animated_ts_by_ptr;
    return true;
}

bool RoomData::RomLoadChestData(const Rom& rom)
{
    uint32_t offsets_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::CHEST_OFFSETS);
    uint32_t chests_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::CHEST_CONTENTS);
    uint32_t offsets_end = chests_begin;
    uint32_t chests_end = rom.get_section(RomLabels::Rooms::CHEST_SECTION).end;
    auto offset_bytes = rom.read_array<uint8_t>(offsets_begin, offsets_end - offsets_begin);
    auto chest_bytes = rom.read_array<uint8_t>(chests_begin, chests_end - chests_begin);
    while (chest_bytes.back() == 0xFF)
    {
        chest_bytes.pop_back();
    }
    m_chests = Chests(offset_bytes, chest_bytes);
    m_chests_orig = m_chests;
    return true;
}

bool RoomData::RomLoadDoorData(const Rom& rom)
{
    uint32_t offsets_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::DOOR_OFFSET_TABLE);
    uint32_t doors_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::DOOR_TABLE);
    uint32_t offsets_end = doors_begin;
    uint32_t doors_end = rom.get_section(RomLabels::Rooms::DOOR_TABLE_SECTION).end;
    auto offset_bytes = rom.read_array<uint8_t>(offsets_begin, offsets_end - offsets_begin);
    auto door_bytes = rom.read_array<uint8_t>(doors_begin, doors_end - doors_begin);
    m_doors = Doors(offset_bytes, door_bytes);
    m_doors_orig = m_doors;
    return true;
}

bool RoomData::RomLoadGfxSwapData(const Rom& rom)
{
    uint32_t flags_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::GFX_SWAP_FLAGS);
    uint32_t locked_door_flags_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::GFX_SWAP_LOCKED_DOOR_FLAGS_LEA1);
    uint32_t tree_flags_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::GFX_SWAP_TREE_FLAGS);
    uint32_t swap_table_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::GFX_SWAP_TABLE);
    uint32_t swap_table_end = rom.get_section(RomLabels::Rooms::GFX_SWAP_SECTION).end;
    auto flag_bytes = rom.read_array<uint8_t>(flags_begin, locked_door_flags_begin - flags_begin);
    auto door_bytes = rom.read_array<uint8_t>(locked_door_flags_begin, tree_flags_begin - locked_door_flags_begin);
    auto tree_bytes = rom.read_array<uint8_t>(tree_flags_begin, swap_table_begin - tree_flags_begin);
    m_gfxswaps = TileSwaps(rom.read_array<uint8_t>(swap_table_begin, swap_table_end - swap_table_begin));
    m_gfxswap_flags = DecodeGfxSwap(flag_bytes);
    m_gfxswap_locked_door_flags = DecodeGfxSwap(door_bytes);
    m_gfxswap_big_tree_flags = DecodeTreeWarp(tree_bytes);
    m_gfxswap_flags_orig = m_gfxswap_flags;
    m_gfxswap_locked_door_flags_orig = m_gfxswap_locked_door_flags;
    m_gfxswap_big_tree_flags_orig = m_gfxswap_big_tree_flags;
    m_gfxswaps_orig = m_gfxswaps;
    return true;
}

bool RoomData::RomLoadMiscData(const Rom& rom)
{
    uint32_t shops_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::SHOP_LIST_LEA1);
    uint32_t shops_end = rom.get_section(RomLabels::Rooms::SHOP_LIST_SECTION).end;
    uint32_t lifestock_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::LIFESTOCK_SOLD_FLAGS);
    uint32_t lifestock_end = rom.get_section(RomLabels::Rooms::LIFESTOCK_SOLD_FLAGS_SECTION).end;
    uint32_t trees_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::BIG_TREE_LOCATIONS);
    uint32_t trees_end = rom.get_section(RomLabels::Rooms::BIG_TREE_LOCATIONS_SECTION).end;
    uint32_t lantern_begin = Disasm::ReadOffset16(rom, RomLabels::Rooms::LANTERN_ROOM_FLAGS);
    uint32_t lantern_end = rom.get_section(RomLabels::Rooms::LANTERN_ROOM_FLAGS_SECTION).end;

    auto shops_bytes = rom.read_array<uint8_t>(shops_begin, shops_end - shops_begin);
    auto lifestock_bytes = rom.read_array<uint8_t>(lifestock_begin, lifestock_end - lifestock_begin);
    auto trees_bytes = rom.read_array<uint8_t>(trees_begin, trees_end - trees_begin);
    auto lantern_bytes = rom.read_array<uint8_t>(lantern_begin, lantern_end - lantern_begin);
    m_shop_list = DecodeToSet(shops_bytes, true);
    m_lifestock_sold_flags = DecodeToMap(lifestock_bytes);
    m_big_tree_list = DecodeToSet(trees_bytes, false);
    m_lantern_flag_list = DecodeToMap(lantern_bytes);
    
    m_shop_list_orig = m_shop_list;
    m_lifestock_sold_flags_orig = m_lifestock_sold_flags;
    m_big_tree_list_orig = m_big_tree_list;
    m_lantern_flag_list_orig = m_lantern_flag_list;
    return true;
}

bool RoomData::AsmSaveMaps(const std::filesystem::path& dir)
{
    try
    {
        AsmFile file;
        file.WriteFileHeader(m_map_data_filename, "Map data include file");
        for (auto& map : m_maps)
        {
            file << AsmFile::Label(map.first) << AsmFile::IncludeFile(map.second->GetFilename(), AsmFile::FileType::BINARY);
            if (!map.second->Save(dir))
            {
                return false;
            }
        }
        file.WriteFile(dir / m_map_data_filename);
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmSaveRoomData(const std::filesystem::path& dir)
{
    try
    {
        AsmFile file;
        file.WriteFileHeader(m_room_data_filename, "Room data include file");
        for (const auto& room : m_roomlist)
        {
            auto params = room->GetParams();
            file << AsmFile::Label(room->name) << room->map << params;
        }
        auto f = dir / m_room_data_filename;
        file.WriteFile(f);
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmSaveWarpData(const std::filesystem::path& dir)
{
    auto warp_bytes = m_warps.GetWarpBytes();
    auto fall_bytes = m_warps.GetFallBytes();
    auto climb_bytes = m_warps.GetClimbBytes();
    auto transition_bytes = m_warps.GetTransitionBytes();

    WriteBytes(warp_bytes, dir / m_warp_data_filename);
    WriteBytes(fall_bytes, dir / m_fall_data_filename);
    WriteBytes(climb_bytes, dir / m_climb_data_filename);
    WriteBytes(transition_bytes, dir / m_transition_data_filename);

    return true;
}

bool RoomData::AsmSaveRoomPalettes(const std::filesystem::path& dir)
{
    try
    {
        AsmFile file;
        file.WriteFileHeader(m_palette_data_filename, "Room palette data include file");
        for (const auto& palette : m_room_pals)
        {
            if (!palette->Save(dir))
            {
                return false;
            }
            file << AsmFile::Label(palette->GetName()) << AsmFile::IncludeFile(palette->GetFilename(), AsmFile::FileType::BINARY);
        }
        file.WriteFile(dir / m_palette_data_filename);
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmSaveMiscPaletteData(const std::filesystem::path& dir)
{
    m_labrynth_lit_palette->Save(dir);

    auto combine_palette_array = [](std::vector<std::shared_ptr<PaletteEntry>>& pals)
    {
        std::vector<uint8_t> ret;
        for (auto& p : pals)
        {
            auto bytes = p->GetBytes();
            ret.insert(ret.end(), bytes->begin(), bytes->end());
        }
        return ret;
    };
    auto lava_pal_bytes = combine_palette_array(m_lava_palette);
    auto warp_pal_bytes = combine_palette_array(m_warp_palette);

    WriteBytes(lava_pal_bytes, dir / m_lava_pal_data_filename);
    WriteBytes(warp_pal_bytes, dir / m_warp_pal_data_filename);
    return true;
}

bool RoomData::AsmSaveBlocksetPointerData(const std::filesystem::path& dir)
{
    try
    {
        AsmFile pri_file;
        AsmFile sec_file;
        pri_file.WriteFileHeader(m_blockset_pri_ptr_filename, "Blockset primary pointer file");
        sec_file.WriteFileHeader(m_blockset_sec_ptr_filename, "Blockset secondary pointer file");

        pri_file << AsmFile::Label(RomLabels::Blocksets::POINTER) << RomLabels::Blocksets::PRI_LABEL;
        pri_file << AsmFile::Label(RomLabels::Blocksets::PRI_LABEL);

        int pri_ptr_count = 0;
        int pri = -1;
        for (const auto& bs : m_blocksets)
        {
            if (bs.first.first != pri)
            {
                pri = bs.first.first;
                auto name = StrPrintf(RomLabels::Blocksets::SEC_LABEL, (pri & 0x1F) + 1);
                if (pri > 0x1F)
                {
                    name += "A";
                }
                do
                {
                    pri_file << name;
                } while (++pri_ptr_count < (pri + 1));
                sec_file << AsmFile::Label(name);
            }
            sec_file << bs.second->GetName();
        }
        while (pri_ptr_count++ < 64)
        {
            pri_file << m_blocksets.begin()->second->GetName();
        }

        auto prifname = dir / m_blockset_pri_ptr_filename;
        auto secfname = dir / m_blockset_sec_ptr_filename;
        pri_file.WriteFile(prifname);
        sec_file.WriteFile(secfname);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool RoomData::AsmSaveBlocksetData(const std::filesystem::path& dir)
{
    try
    {
        AsmFile file;
        file.WriteFileHeader(m_blockset_data_filename, "Blockset data include file");
        for (const auto& bs : m_blocksets)
        {
            file << AsmFile::Label(bs.second->GetName()) << AsmFile::IncludeFile(bs.second->GetFilename(), AsmFile::FileType::BINARY);
            bs.second->Save(dir);
        }
        auto incfname = dir / m_blockset_data_filename;
        file.WriteFile(incfname);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool RoomData::AsmSaveTilesetData(const std::filesystem::path& dir)
{
    try
    {
        AsmFile file;
        file.WriteFileHeader(m_tileset_data_filename, "Tileset data include file");
        for (const auto& ts : m_tilesets)
        {
            file << AsmFile::Label(ts.second->GetName()) << AsmFile::IncludeFile(ts.second->GetFilename(), AsmFile::FileType::BINARY);
            ts.second->Save(dir);
        }
        file << AsmFile::Align(2);
        for (const auto& ts : m_animated_ts)
        {
            file << AsmFile::Label(ts.second->GetName()) << AsmFile::IncludeFile(ts.second->GetFilename(), AsmFile::FileType::BINARY);
            ts.second->Save(dir);
        }
        file << AsmFile::Label(m_intro_font->GetName()) << AsmFile::IncludeFile(m_intro_font->GetFilename(), AsmFile::FileType::BINARY);
        m_intro_font->Save(dir);
        auto incfname = dir / m_tileset_data_filename;
        file.WriteFile(incfname);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool RoomData::AsmSaveTilesetPointerData(const std::filesystem::path& dir)
{
    try
    {
        AsmFile file;
        file.WriteFileHeader(m_tileset_ptrtab_filename, "Tileset pointer table file");
        
        file << AsmFile::Label(RomLabels::Tilesets::PTR_LOC);
        file << RomLabels::Tilesets::PTRTAB_BEGIN_LOC;

        for (const auto& ts : m_animated_ts)
        {
            file << AsmFile::Label(ts.second->GetPointerName()) << ts.second->GetName();
        }
        file << AsmFile::Label(m_intro_font->GetPointerName()) << m_intro_font->GetName();
        file << AsmFile::Label(RomLabels::Tilesets::PTRTAB_BEGIN_LOC);
        unsigned int i = 0;
        for (const auto& ts : m_tilesets)
        {
            do
            {
                file << ts.second->GetName();
            } while (i++ < ts.first);
        }
        while (i++ < 32)
        {
            file << m_animated_ts_by_name.begin()->first;
        }

        auto incfname = dir / m_tileset_ptrtab_filename;
        file.WriteFile(incfname);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

bool RoomData::AsmSaveAnimatedTilesetData(const std::filesystem::path& dir)
{
    try
    {
        AsmFile file;
        file.WriteFileHeader(m_tileset_anim_filename, "Animated tileset definition file");

        std::vector<uint8_t> idxs;
        for (const auto& ts : m_animated_ts)
        {
            idxs.push_back(ts.second->GetData()->GetBaseTileset());
        }
        idxs.push_back(0xFF);
        file << AsmFile::Label(RomLabels::Tilesets::ANIM_IDX_LOC) << idxs;
        file << AsmFile::Align(2) << AsmFile::Label(RomLabels::Tilesets::ANIM_LIST_LOC);

        for (const auto& ts : m_animated_ts)
        {
            auto ats = ts.second->GetData();
            file << ats->GetBaseBytes() << ats->GetFrameSizeBytes()
                << ats->GetAnimationSpeed() << ats->GetAnimationFrames()
                << ts.second->GetPointerName();
        }
        auto f = dir / m_tileset_anim_filename;
        file.WriteFile(f);
        return true;
    }
    catch (const std::exception&)
    {
    }
    return false;
}

bool RoomData::AsmSaveChestData(const std::filesystem::path& dir)
{
    auto result = m_chests.GetData(GetRoomCount());
    WriteBytes(result.first, dir / m_chest_offset_data_filename);
    WriteBytes(result.second, dir / m_chest_data_filename);
    return true;
}

bool RoomData::AsmSaveDoorData(const std::filesystem::path& dir)
{
    auto result = m_doors.GetData(GetRoomCount());
    WriteBytes(result.first, dir / m_door_offset_data_filename);
    WriteBytes(result.second, dir / m_door_table_data_filename);
    return true;
}

bool RoomData::AsmSaveGfxSwapData(const std::filesystem::path& dir)
{
    WriteBytes(EncodeGfxSwap(m_gfxswap_flags), dir / m_gfxswap_flag_data_filename);
    WriteBytes(EncodeGfxSwap(m_gfxswap_locked_door_flags), dir / m_gfxswap_locked_door_flag_data_filename);
    WriteBytes(EncodeTreeWarp(m_gfxswap_big_tree_flags), dir / m_gfxswap_big_tree_flag_data_filename);
    WriteBytes(m_gfxswaps.GetData(), dir / m_gfxswap_table_data_filename);
    return true;
}

bool RoomData::AsmSaveMiscData(const std::filesystem::path& dir)
{
    WriteBytes(EncodeFromSet(m_shop_list, true), dir / m_shop_table_data_filename);
    WriteBytes(EncodeFromMap(m_lifestock_sold_flags), dir / m_lifestock_sold_flag_data_filename);
    WriteBytes(EncodeFromSet(m_big_tree_list, false), dir / m_bigtree_data_filename);
    WriteBytes(EncodeFromMap(m_lantern_flag_list), dir / m_lantern_flag_data_filename);
    return true;
}

bool RoomData::RomPrepareInjectMiscWarp(const Rom& rom)
{
    auto fall_bytes = m_warps.GetFallBytes();
    auto climb_bytes = m_warps.GetClimbBytes();
    auto transition_bytes = m_warps.GetTransitionBytes();
    ByteVectorPtr pend_write = std::make_shared<ByteVector>(fall_bytes);
    pend_write->insert(pend_write->end(), climb_bytes.begin(), climb_bytes.end());
    pend_write->insert(pend_write->end(), transition_bytes.begin(), transition_bytes.end());
    m_pending_writes.push_back({ RomLabels::Rooms::MISC_WARP_SECTION, pend_write });

    uint32_t fall_addr = rom.get_section(RomLabels::Rooms::MISC_WARP_SECTION).begin;
    uint32_t climb_addr = fall_addr + fall_bytes.size();
    uint32_t transition_addr = climb_addr + climb_bytes.size();

    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::FALL_TABLE_LEA_LOC, fall_addr));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::CLIMB_TABLE_LEA_LOC, climb_addr));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::TRANSITION_TABLE_LEA_LOC1, transition_addr));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::TRANSITION_TABLE_LEA_LOC2, transition_addr));

    return true;
}

bool RoomData::RomPrepareInjectRoomData(const Rom& rom)
{
    std::map<std::string, uint32_t> map_addrs;
    std::vector<uint8_t> map_bytes;
    std::vector<uint8_t> pal_bytes;
    auto bytes = std::make_shared<std::vector<uint8_t>>();
    uint32_t roomlist_size = m_roomlist.size() * 8;
    uint32_t data_begin = rom.get_section(RomLabels::Rooms::ROOM_DATA_SECTION).begin;
    auto warp_bytes = m_warps.GetWarpBytes();
    for (auto& map : m_maps)
    {
        map_addrs.insert({ map.first, map_bytes.size() + roomlist_size + data_begin });
        auto mbytes = map.second->GetBytes();
        map_bytes.insert(map_bytes.end(), mbytes->begin(), mbytes->end());
    }
    for (const auto& room : m_roomlist)
    {
        auto mapaddr = Split<uint8_t>(map_addrs[room->map]);
        auto params = room->GetParams();
        bytes->insert(bytes->end(), mapaddr.begin(), mapaddr.end());
        bytes->insert(bytes->end(), params.begin(), params.end());
    }
    bytes->insert(bytes->end(), map_bytes.begin(), map_bytes.end());
    // Alignment correction
    if ((bytes->size() & 1) == 1)
    {
        bytes->push_back(0xFF);
    }
    uint32_t pal_begin = data_begin + bytes->size();
    for (auto& pal : m_room_pals)
    {
        auto pbytes = pal->GetBytes();
        bytes->insert(bytes->end(), pbytes->begin(), pbytes->end());
    }
    uint32_t warp_begin = data_begin + bytes->size();
    bytes->insert(bytes->end(), warp_bytes.begin(), warp_bytes.end());
    m_pending_writes.push_back({ RomLabels::Rooms::ROOM_DATA_SECTION, bytes });
    m_pending_writes.push_back({ RomLabels::Rooms::ROOM_DATA_PTR, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(data_begin)) });
    m_pending_writes.push_back({ RomLabels::Rooms::ROOM_PALS_PTR, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(pal_begin)) });
    m_pending_writes.push_back({ RomLabels::Rooms::ROOM_EXITS_PTR, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(warp_begin)) });
    return true;
}

bool RoomData::RomPrepareInjectMiscPaletteData(const Rom& /*rom*/)
{
    auto combine_palette_array = [](std::vector<std::shared_ptr<PaletteEntry>>& pals)
    {
        std::shared_ptr<std::vector<uint8_t>> ret = std::make_shared<std::vector<uint8_t>>();
        for (auto& p : pals)
        {
            auto bytes = p->GetBytes();
            ret->insert(ret->end(), bytes->begin(), bytes->end());
        }
        return ret;
    };
    auto lantern_pal_bytes = m_labrynth_lit_palette->GetBytes();
    auto lava_pal_bytes = combine_palette_array(m_lava_palette);
    auto warp_pal_bytes = combine_palette_array(m_warp_palette);

    m_pending_writes.push_back({ RomLabels::Rooms::PALETTE_LANTERN, lantern_pal_bytes });
    m_pending_writes.push_back({ RomLabels::Rooms::PALETTE_LAVA, lava_pal_bytes });
    m_pending_writes.push_back({ RomLabels::Rooms::PALETTE_WARP, warp_pal_bytes });

    return true;
}

bool RoomData::RomPrepareInjectBlocksetData(const Rom& rom)
{
    std::map<std::string, uint32_t> blockset_addrs;
    uint32_t base = rom.get_section(RomLabels::Blocksets::SECTION).begin;
    uint32_t pri_size = (64 + 1) * 4;
    uint32_t sec_size = m_blocksets.size() * 4;
    uint32_t blockset_data_offset = base + pri_size + sec_size;
    ByteVectorPtr bytes = std::make_shared<ByteVector>();
    ByteVector blocks;
    ByteVector sec;
    auto start_ptr = Split<uint8_t, uint32_t>(base + 4);
    bytes->insert(bytes->end(), start_ptr.begin(), start_ptr.end());
    int pri_ptr_count = 0;
    int pri = -1;
    int idx = 0;
    for (const auto& bs : m_blocksets)
    {
        blockset_addrs.insert({ bs.second->GetName(), blocks.size() + blockset_data_offset});
        auto data = bs.second->GetBytes();
        blocks.insert(blocks.end(), data->begin(), data->end());
    }
    for (const auto& bs : m_blocksets)
    {
        if (bs.first.first != pri)
        {
            pri = bs.first.first;
            auto secptr = Split<uint8_t, uint32_t>(base + pri_size + idx * 4);
            do
            {
                bytes->insert(bytes->end(), secptr.begin(), secptr.end());
            } while (++pri_ptr_count < (pri + 1));
        }
        auto bsptr = Split<uint8_t, uint32_t>(blockset_addrs[bs.second->GetName()]);
        sec.insert(sec.end(), bsptr.begin(), bsptr.end());
        idx++;
    }
    auto bsptr = Split<uint8_t, uint32_t>(blockset_addrs.begin()->second);
    while (pri_ptr_count++ < 64)
    {
        bytes->insert(bytes->end(), bsptr.begin(), bsptr.end());
    }
    bytes->insert(bytes->end(), sec.begin(), sec.end());
    bytes->insert(bytes->end(), blocks.begin(), blocks.end());
    m_pending_writes.push_back({ RomLabels::Blocksets::SECTION, bytes });
    return true;
}

bool RoomData::RomPrepareInjectTilesetData(const Rom& rom)
{
    const std::size_t tilesets_begin = rom.get_section(RomLabels::Tilesets::SECTION).begin;
    auto bytes = std::make_shared<ByteVector>();
    ByteVector tilesets;
    const uint32_t misc_pointer_space = (2 + m_animated_ts.size()) * sizeof(uint32_t);
    const uint32_t tileset_pointer_space = 32 * sizeof(uint32_t);
    const uint32_t tileset_base = tilesets_begin + misc_pointer_space + tileset_pointer_space;
    std::map<std::string, uint32_t> addrs;

    for (const auto& ts : m_tilesets_by_name)
    {
        addrs[ts.first] = tileset_base + tilesets.size();
        auto data = ts.second->GetBytes();
        tilesets.insert(tilesets.end(), data->cbegin(), data->cend());
    }
    if ((tilesets.size() & 1) == 1)
    {
        tilesets.push_back(0xFF);
    }
    for (const auto& ts : m_animated_ts_by_name)
    {
        addrs[ts.first] = tileset_base + tilesets.size();
        auto data = ts.second->GetBytes();
        tilesets.insert(tilesets.end(), data->cbegin(), data->cend());
    }
    addrs[m_intro_font->GetName()] = tileset_base + tilesets.size();
    auto data = m_intro_font->GetBytes();
    tilesets.insert(tilesets.end(), data->cbegin(), data->cend());

    const auto insert_pointer = [](ByteVectorPtr& dest, uint32_t value)
    {
        auto ptrbytes = Split<uint8_t, uint32_t>(value);
        dest->insert(dest->end(), ptrbytes.cbegin(), ptrbytes.cend());
    };

    insert_pointer(bytes, tilesets_begin + misc_pointer_space);
    for (const auto& ts : m_animated_ts_by_name)
    {
        insert_pointer(bytes, addrs.at(ts.first));
    }
    insert_pointer(bytes, addrs.at(m_intro_font->GetName()));

    int i = 0;
    for (const auto& ts : m_tilesets)
    {
        do
        {
            insert_pointer(bytes, addrs.at(ts.second->GetName()));
        } while (i++ < ts.first);
    }
    while (i++ < 32)
    {
        insert_pointer(bytes, addrs.at(m_animated_ts_by_name.begin()->first));
    }

    bytes->insert(bytes->end(), tilesets.cbegin(), tilesets.cend());

    m_pending_writes.push_back({ RomLabels::Tilesets::SECTION, bytes });

    return true;
}

bool RoomData::RomPrepareInjectAnimatedTilesetData(const Rom& rom)
{
    const std::size_t tilesets_begin = rom.get_section(RomLabels::Tilesets::SECTION).begin;

    ByteVectorPtr bytes = std::make_shared<ByteVector>();
    for (const auto& ts : m_animated_ts)
    {
        bytes->push_back(ts.second->GetData()->GetBaseTileset());
    }
    bytes->push_back(0xFF);

    if ((bytes->size() & 1) == 1)
    {
        bytes->push_back(0xFF);
    }

    int i = 1;
    for (const auto& ts : m_animated_ts)
    {
        bytes->push_back(ts.second->GetData()->GetBaseBytes() >> 8);
        bytes->push_back(ts.second->GetData()->GetBaseBytes() & 0xFF);
        bytes->push_back(ts.second->GetData()->GetFrameSizeBytes() >> 8);
        bytes->push_back(ts.second->GetData()->GetFrameSizeBytes() & 0xFF);
        bytes->push_back(ts.second->GetData()->GetAnimationSpeed());
        bytes->push_back(ts.second->GetData()->GetAnimationFrames());
        auto ptr = Split<uint8_t, uint32_t>(tilesets_begin + (i * sizeof(uint32_t)));
        bytes->insert(bytes->end(), ptr.cbegin(), ptr.cend());
        i++;
    }

    m_pending_writes.push_back({ RomLabels::Tilesets::ANIM_DATA_LOC, bytes });

    return true;
}

bool RoomData::RomPrepareInjectChestData(const Rom& rom)
{
    auto results = m_chests.GetData(GetRoomCount());
    uint32_t offsets_begin = rom.get_section(RomLabels::Rooms::CHEST_SECTION).begin;
    uint32_t chests_begin = offsets_begin + results.first.size();
    auto data = std::make_shared<ByteVector>(results.first);
    data->insert(data->end(), results.second.begin(), results.second.end());
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::CHEST_OFFSETS, offsets_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::CHEST_CONTENTS, chests_begin));
    m_pending_writes.push_back({ RomLabels::Rooms::CHEST_SECTION, data });
    return true;
}

bool RoomData::RomPrepareInjectDoorData(const Rom& rom)
{
    auto result = m_doors.GetData(GetRoomCount());
    uint32_t offsets_begin = rom.get_section(RomLabels::Rooms::DOOR_TABLE_SECTION).begin;
    uint32_t doors_begin = offsets_begin + result.first.size();
    auto data = std::make_shared<ByteVector>(result.first);
    data->insert(data->end(), result.second.begin(), result.second.end());
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::DOOR_OFFSET_TABLE, offsets_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::DOOR_TABLE, doors_begin));
    m_pending_writes.push_back({ RomLabels::Rooms::DOOR_TABLE_SECTION, data });
    return true;
}

bool RoomData::RomPrepareInjectGfxSwapData(const Rom& rom)
{
    auto flag_bytes = EncodeGfxSwap(m_gfxswap_flags);
    auto door_bytes = EncodeGfxSwap(m_gfxswap_locked_door_flags);
    auto tree_bytes = EncodeTreeWarp(m_gfxswap_big_tree_flags);
    auto table_bytes = m_gfxswaps.GetData();
    uint32_t flags_begin = rom.get_section(RomLabels::Rooms::GFX_SWAP_SECTION).begin;
    uint32_t doors_begin = flags_begin + flag_bytes.size();
    uint32_t trees_begin = doors_begin + door_bytes.size();
    uint32_t table_begin = trees_begin + tree_bytes.size();
    auto data = std::make_shared<ByteVector>(flag_bytes);
    data->insert(data->end(), door_bytes.begin(), door_bytes.end());
    data->insert(data->end(), tree_bytes.begin(), tree_bytes.end());
    data->insert(data->end(), table_bytes.begin(), table_bytes.end());
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::GFX_SWAP_FLAGS, flags_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::GFX_SWAP_LOCKED_DOOR_FLAGS_LEA1, doors_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::GFX_SWAP_LOCKED_DOOR_FLAGS_LEA2, doors_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::GFX_SWAP_TREE_FLAGS, trees_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::GFX_SWAP_TABLE, table_begin));
    m_pending_writes.push_back({ RomLabels::Rooms::GFX_SWAP_SECTION, data });
    return true;
}

bool RoomData::RomPrepareInjectMiscData(const Rom& rom)
{
    auto shop_bytes = std::make_shared<ByteVector>(EncodeFromSet(m_shop_list, true));
    auto lifestock_bytes = std::make_shared<ByteVector>(EncodeFromMap(m_lifestock_sold_flags));
    auto tree_bytes = std::make_shared<ByteVector>(EncodeFromSet(m_big_tree_list, false));
    auto lantern_bytes = std::make_shared<ByteVector>(EncodeFromMap(m_lantern_flag_list));
    uint32_t shops_begin = rom.get_section(RomLabels::Rooms::SHOP_LIST_SECTION).begin;
    uint32_t lifestock_begin = rom.get_section(RomLabels::Rooms::LIFESTOCK_SOLD_FLAGS_SECTION).begin;
    uint32_t trees_begin = rom.get_section(RomLabels::Rooms::BIG_TREE_LOCATIONS_SECTION).begin;
    uint32_t lantern_begin = rom.get_section(RomLabels::Rooms::LANTERN_ROOM_FLAGS_SECTION).begin;

    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::SHOP_LIST_LEA1, shops_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::SHOP_LIST_LEA2, shops_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::LIFESTOCK_SOLD_FLAGS, lifestock_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::BIG_TREE_LOCATIONS, trees_begin));
    m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Rooms::LANTERN_ROOM_FLAGS, lantern_begin));
    m_pending_writes.push_back({ RomLabels::Rooms::SHOP_LIST_SECTION, shop_bytes });
    m_pending_writes.push_back({ RomLabels::Rooms::LIFESTOCK_SOLD_FLAGS_SECTION, lifestock_bytes });
    m_pending_writes.push_back({ RomLabels::Rooms::BIG_TREE_LOCATIONS_SECTION, tree_bytes });
    m_pending_writes.push_back({ RomLabels::Rooms::LANTERN_ROOM_FLAGS_SECTION, lantern_bytes });
    return true;
}

void RoomData::UpdateTilesetRecommendedPalettes()
{
    std::vector<std::string> palettes;
    for (const auto& p : GetAllPalettes())
    {
        palettes.push_back(p.first);
    }
    std::unordered_map<uint8_t, std::unordered_map<uint8_t, int>> frequencies;
    for (const auto& r : m_roomlist)
    {
        frequencies[r->tileset][r->room_palette]++;
    }
    for (const auto& freqs : frequencies)
    {
        auto ts = GetTileset(freqs.first);
        std::vector<std::pair<uint8_t, int>> freqlist(freqs.second.cbegin(), freqs.second.cend());
        std::sort(freqlist.begin(), freqlist.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.second > rhs.second;
            });
        std::vector<std::string> recommended_pals;
        for (const auto& f : freqlist)
        {
            auto p = GetRoomPalette(f.first);
            recommended_pals.push_back(p->GetName());
        }
        ts->SetAllPalettes(palettes);
        ts->SetRecommendedPalettes(recommended_pals, true);
        for (const auto& a : m_animated_ts)
        {
            if (a.first.first == freqs.first)
            {
                a.second->SetAllPalettes(palettes);
                a.second->SetRecommendedPalettes(recommended_pals, true);
            }
        }
    }
}

void RoomData::ResetTilesetDefaultPalettes()
{
    for (const auto& ts : m_tilesets_by_name)
    {
        if (ts.second->GetRecommendedPalettes().size() == 0)
        {
            ts.second->SetDefaultPalette(m_room_pals.front()->GetName());
        }
        else
        {
            ts.second->SetDefaultPalette(ts.second->GetRecommendedPalettes().front());
        }
    }
}
