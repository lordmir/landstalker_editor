#ifndef _ENTRY_PREFERENCES_H_
#define _ENTRY_PREFERENCES_H_

#include <string>
#include <vector>

class PalettePreferences
{
public:

	const std::string& GetDefaultPalette() const;
	const std::vector<std::string>& GetRecommendedPalettes() const;
	const std::vector<std::string>& GetAllPalettes() const;

	void SetDefaultPalette(const std::string& p);
	void SetRecommendedPalettes(const std::vector<std::string>& pl, bool set_default = false);
	void SetAllPalettes(const std::vector<std::string>& pl, bool set_recommended = false, bool set_default = false);
private:
	std::string m_default_palette;
	std::vector<std::string> m_recommended_palettes;
	std::vector<std::string> m_all_palettes;
};

#endif // _ENTRY_PREFERENCES_H_
