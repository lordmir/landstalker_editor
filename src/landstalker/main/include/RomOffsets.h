#ifndef _ROM_OFFSETS_H_
#define _ROM_OFFSETS_H_

#include <cstdint>
#include <unordered_map>
#include <string>

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

	extern const uint32_t CHECKSUM_ADDRESS;
	extern const uint32_t CHECKSUM_BEGIN;
	extern const uint32_t BUILD_DATE_BEGIN;
	extern const uint32_t BUILD_DATE_LENGTH;
	extern const uint32_t EXPECTED_SIZE;

	extern const std::unordered_map<Region, std::string> REGION_NAMES;
	extern const std::unordered_map<std::string, Region> RELEASE_BUILD_DATE;

	extern const std::unordered_map<std::string, std::unordered_map<Region, uint32_t>> ADDRESS;
	extern const std::unordered_map<std::string, std::unordered_map<Region, Section>> SECTION;
} // namespace RomOffsets

#endif // _ROM_OFFSETS_H_
