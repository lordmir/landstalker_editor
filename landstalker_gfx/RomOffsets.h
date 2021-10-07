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
		{"sprite_table_ptr",      {{Region::JP, 0x120000}, {Region::US, 0x120000}, {Region::UK, 0x120000}, {Region::FR, 0x120000}, {Region::DE, 0x120000}, {Region::US_BETA, 0x120000}}},
		{"big_tiles_ptr",         {{Region::JP, 0x1AF800}, {Region::US, 0x1AF800}, {Region::UK, 0x1AF800}, {Region::FR, 0x1AF800}, {Region::DE, 0x1AF800}, {Region::US_BETA, 0x1AF800}}},
		{"room_data_ptr",         {{Region::JP, 0x0A0A00}, {Region::US, 0x0A0A00}, {Region::UK, 0x0A0A00}, {Region::FR, 0x0A0A00}, {Region::DE, 0x0A0A00}, {Region::US_BETA, 0x0A0A00}}},
		{"room_palette_ptr",      {{Region::JP, 0x0A0A04}, {Region::US, 0x0A0A04}, {Region::UK, 0x0A0A04}, {Region::FR, 0x0A0A04}, {Region::DE, 0x0A0A04}, {Region::US_BETA, 0x0A0A04}}},
		{"room_exit_table_ptr",   {{Region::JP, 0x0A0A08}, {Region::US, 0x0A0A08}, {Region::UK, 0x0A0A08}, {Region::FR, 0x0A0A08}, {Region::DE, 0x0A0A08}, {Region::US_BETA, 0x0A0A08}}}
	};

	inline const std::unordered_map<std::string, std::unordered_map<Region, Section>> SECTION
	{
		{"big_tiles_ptr_table",     {{Region::JP, {0x1AF804, 0x1AF8C8}}, {Region::US, {0x1AF804, 0x1AF8C8}}, {Region::UK, {0x1AF804, 0x1AF8C8}}, {Region::FR, {0x1AF804, 0x1AF8C8}}, {Region::DE, {0x1AF804, 0x1AF8C8}}, {Region::US_BETA, {0x1AF804, 0x1AF8C8}}}},
		{"tileset_offset_table",    {{Region::JP, {0x043E70, 0x043EF0}}, {Region::US, {0x044070, 0x0440F0}}, {Region::UK, {0x044070, 0x0440F0}}, {Region::FR, {0x044070, 0x0440F0}}, {Region::DE, {0x044070, 0x0440F0}}, {Region::US_BETA, {0x043E70, 0x043EF0}}}},
		{"sprite_gfx_lookup",       {{Region::JP, {0x01ABBE, 0x01AD96}}, {Region::US, {0x01ABF2, 0x01ADCA}}, {Region::UK, {0x01ABF2, 0x01ADCA}}, {Region::FR, {0x01ABE6, 0x01ADBE}}, {Region::DE, {0x01ABEC, 0x01ADC4}}, {Region::US_BETA, {0x01ABB8, 0x01AD90}}}},
		{"sprite_gfx_offset_table", {{Region::JP, {0x120004, 0x1201B4}}, {Region::US, {0x120004, 0x1201B4}}, {Region::UK, {0x120004, 0x1201B4}}, {Region::FR, {0x120004, 0x1201B4}}, {Region::DE, {0x120004, 0x1201B4}}, {Region::US_BETA, {0x120004, 0x1201B4}}}},
		{"sprite_palette_lookup",   {{Region::JP, {0x1A453A, 0x1A47E0}}, {Region::US, {0x1A453A, 0x1A47E0}}, {Region::UK, {0x1A453A, 0x1A47E0}}, {Region::FR, {0x1A453A, 0x1A47E0}}, {Region::DE, {0x1A453A, 0x1A47E0}}, {Region::US_BETA, {0x1A453A, 0x1A47E0}}}},
		{"sprite_low_palettes",     {{Region::JP, {0x1A47E0, 0x1A4BA0}}, {Region::US, {0x1A47E0, 0x1A4BA0}}, {Region::UK, {0x1A47E0, 0x1A4BA0}}, {Region::FR, {0x1A47E0, 0x1A4BA0}}, {Region::DE, {0x1A47E0, 0x1A4BA0}}, {Region::US_BETA, {0x1A47E0, 0x1A4BA0}}}},
		{"sprite_high_palettes",    {{Region::JP, {0x1A4BA0, 0x1A4C8E}}, {Region::US, {0x1A4BA0, 0x1A4C8E}}, {Region::UK, {0x1A4BA0, 0x1A4C8E}}, {Region::FR, {0x1A4BA0, 0x1A4C8E}}, {Region::DE, {0x1A4BA0, 0x1A4C8E}}, {Region::US_BETA, {0x1A4BA0, 0x1A4C8E}}}},
		{"room_palettes",           {{Region::JP, {0x11C974, 0x11CEF0}}, {Region::US, {0x11C926, 0x11CEA2}}, {Region::UK, {0x11C926, 0x11CEA2}}, {Region::FR, {0x11C926, 0x11CEA2}}, {Region::DE, {0x11C926, 0x11CEA2}}, {Region::US_BETA, {0x11C904, 0x11CE80}}}},
		{"room_exits",              {{Region::JP, {0x11CEF0, 0x11EAB0}}, {Region::US, {0x11CEA2, 0x11EA62}}, {Region::UK, {0x11CEA2, 0x11EA62}}, {Region::FR, {0x11CEA2, 0x11EA62}}, {Region::DE, {0x11CEA2, 0x11EA62}}, {Region::US_BETA, {0x11CE80, 0x11EA40}}}},
		{"compressed_string_banks", {{Region::JP, {0x038532, 0x038552}}, {Region::US, {0x038368, 0x03838C}}, {Region::UK, {0x038368, 0x03838C}}, {Region::FR, {0x038216, 0x03822E}}, {Region::DE, {0x037D14, 0x037D2C}}, {Region::US_BETA, {0x0383EE, 0x038412}}}},
		{"huff_table_offsets",      {{Region::JP, {0x023C94, 0x023E8E}}, {Region::US, {0x023D60, 0x023E38}}, {Region::UK, {0x023D60, 0x023E38}}, {Region::FR, {0x023D76, 0x023E6C}}, {Region::DE, {0x023D8A, 0x023E3A}}, {Region::US_BETA, {0x023CBC, 0x023D94}}}},
		{"huff_tables",             {{Region::JP, {0x023E8E, 0x025562}}, {Region::US, {0x023E38, 0x02469C}}, {Region::UK, {0x023E38, 0x02469C}}, {Region::FR, {0x023E6C, 0x024732}}, {Region::DE, {0x023E3A, 0x024412}}, {Region::US_BETA, {0x023D94, 0x0245C4}}}},
		{"character_name_table",    {{Region::JP, {0x02A2EA, 0x02A3E4}}, {Region::US, {0x029552, 0x0296E0}}, {Region::UK, {0x029552, 0x0296E0}}, {Region::FR, {0x0291B2, 0x02936E}}, {Region::DE, {0x028E98, 0x029078}}, {Region::US_BETA, {0x029444, 0x0295D2}}}},
		{"special_char_name_table", {{Region::JP, {0x02A3E4, 0x02A438}}, {Region::US, {0x0296E0, 0x029730}}, {Region::UK, {0x0296E0, 0x029730}}, {Region::FR, {0x02936E, 0x0293BE}}, {Region::DE, {0x029078, 0x0290C8}}, {Region::US_BETA, {0x0295D2, 0x029622}}}},
		{"default_char_name_table", {{Region::JP, {0x02A438, 0x02A43A}}, {Region::US, {0x029730, 0x029732}}, {Region::UK, {0x029730, 0x029732}}, {Region::FR, {0x0293BE, 0x0293C0}}, {Region::DE, {0x0290C8, 0x0290CA}}, {Region::US_BETA, {0x029622, 0x029624}}}},
		{"item_name_table",         {{Region::JP, {0x02A43A, 0x02A648}}, {Region::US, {0x029732, 0x029A0A}}, {Region::UK, {0x029732, 0x029A0A}}, {Region::FR, {0x0293C0, 0x0296FC}}, {Region::DE, {0x0290CA, 0x0293EC}}, {Region::US_BETA, {0x029624, 0x0298F8}}}},
		{"menu_string_table",       {{Region::JP, {0x02A648, 0x02A700}}, {Region::US, {0x029A0A, 0x029ACC}}, {Region::UK, {0x029A0A, 0x029ACC}}, {Region::FR, {0x0296FC, 0x0297D4}}, {Region::DE, {0x0293EC, 0x0295DA}}, {Region::US_BETA, {0x0298F8, 0x0299BA}}}},
		{"intro_string_pointers",   {{Region::JP, {0x00C538, 0x00C568}}, {Region::US, {0x00C59E, 0x00C5CE}}, {Region::UK, {0x00C59E, 0x00C5CE}}, {Region::FR, {0x00C59E, 0x00C5CE}}, {Region::DE, {0x00C5A8, 0x00C5D8}}, {Region::US_BETA, {0x00C546, 0x00C576}}}},
		{"end_credit_strings",      {{Region::JP, {0x09ED12, 0x09F62D}}, {Region::US, {0x09ED1A, 0x09F644}}, {Region::UK, {0x09ED1A, 0x09F474}}, {Region::FR, {0x09ED1A, 0x09F473}}, {Region::DE, {0x09ED1A, 0x09F472}}, {Region::US_BETA, {0x09ED1A, 0x09F635}}}}
	};
} // namespace RomOffsets

#endif // _ROM_OFFSETS_H_
