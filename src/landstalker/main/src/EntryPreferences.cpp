#include <landstalker/main/include/EntryPreferences.h>

const std::string& PalettePreferences::GetDefaultPalette() const
{
	return m_default_palette;
}

const std::vector<std::string>& PalettePreferences::GetRecommendedPalettes() const
{
	return m_recommended_palettes;
}

const std::vector<std::string>& PalettePreferences::GetAllPalettes() const
{
	return m_all_palettes;
}

void PalettePreferences::SetDefaultPalette(const std::string& p)
{
	m_default_palette = p;
}

void PalettePreferences::SetRecommendedPalettes(const std::vector<std::string>& pl, bool set_default)
{
	m_recommended_palettes = pl;
	if (set_default)
	{
		m_default_palette = pl.front();
	}
}

void PalettePreferences::SetAllPalettes(const std::vector<std::string>& pl, bool set_recommended, bool set_default)
{
	m_all_palettes = pl;
	if (set_recommended)
	{
		SetRecommendedPalettes(pl, set_default);
	}
}
