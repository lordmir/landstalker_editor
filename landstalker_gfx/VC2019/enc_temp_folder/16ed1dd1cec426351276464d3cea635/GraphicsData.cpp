#include "GraphicsData.h"
#include "AsmUtils.h"

GraphicsData::GraphicsData(const filesystem::path& asm_file)
	: DataManager(asm_file)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadFonts())
	{
		throw std::runtime_error(std::string("Unable to load font data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadStrings())
	{
		throw std::runtime_error(std::string("Unable to load string data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadInventoryGraphics())
	{
		throw std::runtime_error(std::string("Unable to load inventory graphics data from \'") + m_inventory_graphics_filename.str() + '\'');
	}
	if (!AsmLoadCompressedStringData())
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from \'") + m_strings_filename.str() + '\'');
	}
	if (!AsmLoadPalettes())
	{
		throw std::runtime_error(std::string("Unable to load palettes from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadTextGraphics())
	{
		throw std::runtime_error(std::string("Unable to load text graphics from \'") + asm_file.str() + '\'');
	}
	InitCache();
	UpdateTilesetRecommendedPalettes();
	ResetTilesetDefaultPalettes();
}

GraphicsData::GraphicsData(const Rom& rom)
	: DataManager(rom)
{
	SetDefaultFilenames();
	if (!RomLoadFonts(rom))
	{
		throw std::runtime_error(std::string("Unable to load font data from ROM"));
	}
	if (!RomLoadStrings(rom))
	{
		throw std::runtime_error(std::string("Unable to load string data from ROM"));
	}
	if (!RomLoadInventoryGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to load inventory graphics from ROM"));
	}
	if (!RomLoadCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from ROM"));
	}
	if (!RomLoadPalettes(rom))
	{
		throw std::runtime_error(std::string("Unable to load palettes from ROM"));
	}
	if (!RomLoadTextGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to load text graphics from ROM"));
	}
	InitCache();
	UpdateTilesetRecommendedPalettes();
	ResetTilesetDefaultPalettes();
}

bool GraphicsData::Save(const filesystem::path& dir)
{
	auto directory = dir;
	if (directory.exists() && directory.is_file())
	{
		directory = directory.parent_path();
	}
	if (!CreateDirectoryStructure(directory))
	{
		throw std::runtime_error(std::string("Unable to create directory structure at \'") + directory.str() + '\'');
	}
	if (!AsmSaveStrings(dir))
	{
		throw std::runtime_error(std::string("Unable to save string data to \'") + m_region_check_strings_filename.str() + '\'');
	}
	if (!AsmSaveGraphics(dir))
	{
		throw std::runtime_error(std::string("Unable to save graphics data to \'") + directory.str() + '\'');
	}
	if (!AsmSaveInventoryGraphics(dir))
	{
		throw std::runtime_error(std::string("Unable to save inventory graphics data to \'") + m_inventory_graphics_filename.str() + '\'');
	}
	if (!AsmSaveCompressedStringData(dir))
	{
		throw std::runtime_error(std::string("Unable to save compressed string data to \'") + m_strings_filename.str() + '\'');
	}
	return true;
}

bool GraphicsData::Save()
{
	return Save(GetBasePath());
}

bool GraphicsData::HasBeenModified() const
{
	auto entry_pred = [](const auto& e) {return e != nullptr && e->HasDataChanged(); };
	auto pair_pred = [](const auto& e) {return e.second != nullptr && e.second->HasDataChanged(); };
	if (std::any_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_pred))
	{
		return true;
	}
	if (m_fonts_by_name != m_fonts_by_name_orig)
	{
		return true;
	}
	if (m_system_strings != m_system_strings_orig)
	{
		return true;
	}
	if (m_palettes_by_name_orig != m_palettes_by_name)
	{
		return true;
	}
	if (m_misc_gfx_by_name_orig != m_misc_gfx_by_name)
	{
		return true;
	}
	return false;
}

void GraphicsData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
	if (!RomPrepareInjectFonts(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare fonts for ROM injection"));
	}
	if (!RomPrepareInjectInvGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare inventory graphics for ROM injection"));
	}
	if (!RomPrepareInjectCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare compressed strings for ROM injection"));
	}
	if (!RomPrepareInjectPalettes(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare palettes for ROM injection"));
	}
	if (!RomPrepareInjectTextGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare text graphics for ROM injection"));
	}
}

std::map<std::string, std::shared_ptr<TilesetEntry>> GraphicsData::GetAllTilesets() const
{
	std::map<std::string, std::shared_ptr<TilesetEntry>> result;
	for (auto& t : m_fonts_by_name)
	{
		result.insert(t);
	}
	for (auto& t : m_misc_gfx_by_name)
	{
		result.insert(t);
	}
	return result;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetFonts() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_fonts_by_name)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetMiscGraphics() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_misc_gfx_by_name)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::map<std::string, std::shared_ptr<PaletteEntry>> GraphicsData::GetAllPalettes() const
{
	std::map<std::string, std::shared_ptr<PaletteEntry>> result;
	for (auto& t : m_palettes_by_name)
	{
		result.insert(t);
	}
	return result;
}

void GraphicsData::CommitAllChanges()
{
	auto entry_commit = [](const auto& e) {return e->Commit(); };
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };
	std::for_each(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_commit);
	m_fonts_by_name_orig = m_fonts_by_name;
	m_system_strings_orig = m_system_strings;
	m_palettes_by_name_orig = m_palettes_by_name;
	m_misc_gfx_by_name_orig = m_misc_gfx_by_name;
	m_strings_orig = m_strings;
	m_pending_writes.clear();
}

bool GraphicsData::LoadAsmFilenames()
{
	try
	{
		bool retval = true;
		AsmFile f(GetAsmFilename().str());
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::REGION_CHECK, m_region_check_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::INV_SECTION, m_inventory_graphics_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::STRING_SECTION, m_strings_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::STRING_BANK_PTR_DATA, m_string_ptr_filename);
		AsmFile r(GetBasePath() / m_region_check_filename);
		retval = retval && GetFilenameFromAsm(r, RomOffsets::Strings::REGION_CHECK_ROUTINE, m_region_check_routine_filename);
		retval = retval && GetFilenameFromAsm(r, RomOffsets::Strings::REGION_CHECK_STRINGS, m_region_check_strings_filename);
		retval = retval && GetFilenameFromAsm(r, RomOffsets::Graphics::SYS_FONT, m_system_font_filename);
		
		return retval;
	}
	catch (...)
	{
	}
	return false;
}

void GraphicsData::SetDefaultFilenames()
{
	if (m_region_check_filename.empty()) m_region_check_filename = RomOffsets::Strings::REGION_CHECK_FILE;
	if (m_region_check_routine_filename.empty()) m_region_check_routine_filename = RomOffsets::Strings::REGION_CHECK_ROUTINE_FILE;
	if (m_region_check_strings_filename.empty()) m_region_check_strings_filename = RomOffsets::Strings::REGION_CHECK_STRINGS_FILE;
	if (m_system_font_filename.empty()) m_system_font_filename = RomOffsets::Graphics::SYS_FONT_FILE;
	if (m_inventory_graphics_filename.empty()) m_inventory_graphics_filename = RomOffsets::Graphics::INV_GRAPHICS_FILE;
	if (m_strings_filename.empty()) m_strings_filename = RomOffsets::Strings::STRINGS_FILE;
	if (m_string_ptr_filename.empty()) m_string_ptr_filename = RomOffsets::Strings::STRING_BANK_PTR_FILE;
	if (m_string_filename_path.empty()) m_string_filename_path = filesystem::path(RomOffsets::Strings::STRING_BANK_FILE).parent_path();
}

bool GraphicsData::CreateDirectoryStructure(const filesystem::path& dir)
{
	bool retval = true;
	retval = retval && CreateDirectoryTree(dir / m_region_check_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_routine_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_system_font_filename);
	retval = retval && CreateDirectoryTree(dir / m_inventory_graphics_filename);
	retval = retval && CreateDirectoryTree(dir / m_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_ptr_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_filename_path / "x");
	for (const auto& f : m_fonts_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& g : m_misc_gfx_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / g.second->GetFilename());
	}
	for (const auto& p : m_palettes_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / p.second->GetFilename());
	}
	return retval;
}

void GraphicsData::InitCache()
{
	m_palettes_by_name_orig = m_palettes_by_name;
	m_system_strings_orig = m_system_strings;
	m_fonts_by_name_orig = m_fonts_by_name;
	m_misc_gfx_by_name_orig = m_misc_gfx_by_name;
	m_strings_orig = m_strings;
}

bool GraphicsData::AsmLoadFonts()
{
	filesystem::path path = GetBasePath() / m_system_font_filename;
	auto e = TilesetEntry::Create(this, ReadBytes(path), RomOffsets::Graphics::SYS_FONT, m_system_font_filename, false, 8, 8);
	m_fonts_by_name.insert({ RomOffsets::Graphics::SYS_FONT, e });
	m_fonts_internal.insert({ RomOffsets::Graphics::SYS_FONT, e });
	return true;
}

bool GraphicsData::AsmLoadStrings()
{
	try
	{
		AsmFile file(GetBasePath() / m_region_check_strings_filename);
		auto read_str = [&](std::string& s)
		{
			char c;
			do
			{
				file >> c;
				if (c != 0)
				{
					s += c;
				}
			} while (c != 0);
		};
		file.Goto(RomOffsets::Strings::REGION_ERROR_LINE1);
		read_str(m_system_strings[0]);
		file.Goto(RomOffsets::Strings::REGION_ERROR_NTSC);
		read_str(m_system_strings[1]);
		file.Goto(RomOffsets::Strings::REGION_ERROR_PAL);
		read_str(m_system_strings[2]);
		file.Goto(RomOffsets::Strings::REGION_ERROR_LINE3);
		read_str(m_system_strings[3]);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadInventoryGraphics()
{
	try
	{
		AsmFile file(GetBasePath() / m_inventory_graphics_filename);
		AsmFile::Label name;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, filesystem::path, ByteVectorPtr>> gfx;
		for(int i = 0; i < 7; ++i)
		{
			file >> name >> inc;
			auto path = GetBasePath() / inc.path;
			auto raw_data = std::make_shared<std::vector<uint8_t>>(ReadBytes(path));
			gfx.push_back({ name, inc.path, raw_data });
		}
		auto menufont = TilesetEntry::Create(this, *std::get<2>(gfx[0]), std::get<0>(gfx[0]),
			std::get<1>(gfx[0]), false, 8, 8, 1);
		auto menucursor = TilesetEntry::Create(this, *std::get<2>(gfx[1]), std::get<0>(gfx[1]),
			std::get<1>(gfx[1]), false, 8, 8, 2, Tileset::BLOCK4X4);
		auto arrow = TilesetEntry::Create(this, *std::get<2>(gfx[2]), std::get<0>(gfx[2]),
			std::get<1>(gfx[2]), false, 8, 8, 2, Tileset::BLOCK2X2);
		auto unused1 = TilesetEntry::Create(this, *std::get<2>(gfx[3]), std::get<0>(gfx[3]),
			std::get<1>(gfx[3]), false, 8, 11, 2);
		auto unused2 = TilesetEntry::Create(this, *std::get<2>(gfx[4]), std::get<0>(gfx[4]),
			std::get<1>(gfx[4]), false, 8, 10, 2);
		auto pal1 = PaletteEntry::Create(this, *std::get<2>(gfx[5]), std::get<0>(gfx[5]),
			std::get<1>(gfx[5]), Palette::Type::LOW8);
		auto pal2 = PaletteEntry::Create(this, *std::get<2>(gfx[6]), std::get<0>(gfx[6]),
			std::get<1>(gfx[6]), Palette::Type::FULL);
		m_fonts_by_name.insert({ menufont->GetName(), menufont });
		m_misc_gfx_by_name.insert({ menucursor->GetName(), menucursor });
		m_misc_gfx_by_name.insert({ arrow->GetName(), arrow });
		m_misc_gfx_by_name.insert({ unused1->GetName(), unused1 });
		m_misc_gfx_by_name.insert({ unused2->GetName(), unused2 });
		m_palettes_by_name.insert({ pal1->GetName(), pal1 });
		m_palettes_by_name.insert({ pal2->GetName(), pal2 });
		m_fonts_internal.insert({ menufont->GetName(), menufont });
		m_misc_gfx_internal.insert({ menucursor->GetName(), menucursor });
		m_misc_gfx_internal.insert({ arrow->GetName(), arrow });
		m_misc_gfx_internal.insert({ unused1->GetName(), unused1 });
		m_misc_gfx_internal.insert({ unused2->GetName(), unused2 });
		m_palettes_internal.insert({ pal1->GetName(), pal1 });
		m_palettes_internal.insert({ pal2->GetName(), pal2 });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadCompressedStringData()
{
	try
	{
		AsmFile file(GetBasePath() / m_strings_filename);
		AsmFile::Label name;
		AsmFile::IncludeFile inc;
		file >> name >> inc;
		auto font = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, false, 16, 15, 1);
		m_fonts_by_name.insert({ font->GetName(), font});
		m_fonts_internal.insert({ RomOffsets::Graphics::MAIN_FONT, font });
		while (file.IsGood())
		{
			file >> name >> inc;
			m_string_filename_path = inc.path.parent_path();
			auto bytes = ReadBytes(GetBasePath() / inc.path);
			auto it = bytes.cbegin();
			while (it != bytes.cend() && *it != 0x00)
			{
				m_strings.push_back(ByteVector(it, it + *it));
				it += *it;
			}
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadPalettes()
{
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomOffsets::Graphics::PLAYER_PAL);
		file >> inc;
		auto player_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::PLAYER_PAL, inc.path, Palette::Type::FULL);
		file.Goto(RomOffsets::Graphics::HUD_PAL);
		file >> inc;
		auto hud_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::HUD_PAL, inc.path, Palette::Type::HUD);
		file.Goto(RomOffsets::Graphics::INV_ITEM_PAL);
		file >> inc;
		auto item_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::INV_ITEM_PAL, inc.path, Palette::Type::HUD);
		m_palettes_by_name.insert({ player_pal->GetName(), player_pal });
		m_palettes_by_name.insert({ hud_pal->GetName(), hud_pal });
		m_palettes_by_name.insert({ item_pal->GetName(), item_pal });
		m_palettes_internal.insert({ player_pal->GetName(), player_pal });
		m_palettes_internal.insert({ hud_pal->GetName(), hud_pal });
		m_palettes_internal.insert({ item_pal->GetName(), item_pal });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadTextGraphics()
{
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomOffsets::Graphics::DOWN_ARROW);
		file >> inc;
		auto down_arrow = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::DOWN_ARROW, inc.path, false, 8, 8, 4, Tileset::BLOCK1X2);
		file.Goto(RomOffsets::Graphics::RIGHT_ARROW);
		file >> inc;
		auto right_arrow = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::RIGHT_ARROW, inc.path, false, 8, 8, 4, Tileset::BLOCK2X1);
		m_misc_gfx_by_name.insert({ down_arrow->GetName(), down_arrow });
		m_misc_gfx_by_name.insert({ down_arrow->GetName(), down_arrow });
		m_misc_gfx_internal.insert({ right_arrow->GetName(), right_arrow });
		m_misc_gfx_internal.insert({ right_arrow->GetName(), right_arrow });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::RomLoadFonts(const Rom& rom)
{
	uint32_t sys_font_lea = rom.read<uint32_t>(RomOffsets::Graphics::SYS_FONT);
	uint32_t sys_font_begin = Disasm::LEA_PCRel(sys_font_lea, rom.get_address(RomOffsets::Graphics::SYS_FONT));
	uint32_t sys_font_size = rom.get_address(RomOffsets::Graphics::SYS_FONT_SIZE);
	auto sys_font_bytes = rom.read_array<uint8_t>(sys_font_begin, sys_font_size);
	auto sys_font = TilesetEntry::Create(this, sys_font_bytes, RomOffsets::Graphics::SYS_FONT,
		RomOffsets::Graphics::SYS_FONT_FILE, false);
	sys_font->SetStartAddress(sys_font_begin);
	m_fonts_by_name.insert({ sys_font->GetName(), sys_font });
	m_fonts_internal.insert({ sys_font->GetName(), sys_font });
	return true;
}

bool GraphicsData::RomLoadStrings(const Rom& rom)
{
	uint32_t line1_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_LINE1);
	uint32_t line1_begin = Disasm::LEA_PCRel(line1_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE1));
	m_system_strings[0] = rom.read_string(line1_begin);
	uint32_t line2_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_NTSC);
	uint32_t line2_begin = Disasm::LEA_PCRel(line2_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_NTSC));
	m_system_strings[1] = rom.read_string(line2_begin);
	uint32_t line3_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_PAL);
	uint32_t line3_begin = Disasm::LEA_PCRel(line3_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_PAL));
	m_system_strings[2] = rom.read_string(line3_begin);
	uint32_t line4_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_LINE3);
	uint32_t line4_begin = Disasm::LEA_PCRel(line4_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE3));
	m_system_strings[3] = rom.read_string(line4_begin);
	return true;
}

