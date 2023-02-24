#ifndef _ROM_OFFSETS_H_
#define _ROM_OFFSETS_H_

#include <cstdint>
#include <unordered_map>

namespace RomOffsets
{

	struct Section
	{
		uint32_t begin;
		uint32_t end;
		inline uint32_t size() const
		{
			return end - begin;
		};
	};

	enum class Region
	{
		JP,
		US,
		UK,
		FR,
		DE,
		US_BETA,
		COUNT
	};

	inline const uint32_t CHECKSUM_ADDRESS  = 0x00018E;
	inline const uint32_t CHECKSUM_BEGIN    = 0x000200;
	inline const uint32_t BUILD_DATE_BEGIN  = 0x000202;
	inline const uint32_t BUILD_DATE_LENGTH = 0x00000E;
	inline const uint32_t EXPECTED_SIZE     = 0x200000;

	namespace Tilesets
	{
		static const std::string INTRO_FONT_NAME("IntroFont");
		static const std::string INTRO_FONT_PTR("IntroFontPtr");
		static const std::string DATA_LOC("TilesetData");
		static const std::string PTRTAB_LOC("TilesetPtrTable");
		static const std::string ANIM_DATA_LOC("AnimatedTilesetTable");
		static const std::string ANIM_LIST_LOC("AnimatedTilesetData");
		static const std::string ANIM_IDX_LOC("AnimatedTilesetIdxs");
		static const std::string PTRTAB_BEGIN_LOC("Tilesets");
		static const std::string PTR_LOC("TilesetPointers");
		static const std::string LABEL_FORMAT_STRING("Tileset%02d");
		static const std::string ANIM_LABEL_FORMAT_STRING("Tileset%02dAnim%02d");
		static const std::string ANIM_PTR_LABEL_FORMAT_STRING("Tileset%02dAnim%02dPtr");

		static const std::string ANIM_FILENAME_FORMAT_STRING("assets_packed/graphics/tilesets/animated/tileset%02dAnim%02d.bin");
		static const std::string FILENAME_FORMAT_STRING("assets_packed/graphics/tilesets/tileset%02d.lz77");
		static const std::string INTRO_FONT_FILENAME("assets_packed/graphics/fonts/introfont.bin");
		static const std::string INCLUDE_FILE("code/graphics/tileset_data.asm");
		static const std::string PTRTAB_FILE("code/pointertables/graphics/tilesetpointers.asm");
		static const std::string ANIM_FILE("code/maps/animtilesettbl.asm");
	}

	namespace Rooms
	{
		static const std::string ROOM_DATA_PTR("RoomDataPtr");
		static const std::string ROOM_DATA("RoomData_0");
		static const std::string MAP_DATA("RoomMaps");
		static const std::string ROOM_EXITS_PTR("RoomExitsPtr");
		static const std::string ROOM_EXITS("RoomExits");
		static const std::string MAP_FORMAT_STRING("Map%03d");

		static const std::string ROOM_DATA_FILE("code/pointertables/maps/roomlist.asm");
		static const std::string MAP_DATA_FILE("code/graphics/roommaps.asm");
		static const std::string MAP_FILENAME_FORMAT_STRING("assets_packed/maps/%s.cmp");
	}

	inline const std::unordered_map<Region, std::string> REGION_NAMES
	{
		{Region::JP,      "Japanese"},
		{Region::US,      "USA"     },
		{Region::UK,      "UK"      },
		{Region::FR,      "French"  },
		{Region::DE,      "German"  },
		{Region::US_BETA, "USA Beta"}
	};

	inline const std::unordered_map<std::string, Region> RELEASE_BUILD_DATE
	{
		{"92/09/18 14:30", Region::JP},
		{"93/07/13 20:03", Region::US},
		{"93/07/15 18:08", Region::UK},
		{"93/11/05 13:22", Region::FR},
		{"93/11/05 13:58", Region::DE},
		{"93/02/24 11:55", Region::US_BETA}
	};

