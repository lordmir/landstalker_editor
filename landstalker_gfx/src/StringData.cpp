#include "StringData.h"
#include "AsmUtils.h"

StringData::StringData(const filesystem::path& asm_file)
	: DataManager(asm_file)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadSystemFont())
	{
		throw std::runtime_error(std::string("Unable to load system font data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadSystemStrings())
	{
		throw std::runtime_error(std::string("Unable to load system string data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadCompressedStringData())
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from \'") + m_strings_filename.str() + '\'');
	}
	if (!AsmLoadHuffmanData())
	{
		throw std::runtime_error(std::string("Unable to load Huffman data from \'") + asm_file.str() + '\'');
	}
	m_region = Charset::DeduceRegion(GetCharsetSize());
	DecompressStrings();
	InitCache();
}

StringData::StringData(const Rom& rom)
	: DataManager(rom),
	  m_region(rom.get_region())
{
	SetDefaultFilenames();
	if (!RomLoadSystemFont(rom))
	{
		throw std::runtime_error(std::string("Unable to load system font from ROM"));
	}
	if (!RomLoadSystemStrings(rom))
	{
		throw std::runtime_error(std::string("Unable to load system strings from ROM"));
	}
	if (!RomLoadCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from ROM"));
	}
	if (!RomLoadHuffmanData(rom))
	{
		throw std::runtime_error(std::string("Unable to load Huffman data from ROM"));
	}
	DecompressStrings();
	InitCache();
}

bool StringData::Save(const filesystem::path& dir)
{
	auto directory = dir;
	CompressStrings();
	if (directory.exists() && directory.is_file())
	{
		directory = directory.parent_path();
	}
	if (!CreateDirectoryStructure(directory))
	{
		throw std::runtime_error(std::string("Unable to create directory structure at \'") + directory.str() + '\'');
	}
	if (!AsmSaveFonts(dir))
	{
		throw std::runtime_error(std::string("Unable to save font data to \'") + directory.str() + '\'');
	}
	if (!AsmSaveSystemText(dir))
	{
		throw std::runtime_error(std::string("Unable to save system text data to \'") + m_region_check_strings_filename.str() + '\'');
	}
	if (!AsmSaveCompressedStringData(dir))
	{
		throw std::runtime_error(std::string("Unable to save compressed string data to \'") + m_strings_filename.str() + '\'');
	}
	if (!AsmSaveHuffmanData(dir))
	{
		throw std::runtime_error(std::string("Unable to save Huffman data to \'") + directory.str() + '\'');
	}
	CommitAllChanges();
	return true;
}

bool StringData::Save()
{
	return Save(GetBasePath());
}

bool StringData::HasBeenModified() const
{
	auto entry_pred = [](const auto& e) {return e != nullptr && e->HasDataChanged(); };
	auto pair_pred = [](const auto& e) {return e.second != nullptr && e.second->HasDataChanged(); };
	if (std::any_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), pair_pred))
	{
		return true;
	}
	if (m_decompressed_strings_orig != m_decompressed_strings)
	{
		return true;
	}
	if (m_fonts_by_name_orig != m_fonts_by_name)
	{
		return true;
	}
	if (m_ui_tilemaps_orig != m_ui_tilemaps)
	{
		return true;
	}
	if (m_system_strings_orig != m_system_strings)
	{
		return true;
	}
	if (m_huffman_offsets_orig != m_huffman_offsets)
	{
		return true;
	}
	if (m_huffman_tables_orig != m_huffman_tables)
	{
		return true;
	}
	return false;
}

void StringData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
	CompressStrings();
	if (!RomPrepareInjectSystemText(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare system text data for ROM injection"));
	}
	if (!RomPrepareInjectCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare compressed strings for ROM injection"));
	}
	if (!RomPrepareInjectHuffmanData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare Huffman data for ROM injection"));
	}
}

std::map<std::string, std::shared_ptr<TilesetEntry>> StringData::GetAllTilesets() const
{
	return m_fonts_by_name;
}

std::vector<std::shared_ptr<TilesetEntry>> StringData::GetFonts() const
{
	std::vector<std::shared_ptr<TilesetEntry>> result;
	for (auto& t : m_fonts_by_name)
	{
		result.push_back(t.second);
	}
	return result;
}

std::map<std::string, std::shared_ptr<Tilemap2DEntry>> StringData::GetAllMaps() const
{
	return m_ui_tilemaps;
}

std::vector<std::shared_ptr<Tilemap2DEntry>> StringData::GetTextboxMaps() const
{
	std::vector<std::shared_ptr<Tilemap2DEntry>> result;
	for (auto& t : m_ui_tilemaps)
	{
		result.push_back(t.second);
	}
	return result;
}

void StringData::CommitAllChanges()
{
	auto entry_commit = [](const auto& e) {return e->Commit(); };
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };
	std::for_each(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_commit);
	std::for_each(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), pair_commit);
	m_fonts_by_name_orig = m_fonts_by_name;
	m_ui_tilemaps_orig = m_ui_tilemaps;
	m_system_strings_orig = m_system_strings;
	m_decompressed_strings_orig = m_decompressed_strings;
	m_compressed_strings_orig = m_compressed_strings;
	m_huffman_offsets_orig = m_huffman_offsets;
	m_huffman_tables_orig = m_huffman_tables;
	m_pending_writes.clear();
}

