#include "Palette.h"
#include <algorithm>
#include <cassert>
#include <numeric>
#include "Utils.h"

const std::unordered_map<Palette::Type, std::vector<bool>> LOCKED_ENTRIES =
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
	{Palette::Type::PROJECTILE2, { true,  true,  true,  true, false, false, false, false,
								   true,  true,  true,  true,  true,  true,  true,  true}},
	{Palette::Type::SWORD,       { true,  true,  true,  true,  true,  true,  true,  true,
								   true,  true,  true,  true,  true, false, false,  true}},
	{Palette::Type::ARMOUR,      { true,  true,  true,  true,  true,  true,  true,  true,
								   true,  true,  true, false, false,  true,  true,  true}},
	{Palette::Type::LAVA,        { true,  true,  true,  true,  true,  true,  true,  true,
								  false, false,  true,  true,  true,  true,  true,  true}},
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

std::unordered_map<Palette::Type, int> PALETTE_SIZES =
{
	{Palette::Type::NONE,                 0},
	{Palette::Type::FULL,                16},
	{Palette::Type::LOW8,                 8},
	{Palette::Type::ROOM,                13},
	{Palette::Type::HUD,                  5},
	{Palette::Type::SPRITE_LOW,           6},
	{Palette::Type::SPRITE_HIGH,          7},
	{Palette::Type::SPRITE_FULL,         13},
	{Palette::Type::PROJECTILE,           2},
	{Palette::Type::PROJECTILE2,          4},
	{Palette::Type::SWORD,                2},
	{Palette::Type::ARMOUR,               2},
	{Palette::Type::LAVA,                 2},
	{Palette::Type::WARP,                 2},
	{Palette::Type::SEGA_LOGO,            7},
	{Palette::Type::CLIMAX_LOGO,          4},
	{Palette::Type::TITLE_YELLOW,         5},
	{Palette::Type::TITLE_SINGLE_COLOUR,  1},
	{Palette::Type::TITLE_BLUE_FADE,     -1},
	{Palette::Type::END_CREDITS,          4}
};

Palette::Palette(const std::string& name, const Type& type)
	: m_name(name),
	  m_type(type)
{
	m_pal.resize(16);
	std::for_each(m_pal.begin(), m_pal.end(), [](auto& e) { e = std::make_shared<Colour>(); });
	m_owner.resize(m_pal.size(), "");
	m_locked = LOCKED_ENTRIES.at(type);
	for (int i = 0; i < m_pal.size(); ++i)
	{
		if (!m_locked[i])
		{
			m_owner[i] = m_name;
		}
	}
	Clear();
	LoadDebugPal();
}

void Palette::Clear()
{
	if (m_pal.size() > 0)
	{
		m_pal[0]->FromGenesis(0x0000, true);
	}
	if (m_pal.size() > 1)
	{
		m_pal[1]->FromGenesis(0x0CCC);
	}
	for (size_t i = 2; i < m_pal.size(); ++i)
	{
		m_pal[i]->FromGenesis(0x0000);
	}
}

void Palette::LoadDebugPal()
{
	m_pal[0]->FromGenesis(0x0C0C);
	m_pal[1]->FromGenesis(0x0CCC);
	m_pal[2]->FromGenesis(0x000E);
	m_pal[3]->FromGenesis(0x00E0);
	m_pal[4]->FromGenesis(0x00EE);
	m_pal[5]->FromGenesis(0x0E00);
	m_pal[6]->FromGenesis(0x0EE0);
	m_pal[7]->FromGenesis(0x0EEE);
	m_pal[8]->FromGenesis(0x0888);
	m_pal[9]->FromGenesis(0x0008);
	m_pal[10]->FromGenesis(0x0080);
	m_pal[11]->FromGenesis(0x0088);
	m_pal[12]->FromGenesis(0x0800);
	m_pal[13]->FromGenesis(0x0808);
	m_pal[14]->FromGenesis(0x0880);
	m_pal[15]->FromGenesis(0x0000);
}

bool Palette::operator==(const Palette& rhs) const
{
	bool retval = (this->m_type == rhs.m_type);
	retval = retval && (this->m_pal.size() == rhs.m_pal.size());
	if (retval)
	{
		for (int i = 0; i < m_pal.size(); ++i)
		{
			retval = retval && (*this->m_pal[i] == *rhs.m_pal[i]);
		}
	}
	return retval;
}

