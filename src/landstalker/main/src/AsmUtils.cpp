#include <landstalker/main/include/AsmUtils.h>
#include <memory>

uint16_t Asm::PCRel16(uint32_t pc, uint32_t loc)
{
	uint16_t offset;
	pc += 2;
	if (loc > pc)
	{
		offset = static_cast<uint16_t>(loc - pc);
	}
	else
	{
		offset = static_cast<uint16_t>(~(pc - loc) + 1);
	}
	return offset;
}

uint8_t Asm::PCRel8(uint32_t pc, uint32_t loc)
{
	uint8_t offset;
	pc += 2;
	if (loc > pc)
	{
		offset = static_cast<uint8_t>(loc - pc);
	}
	else
	{
		offset = static_cast<uint8_t>(~(pc - loc) + 1);
	}
	return offset;
}

std::pair<std::string, ByteVectorPtr> Asm::WriteAddress32(const std::string& pc, uint32_t addr)
{
	return { pc, std::make_shared<ByteVector>(Split<uint8_t>(addr)) };
}

std::pair<std::string, ByteVectorPtr> Asm::WriteOffset16(const Rom& rom, const std::string& pc, uint32_t addr)
{
	uint32_t ins = rom.read<uint32_t>(pc);
	uint16_t offset = PCRel16(rom.get_address(pc), addr);
	ins &= 0xFFFF0000;
	ins |= offset;
	return { pc, std::make_shared<ByteVector>(Split<uint8_t>(ins)) };
}

std::pair<std::string, ByteVectorPtr> Asm::WriteOffset8(const Rom& rom, const std::string& pc, uint32_t addr)
{
	uint32_t ins = rom.read<uint32_t>(pc);
	uint8_t offset = PCRel8(rom.get_address(pc), addr);
	ins &= 0xFFFFFF00;
	ins |= offset;
	return { pc, std::make_shared<ByteVector>(Split<uint8_t>(ins)) };
}

uint32_t Disasm::PCRel16(uint32_t ins, uint32_t pc)
{
	int16_t offset = ins & 0xFFFF;
	return pc + offset + 2;
}

uint32_t Disasm::PCRel8(uint32_t ins, uint32_t pc)
{
	int8_t offset = ins & 0xFF;
	return pc + offset + 2;
}

uint32_t Disasm::ReadOffset16(const Rom& rom, const std::string& pc)
{
	return Disasm::PCRel16(rom.read<uint32_t>(pc), rom.get_address(pc));
}

uint32_t Disasm::ReadOffset8(const Rom& rom, const std::string& pc)
{
	return Disasm::PCRel8(rom.read<uint32_t>(pc), rom.get_address(pc));
}
