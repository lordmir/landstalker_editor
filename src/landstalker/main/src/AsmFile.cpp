#include <landstalker/main/include/AsmFile.h>

#include <regex>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>

const std::unordered_map<std::string, AsmFile::Inst> AsmFile::INSTRUCTIONS{ {"dc", Inst::DC}, {"dcb", Inst::DCB}, {"include", Inst::INCLUDE}, {"incbin", Inst::INCBIN}, {"Align", Inst::ALIGN} };
const std::unordered_map<std::string, std::size_t> AsmFile::WIDTHS{ {"", 0}, {"b", 1}, {"w", 2}, {"l", 4}, {"s", 99} };

AsmFile::AsmFile(const filesystem::path& filename, FileType type)
	: m_type(type),
	  m_filename(filename)
{
	if (!ReadFile(m_filename, m_type))
	{
		throw std::runtime_error(std::string("File \'") + m_filename.str() + ("\' cannot be read!"));
	}
}

AsmFile::AsmFile(FileType type)
	: m_type(type)
{
	Clear();
}

std::vector<uint8_t> AsmFile::ToBinary()
{
	std::vector<uint8_t> data;
	for (const auto& elem : m_data)
	{
		if (std::holds_alternative<uint8_t>(elem))
		{
			data.push_back(std::get<uint8_t>(elem));
		}
		else if (std::holds_alternative<std::string>(elem))
		{

		}
	}
	return data;
}

std::string AsmFile::ToAssembly()
{
	std::string ass;
	for (const auto& line : m_asm)
	{
		ass += ToAsmLine(line) + "\n";
	}
	return ass;
}

void AsmFile::Clear()
{
	m_asm.clear();
	m_data.clear();
	m_labels.clear();
	m_labelpos.clear();
	m_nextline.Clear();
	m_readptr = m_data.begin();
	m_good = true;
}

void AsmFile::Reset()
{
	m_readptr = m_data.begin();
	m_good = true;
}

std::string AsmFile::ReadLabel()
{
	std::size_t pos = m_readptr - m_data.begin();
	if (m_labelpos.find(pos) == m_labelpos.end())
	{
		return std::string();
	}
	return m_labelpos[pos];
}

bool AsmFile::IsLabel()
{
	std::size_t pos = m_readptr - m_data.begin();
	return m_labelpos.find(pos) != m_labelpos.end();
}

bool AsmFile::IsLabel(const std::string& label)
{
	return ReadLabel() == label;
}

bool AsmFile::LabelExists(const std::string& label)
{
	return m_labels.find(label) != m_labels.cend();
}

bool AsmFile::Goto(const std::string& label)
{
	return Goto(GotoLabel(label));
}

bool AsmFile::Goto(const GotoLabel& label)
{
	return Read(label);
}

bool AsmFile::Goto(const Label& label)
{
	return Goto(GotoLabel(label.label));
}

bool AsmFile::IsGood() const
{
	return m_good && (m_readptr != m_data.end());
}