	inline const std::unordered_map<std::string, std::unordered_map<Region, uint32_t>> ADDRESS
	{
		{"sprite_table_ptr",        {{Region::JP, 0x120000}, {Region::US, 0x120000}, {Region::UK, 0x120000}, {Region::FR, 0x120000}, {Region::DE, 0x120000}, {Region::US_BETA, 0x120000}}},
		{"sprite_data_end",         {{Region::JP, 0x1A4400}, {Region::US, 0x1A4400}, {Region::UK, 0x1A4400}, {Region::FR, 0x1A4400}, {Region::DE, 0x1A4400}, {Region::US_BETA, 0x1A4400}}},
		{"big_tiles_ptr",           {{Region::JP, 0x1AF800}, {Region::US, 0x1AF800}, {Region::UK, 0x1AF800}, {Region::FR, 0x1AF800}, {Region::DE, 0x1AF800}, {Region::US_BETA, 0x1AF800}}},
		{ Rooms::ROOM_DATA_PTR,     {{Region::JP, 0x0A0A00}, {Region::US, 0x0A0A00}, {Region::UK, 0x0A0A00}, {Region::FR, 0x0A0A00}, {Region::DE, 0x0A0A00}, {Region::US_BETA, 0x0A0A00}}},
		{"room_palette_ptr",        {{Region::JP, 0x0A0A04}, {Region::US, 0x0A0A04}, {Region::UK, 0x0A0A04}, {Region::FR, 0x0A0A04}, {Region::DE, 0x0A0A04}, {Region::US_BETA, 0x0A0A04}}},
		{ Rooms::ROOM_EXITS_PTR,    {{Region::JP, 0x0A0A08}, {Region::US, 0x0A0A08}, {Region::UK, 0x0A0A08}, {Region::FR, 0x0A0A08}, {Region::DE, 0x0A0A08}, {Region::US_BETA, 0x0A0A08}}},
		{ Tilesets::INTRO_FONT_PTR, {{Region::JP, 0x000000}, {Region::US, 0x00C528}, {Region::UK, 0x000000}, {Region::FR, 0x000000}, {Region::DE, 0x000000}, {Region::US_BETA, 0x000000}}}
	};

