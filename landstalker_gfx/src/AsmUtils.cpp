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

uint32_t Asm::MOVE_DOffset_PCRel(Width width, DReg reg, uint32_t pc, uint32_t loc)
{
	uint32_t ins = 0x00 << 30;
	uint32_t w = static_cast<uint32_t>(width) << 28;
	uint32_t dest_mode = 0b001111 << 22; // Absolute long
	uint32_t src_mode = 0b111011 << 16; // PC-Relative with index and 8-bit offset
	uint32_t regbits = static_cast<uint32_t>(reg) << 12;
	uint32_t index_mode = 0b0000 << 8; // Word, no scale
	uint8_t offset = 0;
	pc += 2;
	if (loc > pc)
	{
		offset = loc - pc;
	}
	else
	{
		offset = ~(pc - loc) + 1;
	}
	return ins | w | dest_mode | src_mode | regbits | index_mode | offset;
}

uint32_t Disasm::LEA_PCRel(uint32_t ins, uint32_t pc)
{
	int16_t offset = ins & 0xFFFF;
	return pc + offset + 2;
}

uint32_t Disasm::MOVE_DOffset_PCRel(uint32_t ins, uint32_t pc)
{
	int8_t offset = ins & 0xFF;
	return pc + offset + 2;
}
