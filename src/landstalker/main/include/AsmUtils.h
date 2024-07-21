#ifndef _ASM_UTILS_H_
#define _ASM_UTILS_H_

#include <cstdint>
#include <landstalker/main/include/Rom.h>
#include <landstalker/main/include/DataManager.h>

namespace Asm
{
	uint16_t PCRel16(uint32_t pc, uint32_t loc);
	uint8_t PCRel8(uint32_t pc, uint32_t loc);
	std::pair<std::string, ByteVectorPtr> WriteAddress32(const std::string& pc, uint32_t addr);
	std::pair<std::string, ByteVectorPtr> WriteOffset16(const Rom& rom, const std::string& pc, uint32_t addr);
	std::pair<std::string, ByteVectorPtr> WriteOffset8(const Rom& rom, const std::string& pc, uint32_t addr);
}

namespace Disasm
{
	uint32_t PCRel16(uint32_t ins, uint32_t pc);
	uint32_t PCRel8(uint32_t ins, uint32_t pc);
	uint32_t ReadOffset16(const Rom& rom, const std::string& pc);
	uint32_t ReadOffset8(const Rom& rom, const std::string& pc);
}

#endif // _ASM_UTILS_H_