bool Palette::operator!=(const Palette& rhs) const
{
	return !(*this == rhs);
}

Palette::Palette(const std::string& name, const std::vector<Colour>& colours, const Type& type)
	: m_name(name),
	  m_type(type)
{
	int size = GetSize();
	assert(colours.size() == size);
	if (IsVarWidth())
	{
		auto it = colours.cbegin();
		m_pal.resize(colours.size());
		std::for_each(m_pal.begin(), m_pal.end(), [&](auto& e) { e = std::make_shared<Colour>(*it++); });
		m_owner.resize(m_pal.size(), m_name);
		m_locked.resize(m_pal.size(), false);
	}
	else
	{
		m_pal.resize(16);
		std::for_each(m_pal.begin(), m_pal.end(), [](auto& e) { e = std::make_shared<Colour>(); });
		Clear();
		auto it = colours.begin();
		for (int i = 0; i < m_pal.size(); ++i)
		{
			if (!LOCKED_ENTRIES.at(m_type).at(i))
			{
				*m_pal[i] = *it++;
			}
		}
		m_pal[0]->SetTransparent(true);
		m_owner.resize(m_pal.size(), "");
		m_locked = LOCKED_ENTRIES.at(type);
		for (int i = 0; i < m_pal.size(); ++i)
		{
			if (!m_locked[i])
			{
				m_owner[i] = m_name;
			}
		}
	}
}

Palette::Palette(const std::string& name, const std::vector<uint8_t>& bytes, const Type& type)
	: m_name(name),
	  m_type(type)
{	// Input must contain the right number of colours
	int size = PALETTE_SIZES[m_type];
	auto it = bytes.begin();
	if (size == -1) // Variable width palette
	{
		size = (*it << 8) | *(it + 1);
		it += 2;
		assert(bytes.size() == ((size + 2) * 2));
	}
	else
	{
		assert(bytes.size() == (size * 2));
	}

	size_t i = 0;
	std::vector<Colour> colours;
	colours.reserve(size);
	for (; it != bytes.end(); it += 2)
	{
		colours.emplace_back((*it << 8) | *(it + 1));
		if (colours.size() == size)
		{
			break;
		}
	}
	if (IsVarWidth())
	{
		auto it = colours.cbegin();
		m_pal.resize(colours.size());
		std::for_each(m_pal.begin(), m_pal.end(), [&](auto& e) { e = std::make_shared<Colour>(*it++); });
		m_owner.resize(m_pal.size(), m_name);
		m_locked.resize(m_pal.size(), false);
	}
	else
	{
		m_pal.resize(16);
		std::for_each(m_pal.begin(), m_pal.end(), [](auto& e) { e = std::make_shared<Colour>(); });
		Clear();
		auto it = colours.begin();
		for (int i = 0; i < m_pal.size(); ++i)
		{
			if (!LOCKED_ENTRIES.at(m_type).at(i))
			{
				*m_pal[i] = *it++;
			}
		}
		m_pal[0]->SetTransparent(true);
		m_owner.resize(m_pal.size(), "");
		m_locked = LOCKED_ENTRIES.at(type);
		for (int i = 0; i < m_pal.size(); ++i)
		{
			if (!m_locked[i])
			{
				m_owner[i] = m_name;
			}
		}
	}
}

Palette::Palette(const Palette& pal)
	: m_name(pal.m_name),
	  m_type(pal.m_type),
	  m_locked(pal.m_locked),
	  m_owner(pal.m_owner)
{
	m_pal.resize(pal.m_pal.size());
	auto it = pal.m_pal.cbegin();
	std::for_each(m_pal.begin(), m_pal.end(), [&](auto& e) { e = std::make_shared<Colour>(*(*it++)); });
}

