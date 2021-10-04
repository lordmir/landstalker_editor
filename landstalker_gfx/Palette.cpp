#include "Palette.h"

template<std::size_t N>
static void PopulatePalettes(const std::vector<uint16_t>& input, std::vector<std::array<uint16_t, N>>& output)
{
	// Input must contain a whole number of palettes
	assert(input.size() % N == 0);

	size_t i = 0;
	for (auto in : input)
	{
		std::array<uint16_t, N> pal;
		pal[i++] = in;
		if (i == N)
		{
			output.push_back(pal);
			i = 0;
		}
	}
}

template <std::size_t N>
static void LoadPalette(const std::array<uint16_t, N>& palette, size_t begin, std::array<PaletteEntry, 16> & outpal)
{
	size_t i = 0;
	for (auto colour : palette)
	{
		outpal[begin + i++] = Palette::FromGenesisColour(colour, (i + begin) == 0);
	}
}

Palette::Palette()
{
	Clear();
}

Palette::Palette(const Rom& rom)
	: Palette()
{
	if (!m_cache_init)
	{
		PopulatePalettes(rom.read_array<uint16_t>("room_palettes"), m_room_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("sprite_high_palettes"), m_sprite_high_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("sprite_low_palettes"), m_sprite_low_palettes);
		m_cache_init = true;
	}
}

void Palette::Clear()
{
	m_pal[0] = FromGenesisColour(0x0000, true);
	m_pal[1] = FromGenesisColour(0x0CCC);

	for (size_t i = 2; i < 16; ++i)
	{
		m_pal[i] = FromGenesisColour(0x0000);
	}
}

void Palette::LoadDebugPal()
{
	m_pal[0] = FromGenesisColour(0x0C0C);
	m_pal[1] = FromGenesisColour(0x0CCC);
	m_pal[2] = FromGenesisColour(0x000E);
	m_pal[3] = FromGenesisColour(0x00E0);
	m_pal[4] = FromGenesisColour(0x00EE);
	m_pal[5] = FromGenesisColour(0x0E00);
	m_pal[6] = FromGenesisColour(0x0EE0);
	m_pal[7] = FromGenesisColour(0x0EEE);
	m_pal[8] = FromGenesisColour(0x0888);
	m_pal[9] = FromGenesisColour(0x0008);
	m_pal[10] = FromGenesisColour(0x0080);
	m_pal[11] = FromGenesisColour(0x0088);
	m_pal[12] = FromGenesisColour(0x0800);
	m_pal[13] = FromGenesisColour(0x0808);
	m_pal[14] = FromGenesisColour(0x0880);
	m_pal[15] = FromGenesisColour(0x0000);
}

std::size_t Palette::GetPaletteCount(PaletteType type) const
{
	switch (type)
	{
	case PaletteType::ROOM_PALETTE:
		return m_room_palettes.size();
	case PaletteType::SPRITE_HIGH_PALETTE:
		return m_sprite_high_palettes.size();
	case PaletteType::SPRITE_LOW_PALETTE:
		return m_sprite_low_palettes.size();
	default:
		break;
	}
	return 0;
}

Palette::Palette(const Rom& rom, std::size_t index, const PaletteType& type)
    : Palette(rom)
{
    Load(type, index);
}

void Palette::Load(PaletteType type, std::size_t index)
{
    switch (type)
    {
	case PaletteType::ROOM_PALETTE:
		LoadPalette(m_room_palettes[index], 2, m_pal);
        break;
	case PaletteType::SPRITE_HIGH_PALETTE:
		LoadPalette(m_sprite_high_palettes[index], 8, m_pal);
        break;
	case PaletteType::SPRITE_LOW_PALETTE:
		LoadPalette(m_sprite_low_palettes[index], 2, m_pal);
        break;
    default:
		LoadDebugPal();
        break;
    }
}

uint8_t Palette::getR(uint8_t index) const
{
    return m_pal[index].r;
}

uint8_t Palette::getG(uint8_t index) const
{
    return m_pal[index].g;
}

uint8_t Palette::getB(uint8_t index) const
{
    return m_pal[index].b;
}

uint8_t Palette::getA(uint8_t index) const
{
    return m_pal[index].a;
}

uint32_t Palette::getRGBA(uint8_t index) const
{
	uint32_t ret = m_pal[index].r;
    ret |= m_pal[index].g << 8;
    ret |= m_pal[index].b << 16;
    ret |= m_pal[index].a << 24;
    return ret;
}

uint16_t Palette::getGenesisColour(uint8_t index) const
{
	return ToGenesisColour(m_pal[index]);
}

PaletteEntry Palette::FromGenesisColour(uint16_t colour, bool transparent)
{
	PaletteEntry retval;
	retval.b = std::min(0xFF, ((colour & 0x0E00) >> 9) * 36);
	retval.g = std::min(0xFF, ((colour & 0x00E0) >> 5) * 36);
	retval.r = std::min(0xFF, ((colour & 0x000E) >> 1) * 36);
	retval.a = transparent ? 0x00 : 0xFF;
	return retval;
}

uint16_t Palette::ToGenesisColour(const PaletteEntry& entry)
{
	uint16_t retval = 0x0000;
	retval |= std::min(0x07, entry.r / 36) << 1;
	retval |= std::min(0x07, entry.g / 36) << 5;
	retval |= std::min(0x07, entry.b / 36) << 9;
	return retval;
}

bool Palette::m_cache_init = false;
std::vector<std::array<uint16_t, 7>> Palette::m_sprite_high_palettes;
std::vector<std::array<uint16_t, 6>> Palette::m_sprite_low_palettes;
std::vector<std::array<uint16_t, 13>> Palette::m_room_palettes;
