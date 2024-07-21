#ifndef PALETTE_H
#define PALETTE_H

#include <array>
#include <vector>
#include <cstdint>
#include <memory>

#include <landstalker/main/include/Rom.h>

class Palette
{
public:
	class Colour
	{
	public:
		Colour(uint8_t r, uint8_t g, uint8_t b, bool transparent = false)
			: m_r(std::min<uint8_t>(0x07, r / 36)),
			  m_g(std::min<uint8_t>(0x07, g / 36)),
			  m_b(std::min<uint8_t>(0x07, b / 36)),
			  m_transparent(transparent)
		{
		}
		Colour(uint16_t c, bool transparent = false)
			: m_r((c & 0x000E) >> 1),
			  m_g((c & 0x00E0) >> 5),
			  m_b((c & 0x0E00) >> 9),
			  m_transparent(transparent)
		{
		}
		Colour()
			: Colour(0x0000, true)
		{}

		static Colour CreateFromBGRA(uint32_t bgra)
		{
			Colour c;
			c.FromBGRA(bgra);
			return c;
		}

		bool operator==(const Colour& rhs) const
		{
			return ((this->m_transparent == rhs.m_transparent) &&
				(this->m_r == rhs.m_r) &&
				(this->m_g == rhs.m_g) &&
				(this->m_b == rhs.m_b));
		}

		bool operator!=(const Colour& rhs) const
		{
			return !(*this == rhs);
		}

		inline void FromRGB(uint32_t c, bool transparent = false)
		{
			uint8_t r = (c >> 16) & 0xFF;
			uint8_t g = (c >> 8) & 0xFF;
			uint8_t b = c & 0xFF;
			m_transparent = transparent;
			m_r = std::min<uint8_t>(0x07, r / 36);
			m_g = std::min<uint8_t>(0x07, g / 36);
			m_b = std::min<uint8_t>(0x07, b / 36);
		}

		inline void FromBGR(uint32_t c, bool transparent = false)
		{
			uint8_t b = (c >> 16) & 0xFF;
			uint8_t g = (c >> 8) & 0xFF;
			uint8_t r = c & 0xFF;
			m_transparent = transparent;
			m_r = std::min<uint8_t>(0x07, r / 36);
			m_g = std::min<uint8_t>(0x07, g / 36);
			m_b = std::min<uint8_t>(0x07, b / 36);
		}

		inline void FromRGBA(uint32_t c)
		{
			uint8_t a = (c >> 24) & 0xFF;
			FromRGB(c, (a == 0x00));
		}

		inline void FromBGRA(uint32_t c)
		{
			uint8_t a = (c >> 24) & 0xFF;
			FromBGR(c, (a == 0x00));
		}

		inline void FromGenesis(uint16_t c, bool transparent = false)
		{
			m_b = (c & 0x0E00) >> 9;
			m_g = (c & 0x00E0) >> 5;
			m_r = (c & 0x000E) >> 1;
			m_transparent = transparent;
		}

		inline uint8_t GetR() const
		{
			return std::min<uint8_t>(0xFF, m_r * 36);
		}
		inline uint8_t GetG() const
		{
			return std::min<uint8_t>(0xFF, m_g * 36);
		}
		inline uint8_t GetB() const
		{
			return std::min<uint8_t>(0xFF, m_b * 36);
		}
		inline uint8_t GetA() const
		{
			return m_transparent ? 0x00 : 0xFF;
		}
		inline void SetTransparent(bool transparent)
		{
			m_transparent = transparent;
		}
		uint16_t GetGenesis() const
		{
			uint16_t retval = 0x0000;
			retval |= (m_b & 0x07) << 9;
			retval |= (m_g & 0x07) << 5;
			retval |= (m_r & 0x07) << 1;
			return retval;
		}
		uint32_t GetRGB(bool include_alpha) const
		{
			uint32_t result = 0;
			if (include_alpha == true)
			{
				result = GetA() << 24;
			}
			result |= GetR() << 16;
			result |= GetG() << 8;
			result |= GetB();
			return result;
		}
		uint32_t GetBGR(bool include_alpha) const
		{
			uint32_t result = 0;
			if (include_alpha == true)
			{
				result = GetA() << 24;
			}
			result |= GetB() << 16;
			result |= GetG() << 8;
			result |= GetR();
			return result;
		}

		template<class Iter>
		inline Iter& CopyRGBA(Iter& begin)
		{
			*begin++ = GetA();
			*begin++ = GetR();
			*begin++ = GetG();
			*begin++ = GetB();
			return begin;
		}
		template<class Iter>
		inline Iter& CopyRGB(Iter& begin)
		{
			*begin++ = GetR();
			*begin++ = GetG();
			*begin++ = GetB();
			return begin;
		}
		template<class Iter>
		inline Iter& CopyA(Iter& begin)
		{
			*begin++ = GetA();
			return begin;
		}
	private:
		uint8_t m_r;
		uint8_t m_g;
		uint8_t m_b;
		bool m_transparent;
	};

	enum class Type
	{
		NONE,
		FULL,
		LOW8,
		HUD,
		ROOM,
		SPRITE_HIGH,
		SPRITE_LOW,
		SPRITE_FULL,
		PROJECTILE,
		PROJECTILE2,
		SWORD,
		ARMOUR,
		LAVA,
		WARP,
		SEGA_LOGO,
		CLIMAX_LOGO,
		TITLE_BLUE_FADE,
		TITLE_YELLOW,
		TITLE_SINGLE_COLOUR,
		END_CREDITS
	};

	Palette(const std::string& name = "default", const Type& type = Type::FULL);
	Palette(const std::string& name, const std::vector<Colour>& colours, const Type& type);
	Palette(const std::string& name, const std::vector<uint8_t>& bytes, const Palette::Type& type);
	Palette(const Palette& pal);
	Palette(const std::vector<std::shared_ptr<Palette>>& pals);

	Palette& operator=(const Palette&);
	bool operator==(const Palette& rhs) const;
	bool operator!=(const Palette& rhs) const;

	std::vector<uint8_t> GetBytes() const;

	void Clear();
	void LoadDebugPal();

	uint8_t  getR(uint8_t index) const;
	uint8_t  getG(uint8_t index) const;
	uint8_t  getB(uint8_t index) const;
	uint8_t  getA(uint8_t index) const;
	uint32_t getRGB(uint8_t index) const;
	uint32_t getRGBA(uint8_t index) const;
	uint32_t getBGRA(uint8_t index) const;
	std::string getOwner(uint8_t index) const;
	uint16_t getGenesisColour(uint8_t index) const;
	Colour GetColour(uint8_t index) const;
	uint8_t GetNthUnlockedIndex(uint8_t n) const;
	Colour GetNthUnlockedColour(uint8_t n) const;
	void SetNthUnlockedGenesisColour(uint8_t n, uint16_t colour);
	void setGenesisColour(uint8_t index, uint16_t colour);
	bool ColourInRange(uint8_t colour) const;
	bool ColourEditable(uint8_t colour) const;
	const std::vector<bool>& GetLockedColours() const;
	int GetSize() const;
	int GetSizeBytes() const;
	bool IsVarWidth() const;

	static const std::vector<bool>& GetLockedColours(const Type& type);
	static int GetSize(const Type& type);
	static int GetSizeBytes(const Type& type);
	static bool IsVarWidth(const Type& type);
private:

	Type m_type;
	std::string m_name;
	std::vector<std::shared_ptr<Colour>> m_pal;
	std::vector<std::string> m_owner;
	std::vector<bool> m_locked;
};

#endif // PALETTE_H