bool StringData::LoadAsmFilenames()
{
	try
	{
		bool retval = true;
		AsmFile f(GetAsmFilename().str());
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::REGION_CHECK, m_region_check_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::STRING_SECTION, m_strings_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::STRING_BANK_PTR_DATA, m_string_ptr_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::HUFFMAN_OFFSETS, m_huffman_offset_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::HUFFMAN_TABLES, m_huffman_table_path);
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

void StringData::SetDefaultFilenames()
{
	if (m_region_check_filename.empty()) m_region_check_filename = RomOffsets::Strings::REGION_CHECK_FILE;
	if (m_region_check_routine_filename.empty()) m_region_check_routine_filename = RomOffsets::Strings::REGION_CHECK_ROUTINE_FILE;
	if (m_region_check_strings_filename.empty()) m_region_check_strings_filename = RomOffsets::Strings::REGION_CHECK_STRINGS_FILE;
	if (m_system_font_filename.empty()) m_system_font_filename = RomOffsets::Graphics::SYS_FONT_FILE;
	if (m_strings_filename.empty()) m_strings_filename = RomOffsets::Strings::STRINGS_FILE;
	if (m_string_ptr_filename.empty()) m_string_ptr_filename = RomOffsets::Strings::STRING_BANK_PTR_FILE;
	if (m_string_filename_path.empty()) m_string_filename_path = filesystem::path(RomOffsets::Strings::STRING_BANK_FILE).parent_path();
	if (m_huffman_offset_path.empty()) m_huffman_offset_path = RomOffsets::Strings::HUFFMAN_OFFSETS_FILE;
	if (m_huffman_table_path.empty()) m_huffman_table_path = RomOffsets::Strings::HUFFMAN_TABLE_FILE;
}

bool StringData::CreateDirectoryStructure(const filesystem::path& dir)
{
	bool retval = true;

	retval = retval && CreateDirectoryTree(dir / m_region_check_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_routine_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_system_font_filename);
	retval = retval && CreateDirectoryTree(dir / m_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_ptr_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_filename_path / "x");
	retval = retval && CreateDirectoryTree(dir / m_huffman_offset_path);
	retval = retval && CreateDirectoryTree(dir / m_huffman_table_path);
	for (const auto& f : m_fonts_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& g : m_ui_tilemaps)
	{
		retval = retval && CreateDirectoryTree(dir / g.second->GetFilename());
	}

	return retval;
}

void StringData::InitCache()
{
	m_ui_tilemaps_orig = m_ui_tilemaps;
	m_fonts_by_name_orig = m_fonts_by_name;
	m_system_strings_orig = m_system_strings;
	m_compressed_strings_orig = m_compressed_strings;
	m_decompressed_strings_orig = m_decompressed_strings;
	m_huffman_offsets_orig = m_huffman_offsets;
	m_huffman_tables_orig = m_huffman_tables;
	m_decompressed_strings_orig = m_decompressed_strings;
}

bool StringData::DecompressStrings()
{
	auto huff_trees = std::make_shared<HuffmanTrees>(m_huffman_offsets.data(), m_huffman_offsets.size(), m_huffman_tables.data(), m_huffman_tables.size(), m_huffman_offsets.size() / 2);
	auto charset = Charset::GetDefaultCharset(m_region);
	auto eos_marker = Charset::GetEOSChar(m_region);
	auto diacritic_map = Charset::GetDiacriticMap(m_region);
	auto decoder = HuffmanString(huff_trees, charset, eos_marker, diacritic_map);
	m_decompressed_strings.clear();
	for (const auto& s : m_compressed_strings)
	{
		decoder.Decode(s.data(), s[0]);
		m_decompressed_strings.push_back(decoder.Str());
	}
	return true;
}

bool StringData::CompressStrings()
{
	if (m_decompressed_strings_orig != m_decompressed_strings)
	{
		auto huff_trees = std::make_shared<HuffmanTrees>();
		auto charset = Charset::GetDefaultCharset(m_region);
		auto eos_marker = Charset::GetEOSChar(m_region);
		auto diacritic_map = Charset::GetDiacriticMap(m_region);
		m_compressed_strings.clear();
		std::vector<std::shared_ptr<LSString>> strs;
		for (const auto& s : m_decompressed_strings)
		{
			strs.push_back(std::make_shared<HuffmanString>(s, huff_trees, charset, eos_marker, diacritic_map));
		}
		huff_trees->RecalculateTrees(strs);
		huff_trees->EncodeTrees(m_huffman_offsets, m_huffman_tables);
		for (const auto& s : strs)
		{
			m_compressed_strings.push_back(ByteVector(256));
			auto sz = s->Encode(m_compressed_strings.back().data(), m_compressed_strings.back().size());
			m_compressed_strings.back().resize(sz);
		}
	}
	return true;
}

uint32_t StringData::GetCharsetSize() const
{
	const auto& font = m_fonts_internal.at(RomOffsets::Graphics::MAIN_FONT);
	const auto& orig_font = m_fonts_by_name.at(font->GetName());
	return orig_font->GetData()->GetTileCount();
}

bool StringData::AsmLoadSystemFont()
{
	filesystem::path path = GetBasePath() / m_system_font_filename;
	auto e = TilesetEntry::Create(this, ReadBytes(path), RomOffsets::Graphics::SYS_FONT, m_system_font_filename, false, 8, 8);
	m_fonts_by_name.insert({ RomOffsets::Graphics::SYS_FONT, e });
	m_fonts_internal.insert({ RomOffsets::Graphics::SYS_FONT, e });
	return true;
}

bool StringData::AsmLoadSystemStrings()
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

bool StringData::AsmLoadCompressedStringData()
{
	try
	{
		AsmFile file(GetBasePath() / m_strings_filename);
		AsmFile::Label name;
		AsmFile::IncludeFile inc;
		file >> name >> inc;
		auto font = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, false, 16, 15, 1);
		m_fonts_by_name.insert({ font->GetName(), font });
		m_fonts_internal.insert({ RomOffsets::Graphics::MAIN_FONT, font });
		while (file.IsGood())
		{
			file >> name >> inc;
			m_string_filename_path = inc.path.parent_path();
			auto bytes = ReadBytes(GetBasePath() / inc.path);
			auto it = bytes.cbegin();
			while (it != bytes.cend() && *it != 0x00)
			{
				m_compressed_strings.push_back(ByteVector(it, it + *it));
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

bool StringData::AsmLoadHuffmanData()
{
	filesystem::path path = GetBasePath() / m_huffman_offset_path;
	m_huffman_offsets = ReadBytes(path);
	path = GetBasePath() / m_huffman_table_path;
	m_huffman_tables = ReadBytes(path);
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomOffsets::Graphics::TEXTBOX_2LINE_MAP);
		file >> inc;
		auto tb2 = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::TEXTBOX_2LINE_MAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 6);
		file.Goto(RomOffsets::Graphics::TEXTBOX_3LINE_MAP);
		file >> inc;
		auto tb3 = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::TEXTBOX_3LINE_MAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 8);
		m_ui_tilemaps.insert({ tb2->GetName(), tb2 });
		m_ui_tilemaps_internal.insert({ tb2->GetName(), tb2 });
		m_ui_tilemaps.insert({ tb3->GetName(), tb3 });
		m_ui_tilemaps_internal.insert({ tb3->GetName(), tb3 });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::RomLoadSystemFont(const Rom& rom)
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

bool StringData::RomLoadSystemStrings(const Rom& rom)
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

bool StringData::RomLoadCompressedStringData(const Rom& rom)
{
	uint32_t font_ptr = rom.read<uint32_t>(RomOffsets::Graphics::MAIN_FONT_PTR);
	uint32_t bank_ptr = rom.read<uint32_t>(RomOffsets::Strings::STRING_BANK_PTR_PTR);
	uint32_t banks_begin = rom.read<uint32_t>(bank_ptr);
	auto font_bytes = rom.read_array<uint8_t>(font_ptr, banks_begin - font_ptr);
	auto string_bytes = rom.read_array<uint8_t>(banks_begin, bank_ptr - banks_begin);
	auto it = string_bytes.cbegin();
	while (it != string_bytes.cend() && *it != 0x00)
	{
		m_compressed_strings.push_back(ByteVector(it, it + *it));
		it += *it;
	}
	auto e = TilesetEntry::Create(this, font_bytes, RomOffsets::Graphics::MAIN_FONT,
		RomOffsets::Graphics::MAIN_FONT_FILE, false, 16, 15, 1);
	m_fonts_by_name.insert({ e->GetName(), e });
	m_fonts_internal.insert({ e->GetName(), e });
	return true;
}

bool StringData::RomLoadHuffmanData(const Rom& rom)
{
	uint32_t textbox_2l_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TEXTBOX_2LINE_MAP),
		rom.get_address(RomOffsets::Graphics::TEXTBOX_2LINE_MAP));
	uint32_t textbox_3l_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TEXTBOX_3LINE_MAP),
		rom.get_address(RomOffsets::Graphics::TEXTBOX_3LINE_MAP));
	uint32_t huff_offsets_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Strings::HUFFMAN_OFFSETS),
		rom.get_address(RomOffsets::Strings::HUFFMAN_OFFSETS));
	uint32_t huff_tables_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Strings::HUFFMAN_TABLES),
		rom.get_address(RomOffsets::Strings::HUFFMAN_TABLES));
	uint32_t end = rom.get_section(RomOffsets::Strings::HUFFMAN_SECTION).end;

	uint32_t textbox_3l_size = textbox_2l_addr - textbox_3l_addr;
	uint32_t textbox_2l_size = huff_offsets_addr - textbox_2l_addr;
	uint32_t huff_offsets_size = huff_tables_addr - huff_offsets_addr;
	uint32_t huff_tables_size = end - huff_tables_addr;

	auto textbox_3l_bytes = rom.read_array<uint8_t>(textbox_3l_addr, textbox_3l_size);
	auto textbox_2l_bytes = rom.read_array<uint8_t>(textbox_2l_addr, textbox_2l_size);
	m_huffman_offsets = rom.read_array<uint8_t>(huff_offsets_addr, huff_offsets_size);
	m_huffman_tables = rom.read_array<uint8_t>(huff_tables_addr, huff_tables_size);

	auto textbox_3l = Tilemap2DEntry::Create(this, textbox_3l_bytes, RomOffsets::Graphics::TEXTBOX_3LINE_MAP,
		RomOffsets::Graphics::TEXTBOX_3LINE_MAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 8);
	auto textbox_2l = Tilemap2DEntry::Create(this, textbox_2l_bytes, RomOffsets::Graphics::TEXTBOX_2LINE_MAP,
		RomOffsets::Graphics::TEXTBOX_2LINE_MAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 6);

	m_ui_tilemaps.insert({ textbox_3l->GetName(), textbox_3l });
	m_ui_tilemaps_internal.insert({ textbox_3l->GetName(), textbox_3l });
	m_ui_tilemaps.insert({ textbox_2l->GetName(), textbox_2l });
	m_ui_tilemaps_internal.insert({ textbox_2l->GetName(), textbox_2l });

	return true;
}

