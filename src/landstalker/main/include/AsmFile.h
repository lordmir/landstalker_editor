#ifndef _ASM_FILE_H_
#define _ASM_FILE_H_

#include <cstdint>
#include <string>
#include <deque>
#include <vector>
#include <unordered_map>
#include <variant>
#include <map>
#include <iterator> 
#include <landstalker/misc/include/Utils.h>
#include <filesystem>
#include <optional>

class AsmFile
{
public:
	enum class FileType
	{
		ASSEMBLER,
		BINARY
	};

	enum class Base
	{
		BIN = 2,
		OCT = 8,
		DEC = 10,
		HEX = 16
	};

	struct IncludeFile
	{
		IncludeFile(const std::string& ppath, FileType ptype)
			: path(ReformatPath(ppath)), type(ptype)
		{}
		IncludeFile(const std::filesystem::path& ppath, FileType ptype)
			: path(ppath), type(ptype)
		{}
		IncludeFile() : type(FileType::ASSEMBLER) {}
		operator std::string() { return path.string(); }
		operator const std::filesystem::path& () { return path; }

		std::filesystem::path path;
		FileType type;
	};

	struct Comment
	{
		Comment(const std::string& pcomment) : comment(pcomment) {}
		Comment() {}
		operator const std::string& () { return comment; }
		std::string comment;
	};

	struct Label
	{
		Label(const std::string& plabel) : label(plabel) {}
		Label() {}
		operator const std::string& () { return label; }
		std::string label;
	};

	struct GotoLabel : public Label
	{
		GotoLabel(const std::string& plabel) : Label(plabel) {}
	};

	struct Align
	{
		Align(std::size_t pamount) : amount(pamount) {}
		operator std::size_t() { return amount; }
		std::size_t amount;
	};

	struct ScriptId
	{
		ScriptId(std::size_t p_script_id, std::size_t p_offset)
			: script_id(p_script_id), offset(p_offset) {}
		std::size_t script_id;
		std::size_t offset;
	};

	struct ScriptJump
	{
		ScriptJump(std::string p_func, std::size_t p_offset)
			: func(p_func), offset(p_offset) {}
		std::string func;
		std::size_t offset;
	};

	struct Instruction
	{
		Instruction(const std::string& p_mnemonic, std::size_t p_width = 0, const std::vector<std::variant<std::string, int64_t>>& p_operands = {})
			: mnemonic(p_mnemonic), width(p_width), operands(p_operands) {}
		std::string mnemonic;
		std::size_t width;
		std::vector<std::variant<std::string, int64_t>> operands;
	};

	struct NewLine {};

	using ScriptAction = std::optional<std::variant<ScriptId, ScriptJump>>;

	AsmFile(const std::filesystem::path& filename, FileType type = FileType::ASSEMBLER);
	AsmFile(const std::filesystem::path& filename, const std::vector<std::string>& inc_files);
	AsmFile(const std::filesystem::path& filename, const std::map<std::string, std::string>& defines);
	AsmFile(FileType type = FileType::ASSEMBLER);

	void SetDefines(const std::map<std::string, std::string>& definitions);
	void SetDefines(const std::string& inc_file);
	const std::map<std::string, std::string>& GetDefines() const;

	template<typename T>
	AsmFile& operator<<(const T& data);
	template<typename T>
	AsmFile& operator>>(T& data);
	template<typename T>
	AsmFile& operator>>(const T& data);

	std::vector<uint8_t> ToBinary();
	std::string ToAssembly();
	void Clear();
	void Reset();
	std::string ReadLabel();
	bool IsLabel();
	bool IsLabel(const std::string& label);
	bool LabelExists(const std::string& label);
	bool Goto(const std::string& label);
	bool Goto(const GotoLabel& label);
	bool Goto(const Label& label);

	bool IsGood() const;
	bool ReadFile(const std::filesystem::path& filename, FileType type = FileType::ASSEMBLER);
	bool WriteFile(const std::filesystem::path& filename, FileType type);
	bool WriteFile(const std::filesystem::path& filename);
	std::ostream& PrintFile(std::ostream& stream);

	std::size_t GetLineCount() const;
	std::size_t GetByteCount() const;
	std::string GetFilename() const;
	FileType GetFileType() const;


	void WriteFileHeader(const std::filesystem::path& p, const std::string& short_description);