bool GraphicsData::RomLoadInventoryGraphics(const Rom& rom)
{
	auto load_bytes = [&](const std::string& lea_loc, const std::string& size)
	{
		uint32_t pc = rom.get_address(lea_loc);
		uint32_t lea = rom.read<uint32_t>(pc);
		uint32_t start = Disasm::LEA_PCRel(lea, pc);
		uint32_t sz = rom.get_address(size);
		return rom.read_array<uint8_t>(start, sz);
	};
	auto load_pal = [&](const std::string& lea_loc, Palette::Type type)
	{
		uint32_t pc = rom.get_address(lea_loc);
		uint32_t lea = rom.read<uint32_t>(pc);
		uint32_t start = Disasm::LEA_PCRel(lea, pc);
		uint32_t sz = Palette::GetSizeBytes(type);
		return rom.read_array<uint8_t>(start, sz);
	};
	auto menu_font = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_FONT, RomOffsets::Graphics::INV_FONT_SIZE),
		RomOffsets::Graphics::INV_FONT, RomOffsets::Graphics::INV_FONT_FILE, false, 8, 8, 1);
	auto cursor = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_CURSOR, RomOffsets::Graphics::INV_CURSOR_SIZE),
		RomOffsets::Graphics::INV_CURSOR, RomOffsets::Graphics::INV_CURSOR_FILE, false, 8, 8, 2, Tileset::BLOCK4X4);
	auto arrow = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_ARROW, RomOffsets::Graphics::INV_ARROW_SIZE),
		RomOffsets::Graphics::INV_ARROW, RomOffsets::Graphics::INV_ARROW_FILE, false, 8, 8, 2, Tileset::BLOCK2X2);
	auto unused1 = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_UNUSED1, RomOffsets::Graphics::INV_UNUSED1_SIZE),
		RomOffsets::Graphics::INV_UNUSED1, RomOffsets::Graphics::INV_UNUSED1_FILE, false, 8, 11, 2);
	auto unused2 = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_UNUSED2, RomOffsets::Graphics::INV_UNUSED2_SIZE),
		RomOffsets::Graphics::INV_UNUSED2, RomOffsets::Graphics::INV_UNUSED2_FILE, false, 8, 10, 2);
	auto pal1 = PaletteEntry::Create(this, load_pal(RomOffsets::Graphics::INV_PAL1, Palette::Type::LOW8),
		RomOffsets::Graphics::INV_PAL1, RomOffsets::Graphics::INV_PAL1_FILE, Palette::Type::LOW8);
	auto pal2 = PaletteEntry::Create(this, load_pal(RomOffsets::Graphics::INV_PAL2, Palette::Type::FULL),
		RomOffsets::Graphics::INV_PAL2, RomOffsets::Graphics::INV_PAL2_FILE, Palette::Type::FULL);
	m_fonts_by_name.insert({ menu_font->GetName(), menu_font });
	m_fonts_internal.insert({ RomOffsets::Graphics::INV_FONT, menu_font });
	m_misc_gfx_by_name.insert({ cursor->GetName(), cursor });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_CURSOR, cursor });
	m_misc_gfx_by_name.insert({ arrow->GetName(), arrow });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_ARROW, arrow });
	m_misc_gfx_by_name.insert({ unused1->GetName(), unused1 });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_UNUSED1, unused1 });
	m_misc_gfx_by_name.insert({ unused2->GetName(), unused2 });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_UNUSED2, unused2 });
	m_palettes_by_name.insert({ pal1->GetName(), pal1 });
	m_palettes_internal.insert({ RomOffsets::Graphics::INV_PAL1, pal1 });
	m_palettes_by_name.insert({ pal2->GetName(), pal2 });
	m_palettes_internal.insert({ RomOffsets::Graphics::INV_PAL2, pal2 });
	return true;
}

