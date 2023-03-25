#ifndef _ASM_UTILS_H_
#define _ASM_UTILS_H_

#include <cstdint>

enum class DReg
{
	D0 = 0,
	D1 = 1,
	D2 = 2,
	D3 = 3,
	D4 = 4,
	D5 = 5,
	D6 = 6,
	D7 = 7
};

enum class AReg
{
	A0 = 0,
	A1 = 1,
	A2 = 2,
	A3 = 3,
	A4 = 4,
	A5 = 5,
	A6 = 6,
	SP = 7
};

enum class Width
{
	B = 1,
	W = 3,
	L = 2
};

namespace Asm
{
	uint32_t LEA_PCRel(AReg reg, uint32_t pc, uint32_t loc);
	uint32_t MOVE_DOffset_PCRel(Width width, DReg reg, uint32_t pc, uint32_t loc);
}

namespace Disasm
{
	uint32_t LEA_PCRel(uint32_t ins, uint32_t pc);
	uint32_t MOVE_DOffset_PCRel(uint32_t ins, uint32_t pc);
}

#endif // _ASM_UTILS_H_
