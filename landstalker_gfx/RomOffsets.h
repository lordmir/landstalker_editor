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

	inline const uint32_t CHECKSUM_ADDRESS = 0x00018E;
	inline const uint32_t CHECKSUM_BEGIN   = 0x000200;
	inline const uint32_t EXPECTED_SIZE    = 0x200000;

	inline const std::unordered_map<Region, std::string> REGION_NAMES
	{
		{Region::JP,      "Japan"   },
		{Region::US,      "USA"     },
		{Region::UK,      "UK"      },
		{Region::FR,      "France"  },
		{Region::DE,      "Germany" },
		{Region::US_BETA, "USA Beta"}
	};

	inline const std::unordered_map<uint16_t, Region> RELEASE_CHECKSUM
	{
		{0xF37C, Region::US}
	};

	inline const std::unordered_map<std::string, std::unordered_map<Region, uint32_t>> ADDRESS
	{
		{"sprite_table_ptr",      {{Region::US, 0x120000}}},
		{"big_tiles_ptr",         {{Region::US, 0x1AF800}}},
		{"room_data_ptr",         {{Region::US, 0x0A0A00}}},
		{"room_palette_ptr",      {{Region::US, 0x0A0A04}}},
		{"room_exit_table_ptr",   {{Region::US, 0x0A0A08}}}
	};

	inline const std::unordered_map<std::string, std::unordered_map<Region, Section>> SECTION
	{
		{"sprite_gfx_lookup",       {{Region::US, {0x01ABF2, 0x01ADCA}}}},
		{"sprite_palette_lookup",   {{Region::US, {0x1A453A, 0x1A47E0}}}},
		{"big_tiles_ptr_table",     {{Region::US, {0x1AF804, 0x1AF8C8}}}},
		{"tileset_offset_table",    {{Region::US, {0x044070, 0x0440F0}}}},
		{"sprite_gfx_offset_table", {{Region::US, {0x120004, 0x1201B4}}}},
		{"sprite_high_palettes",    {{Region::US, {0x1A4BA0, 0x1A4C8E}}}},
		{"sprite_low_palettes",     {{Region::US, {0x1A47E0, 0x1A4BA0}}}},
		{"room_palettes",           {{Region::US, {0x11C926, 0x11CEA2}}}},
		{"room_exits",              {{Region::US, {0x11CEA2, 0x11EA62}}}}
	};
} // namespace RomOffsets

#endif // _ROM_OFFSETS_H_