bool GraphicsData::RomLoadCompressedStringData(const Rom& rom)
{
	uint32_t font_ptr = rom.read<uint32_t>(RomOffsets::Graphics::MAIN_FONT_PTR);
	uint32_t bank_ptr = rom.read<uint32_t>(RomOffsets::Strings::STRING_BANK_PTR_PTR);
	uint32_t banks_begin = rom.read<uint32_t>(bank_ptr);
	auto font_bytes = rom.read_array<uint8_t>(font_ptr, banks_begin - font_ptr);
	auto string_bytes = rom.read_array<uint8_t>(banks_begin, bank_ptr - banks_begin);
	auto it = string_bytes.cbegin();
	while (it != string_bytes.cend() && *it != 0x00)
	{
		m_strings.push_back(ByteVector(it, it + *it));
		it += *it;
	}
	auto e = TilesetEntry::Create(this, font_bytes, RomOffsets::Graphics::MAIN_FONT,
		RomOffsets::Graphics::MAIN_FONT_FILE, false, 16, 15, 1);
	m_fonts_by_name.insert({ e->GetName(), e });
	m_fonts_internal.insert({ e->GetName(), e });
	return true;
}

bool GraphicsData::RomLoadPalettes(const Rom& rom)
{
	uint32_t player_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::PLAYER_PAL),
		rom.get_address(RomOffsets::Graphics::PLAYER_PAL));
	uint32_t hud_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::HUD_PAL),
		rom.get_address(RomOffsets::Graphics::HUD_PAL));
	uint32_t item_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::INV_ITEM_PAL),
		rom.get_address(RomOffsets::Graphics::INV_ITEM_PAL));
	auto player_bytes = rom.read_array<uint8_t>(player_addr, Palette::GetSizeBytes(Palette::Type::FULL));
	auto hud_bytes = rom.read_array<uint8_t>(hud_addr, Palette::GetSizeBytes(Palette::Type::HUD));
	auto item_bytes = rom.read_array<uint8_t>(item_addr, Palette::GetSizeBytes(Palette::Type::FULL));
	auto player_pal = PaletteEntry::Create(this, player_bytes, RomOffsets::Graphics::PLAYER_PAL,
		RomOffsets::Graphics::PLAYER_PAL_FILE, Palette::Type::FULL);
	auto hud_pal = PaletteEntry::Create(this, hud_bytes, RomOffsets::Graphics::HUD_PAL,
		RomOffsets::Graphics::HUD_PAL_FILE, Palette::Type::HUD);
	auto item_pal = PaletteEntry::Create(this, item_bytes, RomOffsets::Graphics::INV_ITEM_PAL,
		RomOffsets::Graphics::INV_ITEM_PAL_FILE, Palette::Type::FULL);
	m_palettes_by_name.insert({ player_pal->GetName(), player_pal });
	m_palettes_internal.insert({ player_pal->GetName(), player_pal });
	m_palettes_by_name.insert({ hud_pal->GetName(), hud_pal });
	m_palettes_internal.insert({ hud_pal->GetName(), hud_pal });
	m_palettes_by_name.insert({ item_pal->GetName(), item_pal });
	m_palettes_internal.insert({ item_pal->GetName(), item_pal });
	return true;
}