Palette::Palette(const std::vector<std::shared_ptr<Palette>>& pals)
	: m_type(Type::FULL)
{
	if (pals.size() > 0)
	{
		m_pal.resize(pals.front()->m_pal.size());
		assert(std::all_of(pals.cbegin(), pals.cend(), [&](const auto& p) { return p->m_pal.size() == m_pal.size(); }));
		std::for_each(m_pal.begin(), m_pal.end(), [](auto& e) { e = std::make_shared<Colour>(); });
		m_locked.resize(m_pal.size(), true);
		m_owner.resize(m_pal.size(), "");
		Clear();
		for (const auto& pal : pals)
		{
			for (int i = 0; i < m_pal.size(); ++i)
			{
				if (pal->m_locked[i] == false)
				{
					m_locked[i] = false;
					m_owner[i] = pal->m_name;
					m_pal[i] = pal->m_pal[i];
				}
			}
		}
		m_name = std::accumulate(std::next(pals.cbegin()), pals.cend(), pals.front()->m_name, [](std::string list, const auto& p) {
			return std::move(list) + ',' + p->m_name;
			});
	}
	else
	{
		m_pal.resize(16);
		std::for_each(m_pal.begin(), m_pal.end(), [](auto& e) { e = std::make_shared<Colour>(); });
		m_owner.resize(m_pal.size(), "Default");
		m_locked.resize(m_pal.size(), true);
		Clear();
	}
}

std::vector<uint8_t> Palette::GetBytes() const
{
	std::vector<uint8_t> retval;
	int size = GetSize();
	if (IsVarWidth())
	{
		retval.reserve((size + 1) * 2);
		retval.push_back((size & 0xFF00) >> 8);
		retval.push_back(size & 0xFF);
	}
	else
	{
		retval.reserve(size * 2);
	}
	for (std::size_t i = 0; i < m_pal.size(); ++i)
	{
		if (!LOCKED_ENTRIES.at(m_type).at(i))
		{
			uint16_t c = m_pal[i]->GetGenesis();
			retval.push_back((c & 0xFF00) >> 8);
			retval.push_back(c & 0xFF);
		}
	}
	return retval;
}

uint8_t Palette::getR(uint8_t index) const
{
    return m_pal[index]->GetR();
}

uint8_t Palette::getG(uint8_t index) const
{
    return m_pal[index]->GetG();
}

uint8_t Palette::getB(uint8_t index) const
{
    return m_pal[index]->GetB();
}

uint8_t Palette::getA(uint8_t index) const
{
    return m_pal[index]->GetA();
}

uint32_t Palette::getRGBA(uint8_t index) const
{
	return m_pal[index]->GetRGB(true);
}

uint32_t Palette::getBGRA(uint8_t index) const
{
	return m_pal[index]->GetBGR(true);
}

uint16_t Palette::getGenesisColour(uint8_t index) const
{
	return m_pal[index]->GetGenesis();
}

Palette::Colour Palette::GetColour(uint8_t index) const
{
	return *m_pal[index];
}

void Palette::setGenesisColour(uint8_t index, uint16_t colour)
{
	*m_pal[index] = Palette::Colour(colour);
}

bool Palette::ColourInRange(uint8_t colour) const
{
	return (colour < m_pal.size());
}

bool Palette::ColourEditable(uint8_t colour) const
{
	if (colour < m_pal.size())
	{
		return !m_locked.at(colour);
	}
	return false;
}

const std::vector<bool>& Palette::GetLockedColours() const
{
	return m_locked;
}

int Palette::GetSize() const
{
	int size = PALETTE_SIZES[m_type];
	if (size == -1)
	{
		// Var-width palette
		size = m_pal.size();
	}
	return size;
}

int Palette::GetSizeBytes() const
{
	int size = PALETTE_SIZES[m_type];
	if (size == -1)
	{
		// Var-width palette
		size = m_pal.size() * 2 + 4;
	}
	return size * 2;
}

bool Palette::IsVarWidth() const
{
	return IsVarWidth(m_type);
}

const std::vector<bool>& Palette::GetLockedColours(const Type& type)
{
	return LOCKED_ENTRIES.at(type);
}

int Palette::GetSize(const Type& type)
{
	int size = PALETTE_SIZES[type];
	if (size == -1)
	{
		size = 0;
	}
	return size;
}

int Palette::GetSizeBytes(const Type& type)
{
	return GetSize(type) * 2;
}

bool Palette::IsVarWidth(const Type& type)
{
	return PALETTE_SIZES[type] == -1;
}