bool StringData::AsmSaveFonts(const filesystem::path& dir)
{
	bool retval = std::all_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	return retval;
}

bool StringData::AsmSaveSystemText(const filesystem::path& dir)
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

bool StringData::AsmSaveCompressedStringData(const filesystem::path& dir)
{
	try
	{
		AsmFile sfile, pfile;
		sfile.WriteFileHeader(m_strings_filename, "Compressed Strings");
		pfile.WriteFileHeader(m_string_ptr_filename, "Compressed String Bank Pointers");
		auto font = m_fonts_internal[RomOffsets::Graphics::MAIN_FONT];
		sfile << AsmFile::Label(font->GetName()) << AsmFile::IncludeFile(font->GetFilename(), AsmFile::FileType::BINARY);
		pfile << AsmFile::Label(RomOffsets::Strings::STRING_BANK_PTR);
		int f = 0;
		for (int i = 0; i < m_compressed_strings.size(); i += 256)
		{
			ByteVector bytes;
			for (int j = 0; j < 256 && (i + j) < m_compressed_strings.size(); ++j)
			{
				const auto& s = m_compressed_strings[i + j];
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

bool StringData::AsmSaveHuffmanData(const filesystem::path& dir)
{
	WriteBytes(m_huffman_offsets, dir / m_huffman_offset_path);
	WriteBytes(m_huffman_tables, dir / m_huffman_table_path);
	return true;
}

bool StringData::RomPrepareInjectSystemText(const Rom& rom)
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

bool StringData::RomPrepareInjectCompressedStringData(const Rom& rom)
{
	std::vector<uint32_t> bank_ptrs;
	uint32_t bank_ptrs_ptr;
	ByteVectorPtr bytes = std::make_shared<ByteVector>(*m_fonts_internal[RomOffsets::Graphics::MAIN_FONT]->GetBytes());
	uint32_t begin = rom.get_section(RomOffsets::Strings::STRING_SECTION).begin;
	for (int i = 0; i < m_compressed_strings.size(); i += 256)
	{
		bank_ptrs.push_back(begin + bytes->size());
		for (int j = 0; j < 256 && (i + j) < m_compressed_strings.size(); ++j)
		{
			const auto& s = m_compressed_strings[i + j];
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

bool StringData::RomPrepareInjectHuffmanData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Strings::HUFFMAN_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t textbox_3l_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TEXTBOX_3LINE_MAP), begin);
	auto textbox_3l_bytes = m_ui_tilemaps_internal[RomOffsets::Graphics::TEXTBOX_3LINE_MAP]->GetBytes();
	bytes->insert(bytes->end(), textbox_3l_bytes->cbegin(), textbox_3l_bytes->cend());

	uint32_t textbox_2l_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TEXTBOX_2LINE_MAP), begin + bytes->size());
	auto textbox_2l_bytes = m_ui_tilemaps_internal[RomOffsets::Graphics::TEXTBOX_2LINE_MAP]->GetBytes();
	bytes->insert(bytes->end(), textbox_2l_bytes->cbegin(), textbox_2l_bytes->cend());

	uint32_t huffman_offsets_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Strings::HUFFMAN_OFFSETS), begin + bytes->size());
	bytes->insert(bytes->end(), m_huffman_offsets.cbegin(), m_huffman_offsets.cend());

	uint32_t huffman_tables_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Strings::HUFFMAN_TABLES), begin + bytes->size());
	bytes->insert(bytes->end(), m_huffman_tables.cbegin(), m_huffman_tables.cend());

	m_pending_writes.push_back({ RomOffsets::Strings::HUFFMAN_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::TEXTBOX_3LINE_MAP, std::make_shared<ByteVector>(Split<uint8_t>(textbox_3l_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TEXTBOX_2LINE_MAP, std::make_shared<ByteVector>(Split<uint8_t>(textbox_2l_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::HUFFMAN_OFFSETS, std::make_shared<ByteVector>(Split<uint8_t>(huffman_offsets_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::HUFFMAN_TABLES, std::make_shared<ByteVector>(Split<uint8_t>(huffman_tables_lea)) });

	return true;
}