bool GraphicsData::RomLoadTextGraphics(const Rom& rom)
{
	uint32_t down_arrow_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::DOWN_ARROW),
		rom.get_address(RomOffsets::Graphics::DOWN_ARROW));
	uint32_t down_arrow_size = rom.get_section(RomOffsets::Graphics::DOWN_ARROW_SECTION).size();
	uint32_t right_arrow_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::RIGHT_ARROW),
		rom.get_address(RomOffsets::Graphics::RIGHT_ARROW));
	uint32_t right_arrow_size = rom.get_section(RomOffsets::Graphics::RIGHT_ARROW_SECTION).size();
	auto down_arrow_bytes = rom.read_array<uint8_t>(down_arrow_addr, down_arrow_size);
	auto right_arrow_bytes = rom.read_array<uint8_t>(right_arrow_addr, right_arrow_size);
	auto down_arrow = TilesetEntry::Create(this, down_arrow_bytes, RomOffsets::Graphics::DOWN_ARROW,
		RomOffsets::Graphics::DOWN_ARROW_FILE, false, 8, 8, 4, Tileset::BLOCK2X2);
	auto right_arrow = TilesetEntry::Create(this, right_arrow_bytes, RomOffsets::Graphics::RIGHT_ARROW,
		RomOffsets::Graphics::RIGHT_ARROW_FILE, false, 8, 8, 4, Tileset::BLOCK2X2);
	down_arrow->SetStartAddress(down_arrow_size);
	right_arrow->SetStartAddress(right_arrow_size);
	m_misc_gfx_by_name.insert({ down_arrow->GetName(), down_arrow });
	m_misc_gfx_by_name.insert({ right_arrow->GetName(), right_arrow });
	m_misc_gfx_internal.insert({ down_arrow->GetName(), down_arrow });
	m_misc_gfx_internal.insert({ right_arrow->GetName(), right_arrow });
	return true;
}

