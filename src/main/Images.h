#ifndef _IMAGES_H_
#define _IMAGES_H_

#include <landstalker/main/RomOffsets.h>
#include <landstalker/main/Blockmap2D.h>
#include <landstalker/main/PaletteO.h>

#include <string>
#include <map>

namespace Images
{
	struct Image
	{
		std::shared_ptr<Landstalker::Tileset> tileset;
		std::shared_ptr<Landstalker::Tilemap2D> map;
		std::shared_ptr<Landstalker::PaletteO> palette;
	};

	struct ImageDescriptor
	{
		// Tileset Properties
		std::string tileset_name;
		bool tileset_compression;
		// Map Properties
		std::string tilemap_name;
		Landstalker::Tilemap2D::Compression map_compression;
		std::size_t map_width;
		std::size_t map_height;
		std::size_t map_base;
		// Palette Properties
		Landstalker::PaletteO::Type palette_type;
		std::size_t palette_idx;
	};

	static const std::map<std::string, ImageDescriptor> IMAGES
	{
		{"Lithograph",            {"lithograph_tiles",  true, "lithograph_map",  Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::FULL, 5}},
		{"Island Map Background", {"islemap_bg_tiles",  true, "islemap_bg_map",  Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::FULL, 9}},
		{"Island Map Foreground", {"islemap_fg_tiles",  true, "islemap_fg_map",  Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::FULL, 8}},
		{"Title Screen 1",        {"title1_tiles",      true, "title1_map",      Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::TITLE_BLUE_FADE, 0}},
		{"Title Screen 2",        {"title2_tiles",      true, "title2_map",      Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::TITLE_BLUE_FADE, 0}},
		{"Title Screen 3",        {"title3_tiles",      true, "title3_map",      Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::FULL, 6}},
		{"Sega Logo",             {"sega_logo_tiles",   true, "",                Tilemap2D::Compression::NONE, 12,  4, 0,     PaletteO::Type::SEGA_LOGO, 0}},
		{"Climax Logo",           {"climax_logo_tiles", true, "climax_logo_map", Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::CLIMAX_LOGO, 0}},
		{"End Credits Logos",     {"ending_logo_tiles", true, "ending_logo_map", Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::END_CREDITS, 0}},
		{"HUD",                   {"hud_tiles",         true, "hud_map",         Tilemap2D::Compression::NONE, 40,  3, 0x6B4, PaletteO::Type::HUD, 0}},
		{"Inventory",             {"hud_tiles",         true, "inventory_map",   Tilemap2D::Compression::LZ77,  0,  0, 0x6B4, PaletteO::Type::HUD, 0}},
		{"Textbox (2 Line)",      {"hud_tiles",         true, "textbox2l_map",   Tilemap2D::Compression::NONE, 40,  6, 0x6B4, PaletteO::Type::HUD, 0}},
		{"Textbox (3 Line)",      {"hud_tiles",         true, "textbox3l_map",   Tilemap2D::Compression::NONE, 40,  8, 0x6B4, PaletteO::Type::HUD, 0}},
		{"Load Game Screen",      {"loadgame_tiles",    true, "loadgame_map",    Tilemap2D::Compression::RLE,   0,  0, 0x100, PaletteO::Type::FULL, 3}}
	};

	static std::map<std::string, Image> GetImages(const Rom& m_rom)
	{
		std::map<std::string, Image> ret;
		for (const auto& elem : IMAGES)
		{
			const auto& name = elem.first;
			const auto& d = elem.second;
			auto ts = std::make_shared<Landstalker::Tileset>(Landstalker::Tileset(m_rom.read_array<uint8_t>(d.tileset_name), d.tileset_compression));
			std::shared_ptr<Tilemap2D> map;
			if (d.tilemap_name != "")
			{
				map = std::make_shared<Landstalker::Tilemap2D>(Landstalker::Tilemap2D(m_rom.read_array<uint8_t>(d.tilemap_name), d.map_width, d.map_height, d.map_compression, d.map_base));
			}
			else
			{
				map = std::make_shared<Landstalker::Tilemap2D>(Landstalker::Tilemap2D(d.map_width, d.map_height, d.map_base));
				map->FillIncrementing(0);
			}
			auto pal = std::make_shared<Landstalker::PaletteO>(Landstalker::PaletteO(m_rom, d.palette_idx, d.palette_type));
			ret[name] = { ts, map, pal };
		}
		return ret;
	}
}
#endif