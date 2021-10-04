#ifndef ROM_H
#define ROM_H

#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <exception>
#include <vector>
#include <iomanip>

#include <wx/wx.h>

#include "RomOffsets.h"

class Rom
{
public:

	Rom(const std::string filename)
	: m_initialised(false)
	{
		load_from_file(filename);
	}

	Rom(const Rom& rhs)
	: m_rom(rhs.m_rom), m_initialised(rhs.m_initialised)
	{}

	Rom()
	: m_initialised(false)
	{}

	void load_from_file(std::string filename)
	{
		std::ifstream infile;
		infile.open(filename, std::ios::in | std::ios::binary | std::ios::ate);

		if (!infile.is_open())
		{
			std::ostringstream ss;
			ss << "Unable to open ROM file \"" << filename << "\".";
			throw std::runtime_error(ss.str());
		}

		infile.seekg(0, std::ios::end);
		size_t size = static_cast<size_t>(infile.tellg());
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

	Rom& operator=(const Rom& rhs)
	{
		m_rom = rhs.m_rom;
		return *this;
	}

	template< class T >
	T read(size_t offset) const;
	template< class T >
	T read(const std::string& name) const;

	template<class T>
	std::vector<T> read_array(size_t offset, size_t count) const;
	template<class T>
	std::vector<T> read_array(const std::string& name) const;

	template< class T >
	T deref(size_t address, size_t offset = 0) const
	{
		return read<T>(read<uint32_t>(address) + offset * sizeof(T));
	}

	const uint8_t* data(size_t address = 0) const
	{
		return m_rom.data() + address;
	}

private:

	void ValidateRomChecksum()
	{
		const uint16_t expected_checksum = read<uint16_t>(RomOffsets::CHECKSUM_ADDRESS);
		uint16_t calculated_checksum = 0x0000;
		for (size_t i = RomOffsets::CHECKSUM_BEGIN; i < RomOffsets::EXPECTED_SIZE; i += 2)
		{
			calculated_checksum += read<uint16_t>(i);
		}
		if (expected_checksum != calculated_checksum)
		{
			std::ostringstream ss;
			ss << "Bad ROM checksum! Expected 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << expected_checksum
			   << " but calculated " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << calculated_checksum << ".";
			wxMessageBox(ss.str());
		}
		auto region = RomOffsets::RELEASE_CHECKSUM.find(expected_checksum);
		if (region != RomOffsets::RELEASE_CHECKSUM.end())
		{
			m_region = region->second;
		}
		else
		{
			std::ostringstream ss;
			ss << "Unable to determine ROM region. Assuming ROM is US version.";
			wxMessageBox(ss.str());
			m_region = RomOffsets::Region::US;
		}
	}

	bool m_initialised;
	std::vector<uint8_t> m_rom;
	RomOffsets::Region m_region;
};

template< class T >
inline T Rom::read(size_t offset) const
{
	T retval = 0;
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to read from uninitialised ROM.");
	}
	for (size_t i = 0; i < sizeof(T); ++i)
	{
		retval <<= 8;
		retval |= m_rom[offset + i];
	}
	return retval;
}

template<>
inline bool Rom::read<bool>(size_t offset) const
{
	if (m_initialised == false)
	{
		throw std::runtime_error("Attempt to read from uninitialised ROM.");
	}
	return (m_rom[offset] > 0);
}

template<class T>
inline T Rom::read(const std::string& name) const
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
	return read<T>(addr->second);
}

template<class T>
inline std::vector<T> Rom::read_array(size_t offset, size_t count) const
{
	std::vector<T> ret;
	ret.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		ret.push_back(read<T>(offset + i * sizeof(T)));
	}
	return ret;
}

template<>
inline std::vector<bool> Rom::read_array(size_t offset, size_t count) const
{
	std::vector<bool> ret;
	ret.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		ret.push_back(read<bool>(offset + i));
	}
	return ret;
}

template<class T>
inline std::vector<T> Rom::read_array(const std::string& name) const
{
	auto secs = RomOffsets::SECTION.find(name);
	if (secs == RomOffsets::SECTION.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist!");
	}
	auto sec = secs->second.find(m_region);
	if (sec == secs->second.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist for " + RomOffsets::REGION_NAMES.find(m_region)->second + " ROM!");
	}
	return read_array<T>(sec->second.begin, sec->second.size() / sizeof(T));
}

template<>
inline std::vector<bool> Rom::read_array<bool>(const std::string& name) const
{
	auto secs = RomOffsets::SECTION.find(name);
	if (secs == RomOffsets::SECTION.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist!");
	}
	auto sec = secs->second.find(m_region);
	if (sec == secs->second.end())
	{
		throw std::runtime_error("Named section " + name + " does not exist for " + RomOffsets::REGION_NAMES.find(m_region)->second + " ROM!");
	}
	return read_array<bool>(sec->second.begin, sec->second.size());
}

#endif // ROM_H
