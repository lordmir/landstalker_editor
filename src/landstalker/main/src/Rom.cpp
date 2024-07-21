#include <landstalker/main/include/Rom.h>

Rom::Rom(const std::string filename)
	: m_filename(filename),
	  m_initialised(false),
	  m_region(RomOffsets::Region::US)
{
	load_from_file(filename);
}

Rom::Rom(const std::vector<uint8_t>& arr)
	: m_initialised(true),
	  m_rom(arr),
	  m_region(RomOffsets::Region::US)
{
}

Rom::Rom(const Rom& rhs)
	: m_filename(rhs.m_filename),
	  m_initialised(rhs.m_initialised),
	  m_rom(rhs.m_rom),
	  m_region(rhs.m_region)
{}

Rom::Rom()
	: m_initialised(false),
	m_region(RomOffsets::Region::US)
{}

void Rom::load_from_file(std::string filename)
{
	m_filename = filename;
	std::ifstream infile;
	infile.open(filename, std::ios::in | std::ios::binary | std::ios::ate);

	if (!infile.is_open())
	{
		std::ostringstream ss;
		ss << "Unable to open ROM file \"" << filename << "\".";
		throw std::runtime_error(ss.str());
	}

	infile.seekg(0, std::ios::end);
	std::size_t size = static_cast<std::size_t>(infile.tellg());
	infile.seekg(0, std::ios::beg);

	if (size < RomOffsets::EXPECTED_SIZE)
	{
		std::ostringstream ss;
		ss << "ROM file " << filename << ": Bad ROM size! Expected " << std::dec << RomOffsets::EXPECTED_SIZE << " bytes, read " << size << " bytes.";
		throw std::runtime_error(ss.str());
	}

	m_rom.assign(size, 0);
	infile.read(reinterpret_cast<char*>(m_rom.data()), size);
	infile.close();

	m_initialised = true;
	ValidateRomChecksum();
}

Rom& Rom::operator=(const Rom& rhs)
{
	m_rom = rhs.m_rom;
	return *this;
}

void Rom::writeFile(const std::string& filename)
{
	FixRomChecksum();
	WriteBytes(m_rom, filename);
}

std::string Rom::read_string(const std::string& name) const
{
	uint32_t addr = get_address(name);
	return read_string(addr);
}

std::string Rom::read_string(uint32_t offset) const
{
	std::string s;
	char c;
	while (true)
	{
		c = inc_read<uint8_t>(offset);
		if (c == '\0')
		{
			break;
		}
		s += c;
	}
	return s;
}

void Rom::write_string(const std::string& str, const std::string& name)
{
	uint32_t addr = get_address(name);
	write_string(str, addr);
}

void Rom::write_string(const std::string& str, uint32_t offset)
{
	for (char c : str)
	{
		write<uint8_t>(c, offset++);
	}
	write<uint8_t>(0, offset);
}

uint32_t Rom::get_address(const std::string& name) const
{
	auto addrs = RomOffsets::ADDRESS.find(name);
	if (addrs == RomOffsets::ADDRESS.end())
	{
		throw std::runtime_error("Named address " + name + " does not exist!");
	}
	auto addr = addrs->second.find(m_region);
	if (addrs == RomOffsets::ADDRESS.end())
	{
		throw std::runtime_error("Named address " + name + " does not exist for " + RomOffsets::REGION_NAMES.find(m_region)->second + " ROM!");
	}
	return addr->second;
}

RomOffsets::Section Rom::get_section(const std::string& name) const
{
	auto addrs = RomOffsets::SECTION.find(name);
	if (addrs == RomOffsets::SECTION.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist!");
	}
	auto addr = addrs->second.find(m_region);
	if (addrs == RomOffsets::SECTION.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist for " + RomOffsets::REGION_NAMES.find(m_region)->second + " ROM!");
	}
	return addr->second;
}

const uint8_t* Rom::data(uint32_t address) const
{
	return m_rom.data() + address;
}

std::size_t Rom::size(uint32_t address) const
{
	return m_rom.size() - address;
}

void Rom::resize(std::size_t amt)
{
	m_rom.resize(amt);
}

std::string Rom::get_description() const
{
	return "[" + m_filename + "] - " + RomOffsets::REGION_NAMES.find(m_region)->second + " Release ROM";
}

RomOffsets::Region Rom::get_region() const
{
	return m_region;
}

const std::vector<uint8_t>& Rom::get_vec() const
{
	return m_rom;
}

bool Rom::section_exists(const std::string& name)
{
	return RomOffsets::SECTION.find(name) != RomOffsets::SECTION.cend();
}

bool Rom::address_exists(const std::string& name)
{
	return RomOffsets::ADDRESS.find(name) != RomOffsets::ADDRESS.cend();
}

uint16_t Rom::calc_checksum()
{
	uint16_t calculated_checksum = 0x0000;
	for (size_t i = RomOffsets::CHECKSUM_BEGIN; i < RomOffsets::EXPECTED_SIZE; i += 2)
	{
		calculated_checksum += read<uint16_t>(i);
	}
	return calculated_checksum;
}

uint16_t Rom::read_checksum()
{
	return read<uint16_t>(RomOffsets::CHECKSUM_ADDRESS);
}

void Rom::ValidateRomChecksum()
{
	const uint16_t expected_checksum = read_checksum();
	uint16_t calculated_checksum = calc_checksum();
	if (expected_checksum != calculated_checksum)
	{
		std::ostringstream ss;
		ss << "Bad ROM checksum! Expected 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << expected_checksum
			<< " but calculated " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << calculated_checksum << ".";
		Debug(ss.str());
	}

	auto build_date_vec = read_array<char>(RomOffsets::BUILD_DATE_BEGIN, RomOffsets::BUILD_DATE_LENGTH);
	std::string build_date(build_date_vec.begin(), build_date_vec.end());
	auto region = RomOffsets::RELEASE_BUILD_DATE.find(build_date);
	if (region != RomOffsets::RELEASE_BUILD_DATE.end())
	{
		m_region = region->second;
	}
	else
	{
		std::ostringstream ss;
		ss << "Unable to determine ROM region from build date '" << build_date << "'. Assuming ROM is US version.";
		Debug(ss.str());
		m_region = RomOffsets::Region::US;
	}
}

void Rom::FixRomChecksum()
{
	uint16_t calculated_checksum = 0x0000;
	for (size_t i = RomOffsets::CHECKSUM_BEGIN; i < RomOffsets::EXPECTED_SIZE; i += 2)
	{
		calculated_checksum += read<uint16_t>(i);
	}
	write<uint16_t>(calculated_checksum, RomOffsets::CHECKSUM_ADDRESS);
}