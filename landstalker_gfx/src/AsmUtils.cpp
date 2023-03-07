#include <AsmUtils.h>

uint32_t Asm::LEA_PCRel(AReg reg, uint32_t pc, uint32_t loc)
{
	uint32_t ins = 0x41C00000;
	uint32_t mode = 0b111010 << 16; // 16-bit PC relative addressing
	uint16_t offset = 0;
	uint32_t regbits = static_cast<uint32_t>(reg) << 25;
	pc += 2;
	if (loc > pc)
	{
		offset = loc - pc;
	}
	else
	{
		offset = ~(pc - loc) + 1;
	}
	return ins | regbits | mode | offset;
}