	inline const std::unordered_map<std::string, std::unordered_map<Region, Section>> SECTION
	{
		{"blockset_ptr_table",       {{Region::JP, {0x1AF804, 0x1AF8C8}}, {Region::US, {0x1AF804, 0x1AF8C8}}, {Region::UK, {0x1AF804, 0x1AF8C8}}, {Region::FR, {0x1AF804, 0x1AF8C8}}, {Region::DE, {0x1AF804, 0x1AF8C8}}, {Region::US_BETA, {0x1AF804, 0x1AF8C8}}}},
		{"tileset_offset_table",     {{Region::JP, {0x043E70, 0x043EF0}}, {Region::US, {0x044070, 0x0440F0}}, {Region::UK, {0x044070, 0x0440F0}}, {Region::FR, {0x044070, 0x0440F0}}, {Region::DE, {0x044070, 0x0440F0}}, {Region::US_BETA, {0x043E70, 0x043EF0}}}},
		{ Tilesets::DATA_LOC,        {{Region::JP, {0x043E70, 0x000000}}, {Region::US, {0x044010, 0x09B000}}, {Region::UK, {0x044010, 0x09B000}}, {Region::FR, {0x044010, 0x09B000}}, {Region::DE, {0x044010, 0x09B000}}, {Region::US_BETA, {0x043E70, 0x000000}}}},
		{ Tilesets::ANIM_DATA_LOC,   {{Region::JP, {0x000000, 0x000000}}, {Region::US, {0x009E04, 0x009EF8}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"sprite_gfx_lookup",        {{Region::JP, {0x01ABBE, 0x01AD96}}, {Region::US, {0x01ABF2, 0x01ADCA}}, {Region::UK, {0x01ABF2, 0x01ADCA}}, {Region::FR, {0x01ABE6, 0x01ADBE}}, {Region::DE, {0x01ABEC, 0x01ADC4}}, {Region::US_BETA, {0x01ABB8, 0x01AD90}}}},
		{"sprite_gfx_offset_table",  {{Region::JP, {0x120004, 0x1201B4}}, {Region::US, {0x120004, 0x1201B4}}, {Region::UK, {0x120004, 0x1201B4}}, {Region::FR, {0x120004, 0x1201B4}}, {Region::DE, {0x120004, 0x1201B4}}, {Region::US_BETA, {0x120004, 0x1201B4}}}},
		{"sprite_palette_lookup",    {{Region::JP, {0x1A453A, 0x1A47E0}}, {Region::US, {0x1A453A, 0x1A47E0}}, {Region::UK, {0x1A453A, 0x1A47E0}}, {Region::FR, {0x1A453A, 0x1A47E0}}, {Region::DE, {0x1A453A, 0x1A47E0}}, {Region::US_BETA, {0x1A453A, 0x1A47E0}}}},
		{"room_exits",               {{Region::JP, {0x11CEF0, 0x11EAB0}}, {Region::US, {0x11CEA2, 0x11EA62}}, {Region::UK, {0x11CEA2, 0x11EA62}}, {Region::FR, {0x11CEA2, 0x11EA62}}, {Region::DE, {0x11CEA2, 0x11EA62}}, {Region::US_BETA, {0x11CE80, 0x11EA40}}}},
		{"compressed_string_banks",  {{Region::JP, {0x038532, 0x038552}}, {Region::US, {0x038368, 0x03838C}}, {Region::UK, {0x038368, 0x03838C}}, {Region::FR, {0x038216, 0x03822E}}, {Region::DE, {0x037D14, 0x037D2C}}, {Region::US_BETA, {0x0383EE, 0x038412}}}},
		{"huff_table_offsets",       {{Region::JP, {0x023C94, 0x023E8E}}, {Region::US, {0x023D60, 0x023E38}}, {Region::UK, {0x023D60, 0x023E38}}, {Region::FR, {0x023D76, 0x023E6C}}, {Region::DE, {0x023D8A, 0x023E3A}}, {Region::US_BETA, {0x023CBC, 0x023D94}}}},
		{"huff_tables",              {{Region::JP, {0x023E8E, 0x025562}}, {Region::US, {0x023E38, 0x02469C}}, {Region::UK, {0x023E38, 0x02469C}}, {Region::FR, {0x023E6C, 0x024732}}, {Region::DE, {0x023E3A, 0x024412}}, {Region::US_BETA, {0x023D94, 0x0245C4}}}},
		{"character_name_table",     {{Region::JP, {0x02A2EA, 0x02A3E4}}, {Region::US, {0x029552, 0x0296E0}}, {Region::UK, {0x029552, 0x0296E0}}, {Region::FR, {0x0291B2, 0x02936E}}, {Region::DE, {0x028E98, 0x029078}}, {Region::US_BETA, {0x029444, 0x0295D2}}}},
		{"special_char_name_table",  {{Region::JP, {0x02A3E4, 0x02A438}}, {Region::US, {0x0296E0, 0x029730}}, {Region::UK, {0x0296E0, 0x029730}}, {Region::FR, {0x02936E, 0x0293BE}}, {Region::DE, {0x029078, 0x0290C8}}, {Region::US_BETA, {0x0295D2, 0x029622}}}},
		{"default_char_name_table",  {{Region::JP, {0x02A438, 0x02A43A}}, {Region::US, {0x029730, 0x029732}}, {Region::UK, {0x029730, 0x029732}}, {Region::FR, {0x0293BE, 0x0293C0}}, {Region::DE, {0x0290C8, 0x0290CA}}, {Region::US_BETA, {0x029622, 0x029624}}}},
		{"item_name_table",          {{Region::JP, {0x02A43A, 0x02A648}}, {Region::US, {0x029732, 0x029A0A}}, {Region::UK, {0x029732, 0x029A0A}}, {Region::FR, {0x0293C0, 0x0296FC}}, {Region::DE, {0x0290CA, 0x0293EC}}, {Region::US_BETA, {0x029624, 0x0298F8}}}},
		{"menu_string_table",        {{Region::JP, {0x02A648, 0x02A700}}, {Region::US, {0x029A0A, 0x029ACC}}, {Region::UK, {0x029A0A, 0x029ACC}}, {Region::FR, {0x0296FC, 0x0297D4}}, {Region::DE, {0x0293EC, 0x0295DA}}, {Region::US_BETA, {0x0298F8, 0x0299BA}}}},
		{"intro_string_pointers",    {{Region::JP, {0x00C538, 0x00C568}}, {Region::US, {0x00C59E, 0x00C5CE}}, {Region::UK, {0x00C59E, 0x00C5CE}}, {Region::FR, {0x00C59E, 0x00C5CE}}, {Region::DE, {0x00C5A8, 0x00C5D8}}, {Region::US_BETA, {0x00C546, 0x00C576}}}},
		{"end_credit_strings",       {{Region::JP, {0x09ED12, 0x09F62D}}, {Region::US, {0x09ED1A, 0x09F644}}, {Region::UK, {0x09ED1A, 0x09F49C}}, {Region::FR, {0x09ED1A, 0x09F473}}, {Region::DE, {0x09ED1A, 0x09F472}}, {Region::US_BETA, {0x09ED1A, 0x09F635}}}},
		// Images
		{"lithograph_tiles",         {{Region::JP, {0x038AB8, 0x03973E}}, {Region::US, {0x038ACC, 0x039752}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"lithograph_map",           {{Region::JP, {0x03973E, 0x03974E}}, {Region::US, {0x039752, 0x039762}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"islemap_bg_tiles",         {{Region::JP, {0x041A19, 0x043B5C}}, {Region::US, {0x041B41, 0x043C84}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"islemap_bg_map",           {{Region::JP, {0x043B5C, 0x043D2A}}, {Region::US, {0x043C84, 0x043D2A}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"islemap_fg_tiles",         {{Region::JP, {0x03EC72, 0x0419D9}}, {Region::US, {0x03ED9A, 0x041B01}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"islemap_fg_map",           {{Region::JP, {0x0419D9, 0x041A19}}, {Region::US, {0x041B01, 0x041B41}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title1_tiles",             {{Region::JP, {0x039EC4, 0x03A550}}, {Region::US, {0x039ED8, 0x03A564}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title1_map",               {{Region::JP, {0x03DC70, 0x03DCDC}}, {Region::US, {0x03DD92, 0x03DDFE}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title2_tiles",             {{Region::JP, {0x03A550, 0x03BC9A}}, {Region::US, {0x03A564, 0x03BCAE}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title2_map",               {{Region::JP, {0x03DCDC, 0x03DD30}}, {Region::US, {0x03DDFE, 0x03DE52}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title3_tiles",             {{Region::JP, {0x03BC9A, 0x03DC70}}, {Region::US, {0x03BCAE, 0x03DD92}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title3_map",               {{Region::JP, {0x03DD30, 0x03DDA2}}, {Region::US, {0x03DE52, 0x03DECA}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"sega_logo_tiles",          {{Region::JP, {0x0386E4, 0x0389C0}}, {Region::US, {0x0386E4, 0x0389D4}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"climax_logo_tiles",        {{Region::JP, {0x03DE62, 0x03E4F9}}, {Region::US, {0x03DF8A, 0x03E621}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"climax_logo_map",          {{Region::JP, {0x03E4F9, 0x03E524}}, {Region::US, {0x03E621, 0x03E64C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"ending_logo_tiles",        {{Region::JP, {0x0A0014, 0x0A083F}}, {Region::US, {0x0A0033, 0x0A086F}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"ending_logo_map",          {{Region::JP, {0x0A083F, 0x0A087B}}, {Region::US, {0x0A086F, 0x0A08AB}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"hud_tiles",                {{Region::JP, {0x0091DC, 0x009546}}, {Region::US, {0x009242, 0x0095AC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"hud_map",                  {{Region::JP, {0x0090EC, 0x0091DC}}, {Region::US, {0x009152, 0x009242}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"inventory_map",            {{Region::JP, {0x0079E2, 0x007AB6}}, {Region::US, {0x007A3C, 0x007B10}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"textbox2l_map",            {{Region::JP, {0x023AB4, 0x023C94}}, {Region::US, {0x023B80, 0x023D60}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"textbox3l_map",            {{Region::JP, {0x023834, 0x023AB4}}, {Region::US, {0x023900, 0x023B80}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"loadgame_tiles",           {{Region::JP, {0x00FEC8, 0x00FFA3}}, {Region::US, {0x00FD5C, 0x00FE37}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"loadgame_chrtiles",        {{Region::JP, {0x00FCBE, 0x00FEC8}}, {Region::US, {0x00FB52, 0x00FD5C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"loadgame_map",             {{Region::JP, {0x00FFA3, 0x01007E}}, {Region::US, {0x00FE37, 0x00FF12}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},

		// Palettes
		{"lava_palettes",            {{Region::JP, {0x002608, 0x002628}}, {Region::US, {0x00260E, 0x00262E}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"labrynth_lantern_palette", {{Region::JP, {0x002DAE, 0x002DC8}}, {Region::US, {0x002DB4, 0x002DCE}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"equipped_sword_palette",   {{Region::JP, {0x0078A6, 0x0078BE}}, {Region::US, {0x007900, 0x007918}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"equipped_armour_palette",  {{Region::JP, {0x0078BE, 0x0078D2}}, {Region::US, {0x007918, 0x00792C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"player_palette",           {{Region::JP, {0x008FB6, 0x008FD6}}, {Region::US, {0x00901C, 0x00903C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"hud_palette",              {{Region::JP, {0x008FD6, 0x008FE0}}, {Region::US, {0x00903C, 0x009046}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"inventory_palette_1_8col", {{Region::JP, {0x00E25C, 0x00E26C}}, {Region::US, {0x00E0E0, 0x00E0F0}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"inventory_palette_2",      {{Region::JP, {0x00E26C, 0x00E28C}}, {Region::US, {0x00E0F0, 0x00E110}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"inventory_palette_3",      {{Region::JP, {0x00783C, 0x00785C}}, {Region::US, {0x007896, 0x0078B6}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"kazalt_warp_palette",      {{Region::JP, {0x00E98A, 0x00E9AE}}, {Region::US, {0x00E80E, 0x00E832}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"new_game_palette",         {{Region::JP, {0x00F828, 0x00F848}}, {Region::US, {0x00F6BC, 0x00F6DC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"new_game_player_palette",  {{Region::JP, {0x00FC9E, 0x00FCBE}}, {Region::US, {0x00FB32, 0x00FB52}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"sega_logo_palette",        {{Region::JP, {0x0386C2, 0x0386D0}}, {Region::US, {0x0386C2, 0x0386D0}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"lithograph_palette",       {{Region::JP, {0x038A98, 0x038AB8}}, {Region::US, {0x038AAC, 0x038ACC}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_blue_palette",   {{Region::JP, {0x039AF4, 0x039B2C}}, {Region::US, {0x039B04, 0x039B3C}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_yellow_pallette",{{Region::JP, {0x039C2C, 0x039C36}}, {Region::US, {0x039C3C, 0x039C46}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_single_colours", {{Region::JP, {0x039C90, 0x039C92}}, {Region::US, {0x039CA0, 0x039CA4}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"title_seq_palette",        {{Region::JP, {0x03DDA2, 0x03DDE2}}, {Region::US, {0x03DECA, 0x03DF0A}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"climax_logo_palette",      {{Region::JP, {0x03E524, 0x03E52C}}, {Region::US, {0x03E64C, 0x03E654}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"island_map_palette",       {{Region::JP, {0x043D06, 0x043D46}}, {Region::US, {0x043E2E, 0x043E6E}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"end_credits_palette",      {{Region::JP, {0x09FA24, 0x09FA2C}}, {Region::US, {0x09FA3A, 0x09FA42}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"projectiles_palette_1",    {{Region::JP, {0x1AC468, 0x1AC474}}, {Region::US, {0x1AC468, 0x1AC474}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"projectiles_palette_2",    {{Region::JP, {0x1AF5F2, 0x1AF5FA}}, {Region::US, {0x1AF5F2, 0x1AF5FA}}, {Region::UK, {0x000000, 0x000000}}, {Region::FR, {0x000000, 0x000000}}, {Region::DE, {0x000000, 0x000000}}, {Region::US_BETA, {0x000000, 0x000000}}}},
		{"sprite_low_palettes",      {{Region::JP, {0x1A47E0, 0x1A4BA0}}, {Region::US, {0x1A47E0, 0x1A4BA0}}, {Region::UK, {0x1A47E0, 0x1A4BA0}}, {Region::FR, {0x1A47E0, 0x1A4BA0}}, {Region::DE, {0x1A47E0, 0x1A4BA0}}, {Region::US_BETA, {0x1A47E0, 0x1A4BA0}}}},
		{"sprite_high_palettes",     {{Region::JP, {0x1A4BA0, 0x1A4C8E}}, {Region::US, {0x1A4BA0, 0x1A4C8E}}, {Region::UK, {0x1A4BA0, 0x1A4C8E}}, {Region::FR, {0x1A4BA0, 0x1A4C8E}}, {Region::DE, {0x1A4BA0, 0x1A4C8E}}, {Region::US_BETA, {0x1A4BA0, 0x1A4C8E}}}},
		{"room_palettes",            {{Region::JP, {0x11C974, 0x11CEF0}}, {Region::US, {0x11C926, 0x11CEA2}}, {Region::UK, {0x11C926, 0x11CEA2}}, {Region::FR, {0x11C926, 0x11CEA2}}, {Region::DE, {0x11C926, 0x11CEA2}}, {Region::US_BETA, {0x11C904, 0x11CE80}}}}
	};
} // namespace RomOffsets

#endif // _ROM_OFFSETS_H_
