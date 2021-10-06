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
		{"room_exits",              {{Region::JP, {0x11CEF0, 0x11EAB0}}, {Region::US, {0x11CEA2, 0x11EA62}}, {Region::UK, {0x11CEA2, 0x11EA62}}, {Region::FR, {0x11CEA2, 0x11EA62}}, {Region::DE, {0x11CEA2, 0x11EA62}}, {Region::US_BETA, {0x11CE80, 0x11EA40}}}}
	};
} // namespace RomOffsets

#endif // _ROM_OFFSETS_H_