	template<typename T>
	bool Read(T& value);
	bool Read(const GotoLabel& label);
	template<template<typename, typename...> class C, typename T, typename... Rest>
	bool Read(C<T, Rest...>& container);
	template<typename T, std::size_t N>
	bool Read(std::array<T, N>& container);
	template<typename T, std::size_t N>
	bool Read(T(&array)[N]);
	template<typename T, typename... Args>
	bool Read(T& value, Args&&... args);

	template<typename T>
	bool Write(const T& data);
	bool Write(const std::string& data);
	template<std::size_t N>
	bool Write(const char(&data)[N]);
	bool Write(const Label& label);
	bool Write(const Comment& comment);
	bool Write(const IncludeFile& file);
	bool Write(const NewLine&);
	bool Write(const Align&);
	bool Write(const ScriptId&);
	bool Write(const ScriptJump&);
	bool Write(const Instruction&);
	template<typename T, typename... Args>
	bool Write(const T& first, Args&&... args);
	template<typename Iter>
	bool Write(Iter begin, Iter end);
	template<typename T, std::size_t N>
	bool Write(const T(&val)[N]);
	template<template <typename, std::size_t> typename C, typename T, std::size_t N>
	bool Write(const C<T, N>& val);
	template<template <typename, typename ...> typename C, typename T, typename... Rest>
	bool Write(const C<T, Rest...>& container);

	friend std::ostream& operator<<(std::ostream& stream, AsmFile& file);
private:
	struct AsmLine
	{
		std::string label;
		std::string instruction;
		std::string width;
		std::string operand;
		std::string comment;
		std::string args;
		bool Empty() const;
		void Clear();
	};

	enum class Inst
	{
		GENERIC,
		DC,
		DCB,
		INCLUDE,
		INCBIN,
		ALIGN,
		SCRIPTID,
		SCRIPTJUMP
	};

	bool ParseLine(AsmLine& line, const std::string& str);
	int64_t ParseValue(std::string val);
	std::size_t GetWidth(const AsmLine& line);
	std::string PrintCentered(const std::string& str);

	template<AsmFile::Inst>
	bool ProcessInst(const AsmFile::AsmLine& line);
	bool ProcessLine(const AsmFile::AsmLine& line);
	std::string ToAsmLine(const AsmFile::AsmLine& line);
	template<typename T>
	std::string ToAsmValue(T value, Base base);
	template<typename T>
	std::string ToAsmValue(const T& value);
	std::string ToAsmValue(const std::string& value);
	std::string ToAsmValue(uint64_t value);
	std::string ToAsmValue(int64_t value);
	std::string ToAsmValue(uint32_t value);
	std::string ToAsmValue(int32_t value);
	std::string ToAsmValue(uint16_t value);
	std::string ToAsmValue(int16_t value);
	std::string ToAsmValue(uint8_t value);
	std::string ToAsmValue(int8_t value);
	template<std::size_t N>
	std::string ToAsmValue(const char(&val)[N]);
	template<typename Iter>
	std::string ToAsmValue(Iter begin, Iter end);
	template<typename T, std::size_t N>
	std::string ToAsmValue(const T(&val)[N]);
	template<template <typename, typename ...> typename C, typename T, typename... Rest>
	std::string ToAsmValue(const C<T, Rest...>& container);
	template<template <typename, std::size_t> typename C, typename T, std::size_t N>
	std::string ToAsmValue(const C<T, N>& val);
	void PushNextLine();

	static const std::unordered_map<std::string, Inst> INSTRUCTIONS;
	static const std::unordered_map<std::string, std::size_t> WIDTHS;
	static const std::size_t MAX_ELEMENTS_ON_LINE = 8;

	using AsmData = std::variant<uint8_t, std::string, IncludeFile, ScriptId, ScriptJump, Instruction>;
	bool m_good;
	FileType m_type;
	std::filesystem::path m_filename;
	std::vector<AsmLine> m_asm;
	AsmLine m_nextline;
	std::vector<AsmData> m_data;
	std::vector<AsmData>::const_iterator m_readptr;
	std::map<std::string, std::size_t> m_labels;
	std::map<std::size_t, std::string> m_labelpos;
	std::map<std::string, std::string> m_defines;
};

std::ostream& operator<<(std::ostream& stream, AsmFile& file);

#include <landstalker/main/include/AsmFile_inl.h>

#endif // _ASM_FILE_H_
