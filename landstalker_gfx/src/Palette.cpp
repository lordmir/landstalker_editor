#include "Palette.h"
#include <algorithm>
#include "Utils.h"

const std::unordered_map<Palette::Type, std::array<bool, 16>> LOCKED_ENTRIES =
{
	{Palette::Type::NONE,        { true,  true,  true,  true,  true,  true,  true,  true,
								   true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::FULL,        {false, false, false, false, false, false, false, false,
								  false, false, false, false, false, false, false, false}},
	{Palette::Type::LOW8,        {false, false, false, false, false, false, false, false,
								   true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::ROOM,        { true,  true, false, false, false, false, false, false,
								  false, false, false, false, false, false, false,  true}},
	{Palette::Type::HUD,         { true,  true,  true,  true,  true,  true,  true,  true,
								   true,  true, false, false, false, false, false,  true}},
	{Palette::Type::SPRITE_LOW,  { true,  true, false, false, false, false, false, false,
								   true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::SPRITE_HIGH, { true,  true,  true,  true,  true,  true,  true,  true,
								  false, false, false, false, false, false, false,  true}},
	{Palette::Type::SPRITE_FULL, { true,  true, false, false, false, false, false, false,
								  false, false, false, false, false, false, false,  true}},
	{Palette::Type::PROJECTILE,  { true,  true,  true,  true,  true,  true,  true,  true,
								  false, false,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::SWORD,       { true,  true,  true,  true,  true,  true,  true,  true,
								   true,  true,  true,  true,  true, false, false,  true}},
	{Palette::Type::ARMOUR,      { true,  true,  true,  true,  true,  true,  true,  true,
								   true,  true,  true, false, false,  true,  true,  true}},
	{Palette::Type::LAVA,        { true,  true,  true,  true,  true,  true,  true,  true,
								  false, false,  true, false, false,  true,  true,  true}},
	{Palette::Type::WARP,        { true,  true,  true,  true,  true,  true,  true,  true,
								   true,  true,  true,  true,  true, false, false,  true}},
	{Palette::Type::SEGA_LOGO,   {false, false, false, false, false, false, false,  true,
								   true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::CLIMAX_LOGO, {false, false, false, false,  true,  true,  true,  true,
								   true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::TITLE_YELLOW,{ true, false, false, false, false, false,  true,  true,
								   true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::TITLE_SINGLE_COLOUR,{ true, false,  true,  true,  true,  true,  true,  true,
										  true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::TITLE_BLUE_FADE, {false, false, false, false, false, false, false, false,
								      false, false, false, false, false, false, false, false}},
	{Palette::Type::END_CREDITS, {false, false, false, false,  true,  true,  true,  true,
								   true,  true,  true,  true,  true,  true,  true,  true}}
};

template<std::size_t N>
static void PopulatePalettes(const std::vector<uint16_t>& input, std::vector<std::array<uint16_t, N>>& output)
{
	// Input must contain a whole number of palettes
	assert(input.size() % N == 0);

	size_t i = 0;
	std::array<uint16_t, N> pal;
	for (const auto& in : input)
	{
		pal[i++] = in;
		if (i == N)
		{
			output.push_back(pal);
			i = 0;
		}
	}
}

template <std::size_t N>
static void LoadPalette(const std::array<uint16_t, N>& palette, size_t begin, std::array<PaletteEntry, 16> & outpal, bool var_width = false)
{
	size_t i = 0;
	assert(!var_width || (palette[0] == N - 2));
	for (auto colour : palette)
	{
		if (var_width)
		{
			// Skip over first entry as it is the palette size
			var_width = false;
			continue;
		}
		outpal[begin + i++] = Palette::FromGenesisColour(colour, (i + begin) == 0);
		if (i >= outpal.size())
		{
			break;
		}
	}
}

Palette::Palette()
	: m_type(Type::NONE)
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
		PopulatePalettes(rom.read_array<uint16_t>("lava_palettes"), m_lava_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("labrynth_lantern_palette"), m_room_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("equipped_sword_palette"), m_equipped_sword_palette);
		PopulatePalettes(rom.read_array<uint16_t>("equipped_armour_palette"), m_equipped_armour_palette);
		PopulatePalettes(rom.read_array<uint16_t>("player_palette"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("hud_palette"), m_hud_palette);
		PopulatePalettes(rom.read_array<uint16_t>("inventory_palette_1_8col"), m_inventory_palette_1_8col);
		PopulatePalettes(rom.read_array<uint16_t>("inventory_palette_2"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("inventory_palette_3"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("kazalt_warp_palette"), m_kazalt_warp_palette);
		PopulatePalettes(rom.read_array<uint16_t>("new_game_palette"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("new_game_player_palette"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("sega_logo_palette"), m_sega_logo_palette);
		PopulatePalettes(rom.read_array<uint16_t>("lithograph_palette"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("title_seq_blue_palette"), m_title_seq_blue_palette);
		PopulatePalettes(rom.read_array<uint16_t>("title_seq_yellow_pallette"), m_title_seq_yellow_pallette);
		PopulatePalettes(rom.read_array<uint16_t>("title_seq_single_colours"), m_title_seq_single_colours);
		PopulatePalettes(rom.read_array<uint16_t>("title_seq_palette"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("climax_logo_palette"), m_climax_logo_palette);
		PopulatePalettes(rom.read_array<uint16_t>("island_map_palette"), m_full_palettes);
		PopulatePalettes(rom.read_array<uint16_t>("end_credits_palette"), m_end_credits_palette);
		PopulatePalettes(rom.read_array<uint16_t>("projectiles_palette_1"), m_projectiles_palette);
		PopulatePalettes(rom.read_array<uint16_t>("projectiles_palette_2"), m_projectiles_palette);
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

void Palette::AddHighPalette(const Palette& high_palette)
{
	if (m_type == Type::SPRITE_LOW)
	{
		m_type = Type::SPRITE_FULL;
	}
	else
	{
		m_type = Type::SPRITE_HIGH;
	}
	const std::size_t begin = std::find(LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin(), LOCKED_ENTRIES.at(Type::SPRITE_HIGH).end(), false) - LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin();
	const std::size_t end = std::find(LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin() + begin, LOCKED_ENTRIES.at(Type::SPRITE_HIGH).end(), true) - LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin();
	for (auto i = begin; i != end; ++i)
	{
		m_pal[i] = high_palette.m_pal[i];
	}
}

void Palette::RemoveHighPalette()
{
	if (m_type == Type::SPRITE_FULL)
	{
		m_type = Type::SPRITE_LOW;
	}
	else
	{
		m_type = Type::NONE;
	}
	const std::size_t begin = std::find(LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin(), LOCKED_ENTRIES.at(Type::SPRITE_HIGH).end(), false) - LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin();
	const std::size_t end = std::find(LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin() + begin, LOCKED_ENTRIES.at(Type::SPRITE_HIGH).end(), true) - LOCKED_ENTRIES.at(Type::SPRITE_HIGH).begin();
	for (auto i = begin; i != end; ++i)
	{
		m_pal[i] = FromGenesisColour(0x0000);
	}
}

void Palette::AddLowPalette(const Palette& low_palette)
{
	if (m_type == Type::SPRITE_HIGH)
	{
		m_type = Type::SPRITE_FULL;
	}
	else
	{
		m_type = Type::SPRITE_LOW;
	}
	const std::size_t begin = std::find(LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin(), LOCKED_ENTRIES.at(Type::SPRITE_LOW).end(), false) - LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin();
	const std::size_t end = std::find(LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin() + begin, LOCKED_ENTRIES.at(Type::SPRITE_LOW).end(), true) - LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin();
	for (auto i = begin; i != end; ++i)
	{
		m_pal[i] = low_palette.m_pal[i];
	}
}

void Palette::RemoveLowPalette()
{
	if (m_type == Type::SPRITE_HIGH)
	{
		m_type = Type::SPRITE_FULL;
	}
	else
	{
		m_type = Type::NONE;
	}
	m_type = Type::SPRITE_HIGH;
	const std::size_t begin = std::find(LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin(), LOCKED_ENTRIES.at(Type::SPRITE_LOW).end(), false) - LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin();
	const std::size_t end = std::find(LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin() + begin, LOCKED_ENTRIES.at(Type::SPRITE_LOW).end(), true) - LOCKED_ENTRIES.at(Type::SPRITE_LOW).begin();
	for (auto i = begin; i != end; ++i)
	{
		m_pal[i] = FromGenesisColour(0x0000);
	}
}

std::size_t Palette::GetPaletteCount(Type type) const
{
	switch (type)
	{
	case Type::ROOM:
		return m_room_palettes.size();
	case Type::SPRITE_HIGH:
		return m_sprite_high_palettes.size();
	case Type::SPRITE_LOW:
		return m_sprite_low_palettes.size();
	case Type::FULL:
		return m_full_palettes.size();
	case Type::LOW8:
		return m_inventory_palette_1_8col.size();
	case Type::LAVA:
		return m_lava_palettes.size();
	case Type::PROJECTILE:
		return m_projectiles_palette.size();
	case Type::HUD:
		return m_hud_palette.size();
	case Type::SWORD:
		return m_equipped_sword_palette.size();
	case Type::ARMOUR:
		return m_equipped_armour_palette.size();
	case Type::WARP:
		return m_kazalt_warp_palette.size();
	case Type::SEGA_LOGO:
		return m_sega_logo_palette.size();
	case Type::CLIMAX_LOGO:
		return m_climax_logo_palette.size();
	case Type::TITLE_YELLOW:
		return m_title_seq_yellow_pallette.size();
	case Type::TITLE_BLUE_FADE:
		return m_title_seq_blue_palette.size();
	case Type::TITLE_SINGLE_COLOUR:
		return m_title_seq_single_colours.size();
	case Type::END_CREDITS:
		return m_end_credits_palette.size();
	default:
		break;
	}
	return 0;
}

Palette::Palette(const Rom& rom, std::size_t index, const Type& type)
    : Palette(rom)
{
	m_type = type;
    Load(index);
}

void Palette::Load(std::size_t index)
{
	const std::size_t begin = std::find(LOCKED_ENTRIES.at(m_type).begin(), LOCKED_ENTRIES.at(m_type).end(), false) - LOCKED_ENTRIES.at(m_type).begin();
    switch (m_type)
    {
	case Type::ROOM:
		LoadPalette(m_room_palettes[index], begin, m_pal);
        break;
	case Type::SPRITE_HIGH:
		LoadPalette(m_sprite_high_palettes[index], begin, m_pal);
        break;
	case Type::SPRITE_LOW:
		LoadPalette(m_sprite_low_palettes[index], begin, m_pal);
        break;
	case Type::FULL:
		LoadPalette(m_full_palettes[index], begin, m_pal);
		break;
	case Type::LOW8:
		LoadPalette(m_inventory_palette_1_8col[index], begin, m_pal);
		break;
	case Type::LAVA:
		LoadPalette(m_lava_palettes[index], begin, m_pal);
		break;
	case Type::PROJECTILE:
		LoadPalette(m_projectiles_palette[index], begin, m_pal);
		break;
	case Type::HUD:
		LoadPalette(m_hud_palette[index], begin, m_pal);
		break;
	case Type::SWORD:
		LoadPalette(m_equipped_sword_palette[index], begin, m_pal);
		break;
	case Type::ARMOUR:
		LoadPalette(m_equipped_armour_palette[index], begin, m_pal);
		break;
	case Type::WARP:
		LoadPalette(m_kazalt_warp_palette[index], begin, m_pal);
		break;
	case Type::SEGA_LOGO:
		LoadPalette(m_sega_logo_palette[index], begin, m_pal);
		break;
	case Type::CLIMAX_LOGO:
		LoadPalette(m_climax_logo_palette[index], begin, m_pal);
		break;
	case Type::TITLE_YELLOW:
		LoadPalette(m_title_seq_yellow_pallette[index], begin, m_pal);
		break;
	case Type::TITLE_BLUE_FADE:
		LoadPalette(m_title_seq_blue_palette[index], begin, m_pal, true);
		break;
	case Type::TITLE_SINGLE_COLOUR:
		LoadPalette(m_title_seq_single_colours[index], begin, m_pal);
		break;
	case Type::END_CREDITS:
		LoadPalette(m_end_credits_palette[index], begin, m_pal);
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

PaletteEntry Palette::GetColour(uint8_t index) const
{
	return m_pal[index];
}

void Palette::setGenesisColour(uint8_t index, uint16_t colour)
{
	m_pal[index] = FromGenesisColour(colour);
}

bool Palette::ColourInRange(uint8_t colour) const
{
	return (colour < m_pal.size());
}

bool Palette::ColourEditable(uint8_t colour) const
{
	if (colour < m_pal.size())
	{
		return !LOCKED_ENTRIES.at(m_type).at(colour);
	}
	return false;
}

const std::array<bool, 16>& Palette::GetLockedColours() const
{
	return LOCKED_ENTRIES.at(m_type);
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

void Palette::Reset()
{
	m_sprite_high_palettes.clear();
	m_sprite_low_palettes.clear();
	m_room_palettes.clear();
	m_cache_init = false;
}

bool Palette::m_cache_init = false;
std::vector<std::array<uint16_t, 7>>  Palette::m_sprite_high_palettes;
std::vector<std::array<uint16_t, 6>>  Palette::m_sprite_low_palettes;
std::vector<std::array<uint16_t, 13>> Palette::m_room_palettes;
std::vector<std::array<uint16_t, 2>>  Palette::m_lava_palettes;
std::vector<std::array<uint16_t, 2>>  Palette::m_equipped_sword_palette;
std::vector<std::array<uint16_t, 2>>  Palette::m_equipped_armour_palette;
std::vector<std::array<uint16_t, 16>> Palette::m_full_palettes;
std::vector<std::array<uint16_t, 5>>  Palette::m_hud_palette;
std::vector<std::array<uint16_t, 8>>  Palette::m_inventory_palette_1_8col;
std::vector<std::array<uint16_t, 2>>  Palette::m_kazalt_warp_palette;
std::vector<std::array<uint16_t, 7>>  Palette::m_sega_logo_palette;
std::vector<std::array<uint16_t, 28>> Palette::m_title_seq_blue_palette;
std::vector<std::array<uint16_t, 5>>  Palette::m_title_seq_yellow_pallette;
std::vector<std::array<uint16_t, 1>>  Palette::m_title_seq_single_colours;
std::vector<std::array<uint16_t, 4>>  Palette::m_climax_logo_palette;
std::vector<std::array<uint16_t, 4>>  Palette::m_end_credits_palette;
std::vector<std::array<uint16_t, 2>>  Palette::m_projectiles_palette;