bool GraphicsData::AsmSaveGraphics(const filesystem::path& dir)
{
	bool retval = std::all_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_misc_gfx_by_name.begin(), m_misc_gfx_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_palettes_by_name.begin(), m_palettes_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	return retval;
}

bool GraphicsData::AsmSaveStrings(const filesystem::path& dir)
{
	try
	{
		AsmFile sfile, ifile;
		ByteVector str[4];
		ifile.WriteFileHeader(m_region_check_filename, "Region Check");
		ifile << AsmFile::Label(RomOffsets::Strings::REGION_CHECK_ROUTINE) << AsmFile::IncludeFile(m_region_check_routine_filename, AsmFile::FileType::ASSEMBLER);
		ifile << AsmFile::Label(RomOffsets::Strings::REGION_CHECK_STRINGS) << AsmFile::IncludeFile(m_region_check_strings_filename, AsmFile::FileType::ASSEMBLER);
		ifile << AsmFile::Align(2) << AsmFile::Label(RomOffsets::Graphics::SYS_FONT) << AsmFile::IncludeFile(m_system_font_filename, AsmFile::FileType::BINARY);
		ifile.WriteFile(dir / m_region_check_filename);
		sfile.WriteFileHeader(m_region_check_strings_filename, "Region Check System Strings");
		for (int i = 0; i < 4; ++i)
		{
			str[i].insert(str[i].end(), m_system_strings[i].cbegin(), m_system_strings[i].cend());
			str[i].push_back(0);
		}
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_LINE1) << str[0];
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_NTSC) << str[1];
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_PAL) << str[2];
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_LINE3) << str[3];
		sfile.WriteFile(dir / m_region_check_strings_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveInventoryGraphics(const filesystem::path& dir)
{

	try
	{
		AsmFile ifile;
		auto write_inc = [&](const std::string& name, const auto& container)
		{
			auto e = container.find(name)->second;
			ifile << AsmFile::Label(name) << AsmFile::IncludeFile(e->GetFilename(), AsmFile::FileType::BINARY);
		};
		ifile.WriteFileHeader(m_inventory_graphics_filename, "Inventory Graphics Data");
		write_inc(RomOffsets::Graphics::INV_FONT, m_fonts_internal);
		write_inc(RomOffsets::Graphics::INV_CURSOR, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_ARROW, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_UNUSED1, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_UNUSED2, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_PAL1, m_palettes_internal);
		write_inc(RomOffsets::Graphics::INV_PAL2, m_palettes_internal);
		ifile.WriteFile(dir / m_inventory_graphics_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveCompressedStringData(const filesystem::path& dir)
{
	try
	{
		AsmFile sfile, pfile;
		sfile.WriteFileHeader(m_strings_filename, "Compressed Strings");
		pfile.WriteFileHeader(m_string_ptr_filename, "Compressed String Bank Pointers");
		auto font = m_fonts_internal[RomOffsets::Graphics::MAIN_FONT];
		sfile << AsmFile::Label(font->GetName()) << AsmFile::IncludeFile(font->GetFilename(),AsmFile::FileType::BINARY);
		pfile << AsmFile::Label(RomOffsets::Strings::STRING_BANK_PTR);
		int f = 0;
		for (int i = 0; i < m_strings.size(); i += 256)
		{
			ByteVector bytes;
			for (int j = 0; j < 256 && (i + j) < m_strings.size(); ++j)
			{
				const auto& s = m_strings[i + j];
				bytes.insert(bytes.end(), s.cbegin(), s.cend());
			}
			filesystem::path fname = StrPrintf(RomOffsets::Strings::STRING_BANK_FILE, f + 1);
			std::string pname = StrPrintf(RomOffsets::Strings::STRING_BANK, f);
			WriteBytes(bytes, dir / m_string_filename_path / fname.filename());
			pfile << pname;
			sfile << AsmFile::Label(pname) << AsmFile::IncludeFile(fname, AsmFile::FileType::BINARY);
			f++;
		}
		sfile.WriteFile(dir / m_strings_filename);
		pfile.WriteFile(dir / m_string_ptr_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::RomPrepareInjectFonts(const Rom& rom)
{
	auto system_font_bytes = m_fonts_by_name[RomOffsets::Graphics::SYS_FONT]->GetBytes();
	ByteVectorPtr bytes = std::make_shared<ByteVector>();
	auto write_string = [](ByteVectorPtr& bytes, const std::string& in)
	{
		for (char c : in)
		{
			bytes->push_back(c);
		}
		bytes->push_back(0);
	};
	uint32_t addrs[5];
	uint32_t begin = rom.get_section(RomOffsets::Strings::REGION_CHECK_DATA_SECTION).begin;
	for (int i = 0; i < 4; ++i)
	{
		addrs[i] = begin + bytes->size();
		write_string(bytes, m_system_strings[i]);
	}
	if ((bytes->size() & 1) == 1)
	{
		bytes->push_back(0xFF);
	}
	addrs[4] = begin + bytes->size();
	bytes->insert(bytes->end(), system_font_bytes->cbegin(), system_font_bytes->cend());
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_CHECK_DATA_SECTION, bytes });
	uint32_t line1_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE1), addrs[0]);
	uint32_t line2_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_NTSC), addrs[1]);
	uint32_t line3_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_PAL), addrs[2]);
	uint32_t line4_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE3), addrs[3]);
	uint32_t font_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::SYS_FONT), addrs[4]);

	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_LINE1, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line1_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_NTSC, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line2_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_PAL, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line3_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_LINE3, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line4_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::SYS_FONT, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(font_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectInvGraphics(const Rom& rom)
{
	ByteVectorPtr bytes = std::make_shared<ByteVector>();
	uint32_t base = rom.get_section(RomOffsets::Graphics::INV_SECTION).begin;
	uint32_t addrs[7];
	addrs[0] = base + bytes->size();
	auto inv_font = m_fonts_internal[RomOffsets::Graphics::INV_FONT]->GetBytes();
	bytes->insert(bytes->end(), inv_font->cbegin(), inv_font->cend());
	addrs[1] = base + bytes->size();
	auto inv_cursor = m_misc_gfx_internal[RomOffsets::Graphics::INV_CURSOR]->GetBytes();
	bytes->insert(bytes->end(), inv_cursor->cbegin(), inv_cursor->cend());
	addrs[2] = base + bytes->size();
	auto inv_arrow = m_misc_gfx_internal[RomOffsets::Graphics::INV_ARROW]->GetBytes();
	bytes->insert(bytes->end(), inv_arrow->cbegin(), inv_arrow->cend());
	addrs[3] = base + bytes->size();
	auto inv_unused1 = m_misc_gfx_internal[RomOffsets::Graphics::INV_UNUSED1]->GetBytes();
	bytes->insert(bytes->end(), inv_unused1->cbegin(), inv_unused1->cend());
	addrs[4] = base + bytes->size();
	auto inv_unused2 = m_misc_gfx_internal[RomOffsets::Graphics::INV_UNUSED2]->GetBytes();
	bytes->insert(bytes->end(), inv_unused2->cbegin(), inv_unused2->cend());
	addrs[5] = base + bytes->size();
	auto inv_pal1 = m_palettes_internal[RomOffsets::Graphics::INV_PAL1]->GetBytes();
	bytes->insert(bytes->end(), inv_pal1->cbegin(), inv_pal1->cend());
	addrs[6] = base + bytes->size();
	auto inv_pal2 = m_palettes_internal[RomOffsets::Graphics::INV_PAL2]->GetBytes();
	bytes->insert(bytes->end(), inv_pal2->cbegin(), inv_pal2->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::INV_SECTION, bytes });

	uint32_t font_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_FONT), addrs[0]);
	uint32_t cursor_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_CURSOR), addrs[1]);
	uint32_t arrow_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_ARROW), addrs[2]);
	uint32_t unused1_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED1), addrs[3]);
	uint32_t unused1p6_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED1_PLUS6), addrs[3] + 12);
	uint32_t unused2_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED2), addrs[4]);
	uint32_t unused2p4_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED2_PLUS4), addrs[4] + 8);
	uint32_t pal1_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::INV_PAL1), addrs[5]);
	uint32_t pal2_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::INV_PAL2), addrs[6]);

	m_pending_writes.push_back({ RomOffsets::Graphics::INV_FONT, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(font_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_CURSOR, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(cursor_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_ARROW, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(arrow_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED1, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused1_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED1_PLUS6, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused1p6_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED2, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused2_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED2_PLUS4, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused2p4_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_PAL1, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(pal1_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_PAL2, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(pal2_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectCompressedStringData(const Rom& rom)
{
	std::vector<uint32_t> bank_ptrs;
	uint32_t bank_ptrs_ptr;
	ByteVectorPtr bytes = std::make_shared<ByteVector>(*m_fonts_internal[RomOffsets::Graphics::MAIN_FONT]->GetBytes());
	uint32_t begin = rom.get_section(RomOffsets::Strings::STRING_SECTION).begin;
	for (int i = 0; i < m_strings.size(); i += 256)
	{
		bank_ptrs.push_back(begin + bytes->size());
		for (int j = 0; j < 256 && (i + j) < m_strings.size(); ++j)
		{
			const auto& s = m_strings[i + j];
			bytes->insert(bytes->end(), s.cbegin(), s.cend());
		}
	}
	while ((bytes->size() & 3) != 0)
	{
		bytes->push_back(0);
	}
	bank_ptrs_ptr = bytes->size() + begin;
	for (auto p : bank_ptrs)
	{
		auto b = Split<uint8_t, uint32_t>(p);
		bytes->insert(bytes->end(), b.cbegin(), b.cend());
	}
	m_pending_writes.push_back({ RomOffsets::Strings::STRING_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Strings::STRING_BANK_PTR_PTR, std::make_shared<ByteVector>(Split<uint8_t>(bank_ptrs_ptr)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::MAIN_FONT_PTR, std::make_shared<ByteVector>(Split<uint8_t>(begin)) });
	return true;
}

bool GraphicsData::RomPrepareInjectPalettes(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Graphics::MISC_PAL_SECTION).begin;
	uint32_t player_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::PLAYER_PAL), begin);
	auto bytes = std::make_shared<ByteVector>();
	auto player_pal = m_palettes_internal[RomOffsets::Graphics::PLAYER_PAL]->GetBytes();
	bytes->insert(bytes->end(), player_pal->cbegin(), player_pal->cend());
	uint32_t hud_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::HUD_PAL), begin + bytes->size());
	uint32_t hud_lea2 = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::HUD_PAL_LEA2), begin + bytes->size());
	auto hud_pal = m_palettes_internal[RomOffsets::Graphics::HUD_PAL]->GetBytes();
	bytes->insert(bytes->end(), hud_pal->cbegin(), hud_pal->cend());
	
	uint32_t item_pal_begin = rom.get_section(RomOffsets::Graphics::INV_ITEM_PAL_SECTION).begin;
	uint32_t item_pal_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::INV_ITEM_PAL), item_pal_begin);
	auto item_pal = m_palettes_internal[RomOffsets::Graphics::INV_ITEM_PAL]->GetBytes();

	m_pending_writes.push_back({ RomOffsets::Graphics::MISC_PAL_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::PLAYER_PAL, std::make_shared<ByteVector>(Split<uint8_t>(player_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::HUD_PAL, std::make_shared<ByteVector>(Split<uint8_t>(hud_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::HUD_PAL_LEA2, std::make_shared<ByteVector>(Split<uint8_t>(hud_lea2)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_ITEM_PAL_SECTION, item_pal });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_ITEM_PAL, std::make_shared<ByteVector>(Split<uint8_t>(item_pal_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectTextGraphics(const Rom& rom)
{
	uint32_t down_arrow_begin = rom.get_section(RomOffsets::Graphics::DOWN_ARROW_SECTION).begin;
	uint32_t down_arrow_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::DOWN_ARROW), down_arrow_begin);
	auto down_arrow_bytes = m_misc_gfx_internal[RomOffsets::Graphics::DOWN_ARROW]->GetBytes();
	uint32_t right_arrow_begin = rom.get_section(RomOffsets::Graphics::RIGHT_ARROW_SECTION).begin;
	uint32_t right_arrow_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::RIGHT_ARROW), right_arrow_begin);
	auto right_arrow_bytes = m_misc_gfx_internal[RomOffsets::Graphics::RIGHT_ARROW]->GetBytes();

	m_pending_writes.push_back({ RomOffsets::Graphics::DOWN_ARROW_SECTION, down_arrow_bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::RIGHT_ARROW_SECTION, right_arrow_bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::DOWN_ARROW, std::make_shared<ByteVector>(Split<uint8_t>(down_arrow_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::RIGHT_ARROW, std::make_shared<ByteVector>(Split<uint8_t>(right_arrow_lea)) });

	return true;
}

void GraphicsData::UpdateTilesetRecommendedPalettes()
{
	std::vector<std::string> palettes;
	for (const auto& p : GetAllPalettes())
	{
		palettes.push_back(p.first);
	}
	auto set_pals = [&](auto& container)
	{
		for (auto& e : container)
		{
			e.second->SetAllPalettes(palettes);
			e.second->SetRecommendedPalettes(palettes);
		}
	};
	set_pals(m_fonts_by_name);
	set_pals(m_misc_gfx_by_name);
}

void GraphicsData::ResetTilesetDefaultPalettes()
{
	auto set_pals = [&](auto& container)
	{
		for (const auto& e : container)
		{
			if (e.second->GetRecommendedPalettes().size() == 0)
			{
				e.second->SetDefaultPalette(m_palettes_by_name.begin()->second->GetName());
			}
			else
			{
				e.second->SetDefaultPalette(e.second->GetRecommendedPalettes().front());
			}
		}
	};
	set_pals(m_fonts_by_name);
	set_pals(m_misc_gfx_by_name);
}