bool AsmFile::ReadFile(const filesystem::path& filename, FileType type)
{
	m_filename = filename;
	m_type = type;
	Clear();
	try
	{
		if (m_type == FileType::ASSEMBLER)
		{
			std::vector<AsmLine> lines;
			std::ifstream ifs(m_filename.str());
			std::string line;
			AsmLine asml;
			while (std::getline(ifs, line))
			{
				if (ParseLine(asml, line))
				{
					if (!asml.label.empty())
					{
						if (m_labels.find(asml.label) != m_labels.end())
						{
							std::cerr << "Duplicate label [" << asml.label << "] in " << m_filename << std::endl;
							std::cerr << "  First defined on byte " << (m_labels[asml.label] + 1) << std::endl;
							std::cerr << "  Now encountered on byte " << (m_data.size() + 1);
						}
						else
						{
							m_labels.insert({ asml.label, m_data.size() });
						}
						m_labelpos.insert({ m_data.size(), asml.label });
					}
					m_asm.push_back(asml);
					ProcessLine(asml);
				}
			}
		}
		else
		{
			std::ifstream ifs(m_filename.str(), std::ios::binary);
			ifs.unsetf(std::ios::skipws);
			m_data.insert(m_data.begin(),
				std::istream_iterator<uint8_t>(ifs),
				std::istream_iterator<uint8_t>());
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Unhandled exception: " << e.what() << std::endl;
		m_readptr = m_data.begin();
		return false;
	}
	m_readptr = m_data.begin();
	return true;
}

bool AsmFile::WriteFile(const filesystem::path& filename, FileType type)
{
	try
	{
		if (!m_nextline.instruction.empty())
		{
			PushNextLine();
		}
		if (type == FileType::BINARY)
		{
			WriteBytes(ToBinary(), filename.str());
			return true;
		}
		else
		{
			std::ofstream ofs(filename.str());
			ofs << ToAssembly();
		}
	}
	catch (const std::exception& e)
	{
		Debug(e.what());
	}
	return false;
}

bool AsmFile::WriteFile(const filesystem::path& filename)
{
	return WriteFile(filename, m_type);
}

std::ostream& AsmFile::PrintFile(std::ostream& stream)
{
	if (!m_nextline.Empty())
	{
		PushNextLine();
	}
	if (m_type == FileType::ASSEMBLER)
	{
		for (const auto& line : m_asm)
		{
			stream << ToAsmLine(line) << std::endl;
		}
	}
	else
	{
		auto bin = ToBinary();
		stream << HexArray(bin.begin(), bin.end());
	}
	return stream;
}

std::size_t AsmFile::GetLineCount() const
{
	return m_asm.size();
}

std::size_t AsmFile::GetByteCount() const
{
	return m_data.size();
}

std::string AsmFile::GetFilename() const
{
	return m_filename.str();
}

AsmFile::FileType AsmFile::GetFileType() const
{
	return m_type;
}

std::string AsmFile::PrintCentered(const std::string& str)
{
	std::size_t width = 72;
	std::string start = ";; ";
	std::string end = " ;;";
	if(str.length() >= width)
	{
		return start + str + end;
	}
	std::size_t spacing = (width - str.length()) / 2;
	std::string result(spacing, ' ');
	result += str;
	result += std::string(width - result.length(), ' ');
	return start + result + end;
}

void AsmFile::WriteFileHeader(const filesystem::path& p, const std::string& short_description)
{
	*this << AsmFile::Comment(std::string(78, ';'));
	*this << AsmFile::Comment(PrintCentered(short_description));
	*this << AsmFile::Comment(PrintCentered(p.str()));
	*this << AsmFile::Comment(PrintCentered(""));
	*this << AsmFile::Comment(PrintCentered("Generated using the Landstalker Editor v0.3.4:"));
	*this << AsmFile::Comment(PrintCentered("https://github.com/lordmir/landstalker_editor"));
	*this << AsmFile::Comment(PrintCentered("For use with the Landstalker disassembly:"));
	*this << AsmFile::Comment(PrintCentered("https://github.com/lordmir/landstalker_disasm"));
	*this << AsmFile::Comment(std::string(78, ';'));
	*this << AsmFile::NewLine() << AsmFile::NewLine();
}

bool AsmFile::Read(const GotoLabel& label)
{
	if (m_labels.find(label.label) == m_labels.end())
	{
		return false;
	}
	m_readptr = m_data.begin() + m_labels[label.label];
	m_good = true;
	return true;
}

template<>
bool AsmFile::Read(filesystem::path& path)
{
	bool ret = false;
	AsmFile::IncludeFile file;
	if (m_readptr != m_data.end())
	{
		try
		{
			file = std::get<IncludeFile>(*m_readptr++);
			ret = true;
		}
		catch (const std::bad_variant_access&)
		{
			return false;
		}
	}
	path = file.path;
	return ret;
}

template <>
bool AsmFile::Read(std::string& value)
{
	bool ret = true;
	if (m_readptr != m_data.end())
	{
		try
		{
			if (std::holds_alternative<std::string>(*m_readptr))
			{
				value = std::get<std::string>(*m_readptr++);
			}
			else
			{
				uint32_t addr = 0;
				ret = Read(addr);
				if (ret == true)
				{
					std::ostringstream ss;
					ss << Hex(addr);
					value = ss.str();
				}
			}
		}
		catch (const std::bad_variant_access&)
		{
			return false;
		}
	}
	return ret;
}

template <>
bool AsmFile::Read(IncludeFile& value)
{
	bool ret = false;
	if (m_readptr != m_data.end())
	{
		try
		{
			value = std::get<IncludeFile>(*m_readptr++);
			ret = true;
		}
		catch (const std::bad_variant_access&)
		{
			return false;
		}
	}
	return ret;
}
template <>
bool AsmFile::Read(Label& value)
{
	std::size_t pos = m_readptr - m_data.begin();
	if (m_labelpos.find(pos) == m_labelpos.end())
	{
		return false;
	}
	value = m_labelpos[pos];
	return true;
}

bool AsmFile::Write(const std::string& data)
{
	if (!m_nextline.instruction.empty())
	{
		PushNextLine();
	}
	m_nextline.instruction = FindMapKey(INSTRUCTIONS, Inst::DC)->first;
	m_nextline.width = FindMapKey(WIDTHS, sizeof(uint32_t))->first;
	m_nextline.operand = data;
	return true;
}

bool AsmFile::Write(const Label& label)
{
	PushNextLine();
	m_nextline.label = label.label;
	return true;
}

bool AsmFile::Write(const Comment& comment)
{
	if (!m_nextline.comment.empty())
	{
		PushNextLine();
	}
	if (comment.comment[0] != ';')
	{
		m_nextline.comment += "; ";
	}
	m_nextline.comment += comment.comment;
	return true;
}

bool AsmFile::Write(const IncludeFile& file)
{
	if (!m_nextline.instruction.empty())
	{
		PushNextLine();
	}
	m_nextline.instruction = FindMapKey(INSTRUCTIONS, file.type == FileType::ASSEMBLER ? Inst::INCLUDE : Inst::INCBIN)->first;
	m_nextline.operand = "\"";
	m_nextline.operand += file.path.str();
	m_nextline.operand += "\"";
	return true;
}

bool AsmFile::Write(const NewLine&)
{
	PushNextLine();
	return true;
}

bool AsmFile::Write(const Align& align)
{
	if (!m_nextline.instruction.empty())
	{
		PushNextLine();
	}
	m_nextline.instruction = FindMapKey(INSTRUCTIONS, Inst::ALIGN)->first;
	m_nextline.operand = ToAsmValue(align.amount);
	return true;
}

bool AsmFile::ParseLine(AsmFile::AsmLine& line, const std::string& str)
{
	std::string s(str);
	size_t end = 0;
	end = s.find(";");
	if (end != std::string::npos)
	{
		line.comment = Trim(s.substr(end));
		s = s.substr(0, end);
	}
	else
	{
		line.comment = "";
	}
	if (!std::isspace(s[0]))
	{
		end = s.find(":");
		if (end == std::string::npos) return false;
		line.label = Trim(s.substr(0, end));
		s = s.substr(end + 1);
	}
	else
	{
		line.label = "";
	}
	s = Trim(s);
	end = s.find_first_of(" \t");
	if (end != std::string::npos)
	{
		line.instruction = Trim(s.substr(0, end));
		s = s.substr(end + 1);
	}
	else
	{
		return false;
	}
	line.operand = Trim(s);
	end = line.instruction.find_first_of(".");
	if (end != std::string::npos)
	{
		line.width = line.instruction[end + 1];
		line.instruction = line.instruction.substr(0, end);
	}

	return true;
}

int64_t AsmFile::ParseValue(std::string val)
{
	const std::unordered_map<char, int> bases{ {'$', 16}, {'@', 8}, {'%', 2} };

	val.erase(std::remove_if(val.begin(), val.end(), ::isspace), val.end());
	int64_t num = -1;
	bool neg = false;
	if (val.length() == 0)
	{
		return num;
	}
	if (val[0] == '-')
	{
		neg = true;
		val.erase(0, 1);
	}
	if (bases.find(val[0]) != bases.end())
	{
		int base = bases.find(val[0])->second;
		val.erase(0, 1);
		num = std::stoull(val, nullptr, base);
	}
	else if (std::isdigit(val[0]))
	{
		num = std::stoull(val, nullptr);
	}
	else
	{
		m_data.push_back(val);
		return -1;
	}
	if (neg)
	{
		num = (~num + 1) & 0xFFFFFFFF;
	}
	return num;
}

int AsmFile::GetWidth(const AsmLine& line)
{
	const std::unordered_map<std::string, int> widths{ {"b", 1}, {"w", 2}, {"l", 4} };

	if (widths.find(line.width) == widths.end())
	{
		return 0;
	}
	return widths.find(line.width)->second;
}

template<>
bool AsmFile::ProcessInst<AsmFile::Inst::DC>(const AsmFile::AsmLine& line)
{
	int width = GetWidth(line);
	if (width == 0)
	{
		return false;
	}
	std::string word;
	std::stringstream ss(line.operand);
	while (ss.good())
	{
		std::getline(ss, word, ',');
		auto result = ParseValue(word);
		if (result != -1)
		{
			for (int i = 0; i < width; ++i)
			{
				uint8_t byte = (result >> ((width - i - 1) * 8)) & 0xFF;
				m_data.push_back(byte);
			}
		}
	}
	return true;
}

template<>
bool AsmFile::ProcessInst<AsmFile::Inst::DCB>(const AsmFile::AsmLine& line)
{
	int width = GetWidth(line);
	if (width == 0)
	{
		return false;
	}

	std::string word;

	std::stringstream ss(line.operand);
	std::getline(ss, word, ',');
	auto repeats = ParseValue(word);
	std::getline(ss, word, ',');
	for (int i = 0; i < repeats; ++i)
	{
		auto val = ParseValue(word);
		if (val != -1)
		{
			for (int j = 0; j < width; ++j)
			{
				uint8_t byte = (val >> ((width - j - 1) * 8)) & 0xFF;
				m_data.push_back(byte);
			}
		}
	}

	return true;
}

template<>
bool AsmFile::ProcessInst<AsmFile::Inst::INCLUDE>(const AsmFile::AsmLine& line)
{
	m_data.push_back(IncludeFile(line.operand, FileType::ASSEMBLER));
	return true;
}

template<>
bool AsmFile::ProcessInst<AsmFile::Inst::INCBIN>(const AsmFile::AsmLine& line)
{
	m_data.push_back(IncludeFile(line.operand, FileType::BINARY));
	return true;
}

template<>
bool AsmFile::ProcessInst<AsmFile::Inst::ALIGN>(const AsmFile::AsmLine& /*line*/)
{
	return true;
}

bool AsmFile::ProcessLine(const AsmFile::AsmLine& line)
{
	auto it = INSTRUCTIONS.find(line.instruction);
	if (it == INSTRUCTIONS.end())
	{
		return false;
	}
	switch (it->second)
	{
	case Inst::DC:      return ProcessInst<Inst::DC>(line);
	case Inst::DCB:     return ProcessInst<Inst::DCB>(line);
	case Inst::INCLUDE: return ProcessInst<Inst::INCLUDE>(line);
	case Inst::INCBIN:  return ProcessInst<Inst::INCBIN>(line);
	case Inst::ALIGN:   return ProcessInst<Inst::ALIGN>(line);
	default:            return false;
	}
}

std::string AsmFile::ToAsmLine(const AsmFile::AsmLine& line)
{
	std::ostringstream ss;
	if (!line.label.empty())
	{
		ss << std::setw(20) << std::left;
		ss << (line.label + ":");
	}
	else if (!line.instruction.empty())
	{
		ss << std::string(20, ' ');
	}
	if (!line.instruction.empty())
	{
		ss << std::setw(0) << ' ';
		if (line.width.empty())
		{
			ss << std::setw(8) << std::left << line.instruction;
		}
		else
		{
			ss << std::setw(8) << std::left << (line.instruction + "." + line.width);
		}
		if (!line.args.empty())
		{
			ss << line.instruction + "(" + line.args + ")";
		}
		if (!line.operand.empty())
		{
			ss << ' ';
			if (!line.comment.empty())
			{
				ss << std::setw(10) << std::left;
			}
			ss << line.operand;
		}
	}
	if (!line.comment.empty())
	{
		ss << std::setw(0);
		if (!line.instruction.empty())
		{
			ss << "    ";
		}
		ss << line.comment;
	}
	return ss.str();
}

std::string AsmFile::ToAsmValue(const std::string& value)
{
	return value;
}

std::string AsmFile::ToAsmValue(uint64_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

std::string AsmFile::ToAsmValue(int64_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

std::string AsmFile::ToAsmValue(uint32_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

std::string AsmFile::ToAsmValue(int32_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

std::string AsmFile::ToAsmValue(uint16_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

std::string AsmFile::ToAsmValue(int16_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

std::string AsmFile::ToAsmValue(uint8_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

std::string AsmFile::ToAsmValue(int8_t value)
{
	return 	ToAsmValue(value, Base::HEX);
}

void AsmFile::PushNextLine()
{
	m_asm.push_back(m_nextline);
	ProcessLine(m_nextline);
	m_nextline.Clear();
}

bool AsmFile::AsmLine::Empty() const
{
	return (label.empty() &&
		instruction.empty() &&
		width.empty() &&
		operand.empty() &&
		comment.empty() &&
		args.empty());
}

void AsmFile::AsmLine::Clear()
{
	label = "";
	instruction = "";
	width = "";
	args = "";
	operand = "";
	comment = "";
}

std::ostream& operator<<(std::ostream& stream, AsmFile& file)
{
	return file.PrintFile(stream);
}
